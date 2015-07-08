#!/usr/bin/python
# dummyanswer_cistlv.py

import socket
import time
import os

UDP_IP = "127.0.0.1"
UDP_PORT = 9002

print "dummyanswer server started"

env = os.getenv("CCNL_HOME") 
#f_t = open(os.path.join(env, "test/ndntlv/nfn/computation_dummy_thunk.ndntlv"))
#c_t = f_t.read()

f = open(os.path.join(env, "test/cistlv/nfn/computation_dummy_result.cistlv"))
c = f.read()

sock = socket.socket(socket.AF_INET, # Internet
                  socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

while True:
    print "dummyanswer server now blocking on recvfrom"
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
#    print "received message:", data
 #   if "THUNK" in data:
  #      sock.sendto(c_t, ("127.0.0.1", 9001))
  #  else:
#        time.sleep(1)
    sock.sendto(c, ("127.0.0.1", 9001))

