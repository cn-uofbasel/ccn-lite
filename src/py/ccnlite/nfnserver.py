# ccn-lite/src/py/ccnlite/nfnserver.py

'''
CCN-lite module for Python:
a NFN server supporting the publishing of user defined Python functions

Copyright (C) 2014, Michaja Pressmar, University of Basel

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

File history:
Nov 2014    created
2015-06-15  <christian.tschudin@unibas.ch> adapted for ccn-lite
            (removed all dependencies on pyndn, must still do it for asyncio)
'''

try:
    # Use builtin asyncio on Python 3.4+, or Tulip on Python 3.3
    import asyncio
except ImportError:
    # Use Trollius on Python <= 3.2
    import trollius as asyncio

import cStringIO
import importlib
# import logging
import signal
import socket
import sys
import time

# ccn-lite specific:
import client
import ndn2013 as ndn
import util

# logging.basicConfig()

class NFNserver():

    class FunctionRetriever():

        def __init__(self, argNum, resultHandle):
            self.dataList = range(argNum)
            self.resultHandle = resultHandle
            self.counter = argNum
            if (argNum == 0):
                self.resultHandle(self.dataList)

        def notify(self, data, index):
            self.counter -= 1
            if (len(self.dataList) == 1 and data == None):
                self.dataList = []
            else:
                self.dataList[index] = data
            if (self.counter == 0):
                self.resultHandle(self.dataList)

    class Notifier(object):

        def __init__(self, index, notifyFunction):
            self.index = index
            self.notifyFunction = notifyFunction

        def onData(self, data):
            self.notifyFunction(data, self.index)

    #Starts a NFN server that will listen on the specified port and react to interests.
    #Currently no concurrent processing of interests is implemented, the server will block until the current computation is done.
    #Parameters:
    #    listenPort
    #        the port the server is going to listen for interests on. Register this port as a UDPFace in a ccn-nfn-relay
    #    faceIP, facePort
    #        the address of the ccn-nfn-relay that should be used for expressing interests and answering with data
    def __init__(self, listenPort, faceIP, facePort):
        self.interrupted = False
        signal.signal(signal.SIGINT, self.onExit)

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(("127.0.0.1", listenPort))

        self.loop = asyncio.get_event_loop()
        self._access = client.Access()
        self._access.connect(faceIP, facePort)

        print "Server listening on port", listenPort, "."

        while (self.interrupted == False):
            # print "Waiting for interests..."
            pkt, addr = self._sock.recvfrom(8192)
            #Server will block until computation is done
            self.processingInterest = True
            self.loop.call_soon(self.stopLoopWhenDone)

            name = ndn.parseInterest(cStringIO.StringIO(pkt))
            self.onInterest(name, addr)
            self.loop.run_forever()

    def onReceivedData(self, data): # ignore data
        pass

    def stopLoopWhenDone(self):
        if (self.processingInterest == False):
            self.loop.stop()
        else:
            self.loop.call_later(0.5, self.stopLoopWhenDone)

    def onInterest(self, name, addr):
        expression = name[-2]
        self.resolve(expression,
                     lambda data: self.returnData(addr, name, data))

    def returnData(self, addr, name, result):
        if type(result) is str:
            pkt = ndn.mkData(name, result)
        else:
            pkt = result[0]
        self._sock.sendto(pkt, addr)
        self.processingInterest = False

    def onExit(self, signal, frame):
        self.interrupted = True
        self.processingInterest = False
        print "Shutting down..."
        sys.exit(0)

    def resolve(self, expression, resultHandle):
#        print "resolving ", expression
        if (expression == ''):
            resultHandle(None)
            return
        # Detect which format the type of the expression equals
        idxLeftParam = expression.find('(')
        idxRightParam = expression.rfind(')')
        idxSpace = expression.find(' ')
                  
        if ((idxLeftParam > -1) and ((idxSpace < 0) or idxLeftParam < idxSpace) and (idxRightParam == len(expression)-1)):
            #   Format 'myfunc(mydata)'
            functionName = expression[:idxLeftParam]
            interior = expression[idxLeftParam+1:-1]

            #   Extract arguments
            level = 0
            startPos = 0
            arguments = []

            for i in range(len(interior)):
                if (interior[i] == '('):
                    level += 1
                elif (interior[i] == ')'):
                    level -= 1
                elif (interior[i] == ','):
                    if (level == 0):
                        arguments.append(interior[startPos:i])
                        startPos = i+1
            if (len(interior) != 0):
                arguments.append(interior[startPos:])
            self.buildFunctionRetriever(functionName, arguments, resultHandle)

        elif (idxSpace >= 0):
            argList = expression.split(' ')
            if (len(argList) > 2):
                if ((argList[0] == 'call') and (argList[1].isdigit())):
                    #   Format 'call 2 ... ...'
                    functionName = argList[2]
                    numArg = (int) (argList[1])
                    if (numArg > 1):
                        argList = argList[3:]
                    else:
                        argList = []
                    self.buildFunctionRetriever(functionName, argList, resultHandle)

        else:
            # print "expressing interest in", expression
            self._access.getLabeledContent(expression, raw=True,
                                      callback=lambda data: resultHandle(data))
                                          

    def buildFunctionRetriever(self, functionName, arguments, resultHandle):
        idxDot = functionName.rfind('/')
        if (idxDot >= 0):
            moduleName = functionName[1:idxDot]
            moduleName.replace('/', '.')
            functionName = functionName[idxDot+1:]
            module = __import__(moduleName)
            callFunc = getattr(module, functionName)
        else:
            callFunc = globals()[functionName]

        funcRetriever = self.FunctionRetriever(len(arguments),
                         lambda allArgs: resultHandle(callFunc(*allArgs)))

        for i in range(len(arguments)):
            notify = self.Notifier(i, funcRetriever.notify)
            self.resolve(arguments[i], notify.onData)

# eof
