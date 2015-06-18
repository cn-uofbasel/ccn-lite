#!/usr/bin/python

# NFN proxy

import ccnlite.nfnserver as ccnl

ccnl.NFNserver(9002, "127.0.0.1", 9001)
