import sys

import time
import urllib
from collections import deque

from NBodyRender import *

try:
    import asyncio
except ImportError:
    import trollius as asyncio

import pyndn


def findSegmentNumber(name):
    for i in range(0, name.size()):
        component = name.get(i)
        if component.isSegment():
            return component.toSegment()
    return -1


def findIntermediateNumber(name):
    for i in range(0, name.size()):
        component = name.get(i)
        if component.toEscapedString() == "INTERMEDIATE" and i < name.size() - 1:
            return int(name.get(i + 1).toEscapedString())
    return -1


def interestToString(interest):
    uri = interest.getName().toUri()
    cmps = uri.split("/")
    lastComponent = cmps[-1]
    isSegment = lastComponent.startswith("%00")
    if isSegment:
        cmps.pop()
    string = urllib.unquote("/".join(cmps))
    if isSegment:
        string += "/" + lastComponent
    return string


class SimpleRequest():
    def __init__(self, face, interest, onData, onTimeout):
        self.data = None
        self.timeout = False
        self.face = face
        self.interest = interest
        self.onData = onData
        self.onTimeout = onTimeout

        self.redirect = None
        self.segmentCount = 0
        self.nextSegmentIndex = 0
        self.pendingSegments = set()
        # self.pendingSegmentMax = 10
        # self.pendingSegmentCount = 0
        self.segments = {}

    def receiveData(self, interest, data):
        content = data.getContent().toRawStr()
        redirectPrefix = "redirect:"
        if content.startswith(redirectPrefix):
            name = content[len(redirectPrefix):]
            self.redirect = pyndn.Interest(pyndn.Name(name))
            print("Received redirect for interest '{}' -> '{}':\n{}"
                  .format(interestToString(interest), interestToString(self.redirect), data.getContent()))
            self.requestNextSegment()
        else:
            print("Received data for interest '{}':\n{}".format(interestToString(interest), urllib.unquote(content)))
            self.data = data
            self.onData(self)

    def timeoutData(self, interest):
        print("Timed out interest '{}'.".format(interest.getName()))
        self.onTimeout(self)

    def send(self):
        print("Sent interest '{}'.".format(interestToString(self.interest)))
        self.face.expressInterest(self.interest, self.receiveData, self.timeoutData)
        # print("Lifetime: {}".format(self.interest.getInterestLifetimeMilliseconds()))

    def requestNextSegments(self):
        if self.segmentCount == 0:
            self.requestNextSegment()
        else:
            while self.nextSegmentIndex < self.segmentCount:
                self.requestNextSegment()

    def requestNextSegment(self):
        if self.nextSegmentIndex < self.segmentCount or self.segmentCount == 0:
            self.requestSegment(self.nextSegmentIndex)
            self.nextSegmentIndex += 1

    def requestSegment(self, segmentNumber):
        # if self.pendingSegmentCount < self.pendingSegmentMax:
        self.pendingSegments.add(segmentNumber)
        segmentName = pyndn.Name(self.redirect.getName())
        segmentName.appendSegment(segmentNumber)
        interest = pyndn.Interest(segmentName)
        print("Sent segment interest '{}'".format(interestToString(interest)))
        self.face.expressInterest(interest, self.receiveSegment, self.timeoutSegment)

    def receiveSegment(self, interest, data):
        if self.segmentCount == 0:
            self.segmentCount = data.getMetaInfo().getFinalBlockId().toNumber() + 1

        content = data.getContent()
        if not content.isNull():
            size = content.size()
            name = interest.getName()
            segmentNumber = findSegmentNumber(name)
            print("Received data ({} bytes) for segment '{}'.".format(size, interestToString(interest)))
            self.segments[segmentNumber] = data
            self.pendingSegments.remove(segmentNumber)
            allSegmentsReceived = self.nextSegmentIndex >= self.segmentCount and self.pendingSegments.__len__() == 0
            if allSegmentsReceived:
                self.handleCompletion()
            else:
                self.requestNextSegments()

        else:
            print("Received EMPTY data for segment '{}'.".format(interestToString(interest)))

    def timeoutSegment(self, interest):
        print("Timed out interest '{}'.".format(interest.getName()))
        self.requestSegment(findSegmentNumber(interest.getName()))  # retry

    def handleCompletion(self):
        # print(self.segments)
        content = bytearray()
        for i in sorted(self.segments):
            segment = self.segments[i]
            content.extend(segment.getContent().buf())
        blob = pyndn.Blob(content)
        data = pyndn.Data(self.interest.getName())
        data.setContent(blob)
        self.data = data
        self.onData(self)


