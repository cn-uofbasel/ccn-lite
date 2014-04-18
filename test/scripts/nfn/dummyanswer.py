#!/usr/bin/python

import socket
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 9002


f_t = open("/home/blacksheeep/ccn-lite/test/ccnb/nfn/computation_dummy_thunk.ccnb")
c_t = f_t.read()

f = open("/home/blacksheeep/ccn-lite/test/ccnb/nfn/computation_dummy_result.ccnb")
c = f.read()

sock = socket.socket(socket.AF_INET, # Internet
                  socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

while True:
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    print "received message:", data
    if "THUNK" in data:
        sock.sendto(c_t, ("127.0.0.1", 9001))
    else:
        time.sleep(1)
        sock.sendto(c, ("127.0.0.1", 9001))

