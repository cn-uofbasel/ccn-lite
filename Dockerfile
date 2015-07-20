FROM ubuntu:14.04
MAINTAINER Basil Kohler<basil.kohler@gmail.com>

ENV DEBIAN_FRONTEND noninteractive
ENV CCNL_HOME /var/ccn-lite
ENV PATH "$PATH:$CCNL_HOME/bin"
ENV CCNL_PORT 9000
ENV USE_NFN 1

RUN apt-get -y update && apt-get install -y libssl-dev build-essential wget openjdk-7-jre

ADD . /var/ccn-lite
WORKDIR /var/ccn-lite
RUN cd src && make clean all

EXPOSE 9000/udp

# CMD ["/var/ccn-lite/bin/ccn-nfn-relay", "-s", "ndn2013", "-d", "test/ndntlv" "-v", "info", "-u", "$CCNL_PORT", "-x", "/tmp/ccn-lite-mgmt.sock"]
CMD /var/ccn-lite/bin/ccn-nfn-relay -s ndn2013 -d test/ndntlv -v info -u $CCNL_PORT -x /tmp/ccn-lite-mgmt.sock
