FROM ubuntu:14.04
MAINTAINER Basil Kohler<basil.kohler@gmail.com>

RUN apt-get update && apt-get install -y libssl-dev git build-essential

RUN ls -la /tmp
RUN cd /ccn-lite && make clean all
RUN sh $CCNL_HOME/test/scripts/demo-relay.sh ccnx2014 udp false

