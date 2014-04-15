#!/usr/bin/python

import socket
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 9002


f = open("/home/blacksheeep/ccn-lite/test/ccnb/nfn/computation_dummy_result.ccnb")
c = f.read()

sock = socket.socket(socket.AF_INET, # Internet
                  socket.SOCK_DGRAM) # UDP
sock.bind((UDP_IP, UDP_PORT))

#while True:
data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
print "received message:", data
time.sleep(2)
sock.sendto(c, ("127.0.0.1", 9001))