class KeepaliveRequest():
    def __init__(self, face, interest, onIntermediateData, onData, onTimeout):
        self.face = face
        self.interest = interest
        self.onIntermediateData = onIntermediateData
        self.onData = onData
        self.onTimeout = onTimeout

        self.data = None
        self.intermediateData = None
        self.intermediateIndex = -1
        self.timeout = False
        self.intermediateCount = -1
        self.childRequests = []

    def send(self):
        # print("Sending interest '{}'.".format(interestToString(self.interest)))
        request = SimpleRequest(self.face, self.interest, self.receiveData, self.timeoutData)
        self.childRequests.append(request)
        request.send()

    def receiveData(self, request):
        interest = request.interest
        data = request.data
        # print("Received data for interest '{}':\n{}".format(interestToString(interest), data.getContent()))
        self.childRequests.remove(request)
        self.data = data
        self.onData(self)

    def timeoutData(self, request):
        interest = request.interest
        # print("Timed out interest '{}'.".format(interestToString(interest)))
        self.childRequests.remove(request)
        self.sendKeepalive()
        self.send()

    def sendKeepalive(self):
        name = self.interest.getName()
        name = name.getSubName(0, name.size() - 1).append("ALIVE").append("NFN")
        interest = pyndn.Interest(name)
        print("Sending keepalive interest '{}'.".format(interestToString(interest)))
        self.face.expressInterest(interest, self.receiveKeepalive, self.timeoutKeepalive)

    def receiveKeepalive(self, interest, data):
        print("Received data for keepalive interest '{}':\n{}".format(interestToString(interest), data.getContent()))
        if not data.getContent().isNull() and data.getContent().size() > 0:
            maxIntermediate = int(data.getContent().toRawStr())
            for i in range(self.intermediateCount + 1, maxIntermediate + 1):
                self.sendIntermediate(i)
            self.intermediateCount = maxIntermediate

    def timeoutKeepalive(self, interest):
        print("Timed out keepalive interest '{}'.".format(interestToString(interest)))
        self.timeout = True
        self.onTimeout(self)

    def sendIntermediate(self, index):
        name = self.interest.getName()
        name = name.getSubName(0, name.size() - 1).append("INTERMEDIATE").append(str(index)).append("NFN")
        interest = pyndn.Interest(name)
        # print("Sending intermediate interest '{}'.".format(interestToString(interest)))
        # self.face.expressInterest(interest, self.receiveIntermediate, self.timeoutIntermediate)
        request = SimpleRequest(self.face, interest, self.receiveIntermediate, self.timeoutIntermediate)
        self.childRequests.append(request)
        request.send()

    def receiveIntermediate(self, request):
        interest = request.interest
        data = request.data
        self.intermediateIndex = findIntermediateNumber(interest.getName())
        self.intermediateData = data
        # print("Received data for intermediate interest '{}':\n{}".format(interestToString(interest), data.getContent()))
        self.childRequests.remove(request)
        self.onIntermediateData(self)

    def timeoutIntermediate(self, request):
        interest = request.interest
        # print("Timed out intermediate interest '{}'.".format(interestToString(interest)))
        self.childRequests.remove(request)


