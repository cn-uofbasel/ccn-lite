# ccn-lite/src/py/pubfunc/myNamedFunctions.py

import cStringIO
import ccnlite.ndn2013 as ndn

'''
how to publish a named function under "/myNamedFunctions/"
[explain here]
'''

def invers(data):
    return data[::-1]

def printInput(data):
    print data

def concatenate(data1, data2):
    return "{" + data1 + "}, {" + data2 + "}"

def returnHelloWorld():
    return "Hello World"

def getName(data):
    name, content = ndn.parseData(cStringIO.StringIO(data[0]))
    return '/'.join(name)

def wordcount(data):
    name, content = ndn.parseData(cStringIO.StringIO(data[0]))
    return str(len(content.split(' ')))

# eof
