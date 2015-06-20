#
# OMNeT++/OMNEST Makefile for ccn-lite-omnet
#
# This file was generated with the command:
#  opp_makemake -f --deep -I../../inet/src/linklayer/ieee80211/radio -I../../inet/src/networklayer/routing/aodv -I../../inet/src/networklayer/common -I../../inet/src/networklayer/icmpv6 -I../../inet/src -I../../inet/src/world/obstacles -I../../inet/src/networklayer/xmipv6 -I../../inet/src/networklayer/contract -I../../inet/src/networklayer/autorouting/ipv4 -I../../inet/src/util -I../../inet/src/transport/contract -I../../inet/src/linklayer/common -I../../inet/src/status -I../../inet/src/linklayer/radio/propagation -I../../inet/src/linklayer/ieee80211/radio/errormodel -I../../inet/src/linklayer/radio -I../../inet/src/util/headerserializers/tcp -I../../inet/src/networklayer/ipv4 -I../../inet/src/mobility/contract -I../../inet/src/util/headerserializers/ipv4 -I../../inet/src/base -I../../inet/src/util/headerserializers -I../../inet/src/world/radio -I../../inet/src/linklayer/ieee80211/mac -I../../inet/src/networklayer/ipv6 -I../../inet/src/transport/sctp -I../../inet/src/util/headerserializers/udp -I../../inet/src/networklayer/ipv6tunneling -I../../inet/src/applications/pingapp -I../../inet/src/battery/models -I../../inet/src/util/headerserializers/ipv6 -I../../inet/src/util/headerserializers/sctp -I../../inet/src/linklayer/contract -I../../inet/src/transport/tcp_common -I../../inet/src/networklayer/arp -I../../inet/src/transport/udp -L../../inet/out/$$\(CONFIGNAME\)/src -linet -DINET_IMPORT -KINET_PROJ=../../inet
#

# Name of target to be created (-o option)
TARGET = ccn-lite-omnet$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I../../inet/src/linklayer/ieee80211/radio \
    -I../../inet/src/networklayer/routing/aodv \
    -I../../inet/src/networklayer/common \
    -I../../inet/src/networklayer/icmpv6 \
    -I../../inet/src \
    -I../../inet/src/world/obstacles \
    -I../../inet/src/networklayer/xmipv6 \
    -I../../inet/src/networklayer/contract \
    -I../../inet/src/networklayer/autorouting/ipv4 \
    -I../../inet/src/util \
    -I../../inet/src/transport/contract \
    -I../../inet/src/linklayer/common \
    -I../../inet/src/status \
    -I../../inet/src/linklayer/radio/propagation \
    -I../../inet/src/linklayer/ieee80211/radio/errormodel \
    -I../../inet/src/linklayer/radio \
    -I../../inet/src/util/headerserializers/tcp \
    -I../../inet/src/networklayer/ipv4 \
    -I../../inet/src/mobility/contract \
    -I../../inet/src/util/headerserializers/ipv4 \
    -I../../inet/src/base \
    -I../../inet/src/util/headerserializers \
    -I../../inet/src/world/radio \
    -I../../inet/src/linklayer/ieee80211/mac \
    -I../../inet/src/networklayer/ipv6 \
    -I../../inet/src/transport/sctp \
    -I../../inet/src/util/headerserializers/udp \
    -I../../inet/src/networklayer/ipv6tunneling \
    -I../../inet/src/applications/pingapp \
    -I../../inet/src/battery/models \
    -I../../inet/src/util/headerserializers/ipv6 \
    -I../../inet/src/util/headerserializers/sctp \
    -I../../inet/src/linklayer/contract \
    -I../../inet/src/transport/tcp_common \
    -I../../inet/src/networklayer/arp \
    -I../../inet/src/transport/udp \
    -I. \
    -Iccn-lite

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = -L../../inet/out/$(CONFIGNAME)/src  -linet
LIBS += -Wl,-rpath,`abspath ../../inet/out/$(CONFIGNAME)/src`

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/CcnInet.o \
    $O/Parser.o \
    $O/Ccn.o \
    $O/shared.o \
    $O/CcnCore.o \
    $O/CcnAdmin.o \
    $O/CcnAppMessage_m.o \
    $O/CcnPacket_m.o

# Message files
MSGFILES = \
    CcnAppMessage.msg \
    CcnPacket.msg

