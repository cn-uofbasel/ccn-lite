FROM ubuntu:14.04
MAINTAINER Basil Kohler<basil.kohler@gmail.com>

ENV CCNL_HOME /ccn-lite
ENV CCNL_PATH /ccn-lite

RUN apt-get update && apt-get install -y libssl-dev git build-essential
RUN cd / && git clone https://github.com/cn-uofbasel/ccn-lite.git

RUN cd /ccn-lite && git fetch
RUN cd /ccn-lite && git checkout docker-integration
RUN cd /ccn-lite && make clean all
RUN sh $CCNL_HOME/test/scripts/demo-relay.sh ccnx2014 udp false

