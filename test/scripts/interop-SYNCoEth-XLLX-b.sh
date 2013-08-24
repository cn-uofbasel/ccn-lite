#!/bin/sh

# ccn-lite/test/scripts/interop-SYNoEth-XLLX-b.sh
# with this script, the CCN-lite relay runs in kernel

# after setting REMOTEMAC, start this script with sudo - and
# start also the *-b.sh script on the remote node


## This is host B in the following setup
##
## +-------------------------+     +--------------------------+
## | A                       |     |                        B |
## | |ccnr|--|ccnd|  |ccnl|  |     |   |ccnl|--|ccnd|--|ccnr| |
## |       9695|__udp__|9000 |     | 9000|__udp__|9695        |
## |                   |     |     |     |                    |
## +-------------------+-----+     +-----+--------------------+
##             LOCALMAC|_______eth_______|REMOTEMAC
##

. ./paths.sh

CCN_REPO=./sample_repo

ETHDEV=eth0
LOCALMAC=`ifconfig eth0 | grep HWaddr | cut -c 39-`
REMOTEMAC=ff:ff:ff:ff:ff:ff # better put the real MAC address of A for unicast
CCNL_UX=/tmp/ccnl-SYNCoEth.sock
ETHTYPE=$CCNL_ETH_TYPE

echo "$0: This host has MAC address $LOCALMAC"
echo

# cleaning up

echo -n "Shutting down old ccnr, ccnd and ccn-lite-relay instances ..."
killall ccnr
killall ccnd
killall ccn-lite-relay
rmmod ccn-lite-lnxkernel
echo

## ccnx: ccnd configuration
export CCND_DEBUG=6
export CCND_LISTEN_ON=0.0.0.0
export CCND_LOCAL_PORT=9695
$CCND_HOME/bin/ccndstart > /tmp/log.ccnd 2>&1

## ccnx: faces and FIB entries
$CCND_HOME/bin/ccndc add ccnx:/unibas.ch udp 127.0.0.1 9000

## ccnl: lite-relay init 
# $CCNL_HOME/ccn-lite-relay -v 99 -u 9000 -x $CCNL_UX 2> /tmp/log.ccnl &
insmod $CCNL_HOME/ccn-lite-lnxkernel.ko v=99 u=9000 x=$CCNL_UX c=0
sleep 2

## ccnl: eth device faces and FIB entries
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX newETHdev $ETHDEV $ETHTYPE ccnx2013 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep ACTION
FACEID1=`$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX newETHface any $REMOTEMAC $ETHTYPE | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
FACEID2=`$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX newUDPface any 127.0.0.1 9695 | $CCNL_HOME/util/ccn-lite-ccnb2xml | grep FACEID | sed -e 's/.*\([0-9][0-9]*\).*/\1/'`
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX setfrag $FACEID1 ccnx2013 1400
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX prefixreg /unibas.ch $FACEID1
$CCNL_HOME/util/ccn-lite-ctrl -x $CCNL_UX prefixreg /unibas.ch $FACEID2
sleep 2


## app: SYNC proto
##  step 1. config and start Repos Protocol and SYNC agent
export CCNR_DIRECTORY=$CCN_REPO
export CCNR_GLOBAL_PREFIX=ccnx:/unibas.ch/synctest
export CCNR_PROTO=unix
export CCNR_DEBUG=WARNING
export CCNS_DEBUG=INFO
export CCNS_ENABLE=1
export CCNS_REPO_STORE=1
export CCNS_SYNC_SCOPE=2
if [ ! -d $CCN_REPO ]; then
  mkdir $CCN_REPO
fi
$CCND_HOME/bin/ccnr &> log.ccnr &

##  step 2. create slice
$CCND_HOME/bin/ccnsyncslice -v create ccnx:/unibas.ch ccnx:/unibas.ch/synctest

## step 3 - add content and list directory
$CCND_HOME/bin/ccnputfile ccnx:/unibas.ch/synctest/`date | cut -c 12-19 | tr ':' '_'`.txt $0
sleep 3
echo
echo Content of repository:
$CCND_HOME/bin/ccnls ccnx:/unibas.ch/synctest/

echo
echo Add fresh content with
echo "  " $CCND_HOME/bin/ccnputfile ccnx:/unibas.ch/synctest/your_name.txt FILE
echo List files in the repo with
echo "  " $CCND_HOME/bin/ccnls ccnx:/unibas.ch/synctest/

# eof