# Other makefile variables (-K)
INET_PROJ=../../inet

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" -loppmain$D $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ($(MAKECMDGOALS),depend)
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $O/$(TARGET)
	$(Q)$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	@echo Creating executable: $@
	$(Q)$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE)
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	$(qecho) Cleaning...
	$(Q)-rm -rf $O
	$(Q)-rm -f ccn-lite-omnet ccn-lite-omnet.exe libccn-lite-omnet.so libccn-lite-omnet.a libccn-lite-omnet.dll libccn-lite-omnet.dylib
	$(Q)-rm -f ./*_m.cc ./*_m.h
	$(Q)-rm -f ccn-lite/*_m.cc ccn-lite/*_m.h

cleanall: clean
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(qecho) Creating dependencies...
	$(Q)$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc ccn-lite/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/Ccn.o: Ccn.cc \
	Ccn.h \
	CcnAdmin.h \
	CcnAppMessage_m.h \
	CcnCore.h \
	CcnInet.h \
	CcnPacket_m.h \
	shared.h \
	$(INET_PROJ)/src/base/AbstractQueue.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h \
	$(INET_PROJ)/src/base/NotifierConsts.h \
	$(INET_PROJ)/src/base/QueueBase.h \
	$(INET_PROJ)/src/linklayer/contract/MACAddress.h \
	$(INET_PROJ)/src/networklayer/common/IInterfaceTable.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPvXAddress.h \
	$(INET_PROJ)/src/transport/contract/UDPSocket.h
$O/CcnAdmin.o: CcnAdmin.cc \
	Ccn.h \
	CcnAdmin.h \
	CcnCore.h \
	CcnInet.h \
	CcnPacket_m.h \
	Parser.h \
	shared.h \
	$(INET_PROJ)/src/base/AbstractQueue.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h \
	$(INET_PROJ)/src/base/NotifierConsts.h \
	$(INET_PROJ)/src/base/QueueBase.h \
	$(INET_PROJ)/src/linklayer/contract/MACAddress.h \
	$(INET_PROJ)/src/networklayer/common/IInterfaceTable.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPvXAddress.h \
	$(INET_PROJ)/src/transport/contract/UDPSocket.h
$O/CcnAppMessage_m.o: CcnAppMessage_m.cc \
	CcnAppMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h
$O/CcnCore.o: CcnCore.cc \
	Ccn.h \
	CcnAdmin.h \
	CcnAppMessage_m.h \
	CcnCore.h \
	CcnInet.h \
	CcnPacket_m.h \
	ccn-lite/ccnl-core.h \
	ccn-lite/ccnl-defs.h \
	ccn-lite/ccnl-ext.h \
	ccn-lite/ccnl-uapi.h \
	shared.h \
	$(INET_PROJ)/src/base/AbstractQueue.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h \
	$(INET_PROJ)/src/base/NotifierConsts.h \
	$(INET_PROJ)/src/base/QueueBase.h \
	$(INET_PROJ)/src/linklayer/contract/MACAddress.h \
	$(INET_PROJ)/src/networklayer/common/IInterfaceTable.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPvXAddress.h \
	$(INET_PROJ)/src/transport/contract/UDPSocket.h
$O/CcnInet.o: CcnInet.cc \
	CcnAppMessage_m.h \
	CcnInet.h \
	CcnPacket_m.h \
	shared.h \
	$(INET_PROJ)/src/base/AbstractQueue.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h \
	$(INET_PROJ)/src/base/ModuleAccess.h \
	$(INET_PROJ)/src/base/NotifierConsts.h \
	$(INET_PROJ)/src/base/QueueBase.h \
	$(INET_PROJ)/src/linklayer/contract/Ieee802Ctrl_m.h \
	$(INET_PROJ)/src/linklayer/contract/MACAddress.h \
	$(INET_PROJ)/src/networklayer/common/IInterfaceTable.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceEntry.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceTableAccess.h \
	$(INET_PROJ)/src/networklayer/common/InterfaceToken.h \
	$(INET_PROJ)/src/networklayer/contract/IPProtocolId_m.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4ControlInfo.h \
	$(INET_PROJ)/src/networklayer/contract/IPv4ControlInfo_m.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6Address.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6ControlInfo.h \
	$(INET_PROJ)/src/networklayer/contract/IPv6ControlInfo_m.h \
	$(INET_PROJ)/src/networklayer/contract/IPvXAddress.h \
	$(INET_PROJ)/src/transport/contract/UDPControlInfo.h \
	$(INET_PROJ)/src/transport/contract/UDPControlInfo_m.h \
	$(INET_PROJ)/src/transport/contract/UDPSocket.h
$O/CcnPacket_m.o: CcnPacket_m.cc \
	CcnPacket_m.h \
	$(INET_PROJ)/src/base/ByteArray.h \
	$(INET_PROJ)/src/base/ByteArrayMessage.h \
	$(INET_PROJ)/src/base/ByteArrayMessage_m.h \
	$(INET_PROJ)/src/base/ByteArray_m.h \
	$(INET_PROJ)/src/base/Compat.h \
	$(INET_PROJ)/src/base/INETDefs.h
$O/Parser.o: Parser.cc \
	Parser.h
$O/shared.o: shared.cc \
	shared.h

