FROM ubuntu:14.04
MAINTAINER Basil Kohler<basil.kohler@gmail.com>

ENV CCNL_HOME /var/ccn-lite
ENV CCNL_PORT 9000
ENV USE_NFN 1

RUN apt-get update && apt-get install -y libssl-dev build-essential

ADD . /var/ccn-lite
WORKDIR /var/ccn-lite
RUN cd src && make clean all

EXPOSE 9000/udp

# CMD ["/var/ccn-lite/bin/ccn-nfn-relay", "-s", "ndn2013", "-d", "test/ndntlv" "-v", "99", "-u", "$CCNL_PORT", "-x", "/tmp/ccn-lite-mgmt.sock"]
 CMD /var/ccn-lite/bin/ccn-nfn-relay -s ndn2013 -d test/ndntlv -v trace -u $CCNL_PORT -x /tmp/ccn-lite-mgmt.sock
