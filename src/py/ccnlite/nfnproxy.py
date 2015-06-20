# ccn-lite/src/py/ccnlite/nfnproxy.py

'''
NFN proxy supporting the publishing of user defined Python functions

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
            (removed all dependencies from pyndn and from asyncio)
'''

import cStringIO
import importlib
# import logging
import socket
import sys
import time

# ccn-lite specific:
import client
import ndn2013 as ndn
import util

# logging.basicConfig()

class NFNproxy(object):

    class FunctionRetriever(object):

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

    '''
    __init__ starts a NFN proxy that will listen on the specified
    port and react to interests. Currently no parallel processing
    of interests is implemented by the proxy: it will block until
    the current computation is done (but it uses parallel resolution
    of arguments).

    Parameters:
    listenPort   - the port where the proxy is listening for interests
    gwIP, gwPort - the IPv4 UDP address of the gateway (ccn-nfn-relay)
    '''
    def __init__(self, listenPort, gwIP, gwPort):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(("127.0.0.1", listenPort))
        self._access = client.Access()
        self._access.connect(gwIP, gwPort)

        print "NFN proxy server listening on UDP port", listenPort, "."

        while True:
            pkt, addr = self._sock.recvfrom(8192)
            name = ndn.parseInterest(cStringIO.StringIO(pkt))
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

    def resolve(self, expression, resultHandle):
#        print "resolving ", expression
        if (expression == ''):
            resultHandle(None)
            return
        # Detect which format the type of the expression equals
        idxLeftParam = expression.find('(')
        idxRightParam = expression.rfind(')')
        idxSpace = expression.find(' ')
                  
        if ((idxLeftParam > -1) and ((idxSpace < 0) or idxLeftParam < idxSpace)
                                      and (idxRightParam == len(expression)-1)):
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
            moduleName = 'pubfunc.' + functionName[1:idxDot]
            moduleName.replace('/', '.')
            module = importlib.import_module(moduleName)
            functionName = functionName[idxDot+1:]
            callFunc = getattr(module, functionName)
        else:
            callFunc = globals()[functionName]

        funcRetriever = self.FunctionRetriever(len(arguments),
                         lambda allArgs: resultHandle(callFunc(*allArgs)))

        for i in range(len(arguments)):
            notify = self.Notifier(i, funcRetriever.notify)
            self.resolve(arguments[i], notify.onData)

# eof
