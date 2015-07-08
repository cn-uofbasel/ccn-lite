/*
 * CcnInet.cc
 *
 * Definitions for abstraction layer from INET framework (BaseQueue and all other INET functionality)
 *
 *  Created on: Sep 18, 2012
 *      Author: manolis sifalakis
 */


#include <cstring>

#include "IPv4ControlInfo.h"
#include "IPv6ControlInfo.h"
#include "Ieee802Ctrl_m.h"
#include "InterfaceTableAccess.h"
#include "UDPControlInfo.h"


#include "CcnInet.h"
#include "CcnPacket_m.h"
#include "CcnAppMessage_m.h"

#include "shared.h"



Define_Module(CcnInet);






/*****************************************************************************
 * Class CcnContext
 *****************************************************************************/


/*****************************************************************************
 *
 */
CcnContext::CcnContext ( unsigned short af ) {
    memset (&src, 0, sizeof(CcnContext::EndPoint) );
    memset (&dst, 0, sizeof(CcnContext::EndPoint) );
    src.sa.sa_family = dst.sa.sa_family = af;
    this->debugLevel = ::defaultDebugLevel;
};


/*****************************************************************************
 *
 */
bool
CcnContext::isAddressFamily( unsigned short af) {
    return ( (src.sa.sa_family == dst.sa.sa_family ) && (af == src.sa.sa_family) );
};


/*****************************************************************************
 *
 */
MACAddress
CcnContext::getSrcAddress802() {
    if (src.sa.sa_family != AF_PACKET) {
        DBGPRT(AUX, Err, "CcnContext::getSrcAddress802()")
                << " CcnContext src.sa.sa_family != AF_PACKET " << std::endl;
        opp_error("Error: failed attempt to access src address type in CcnContext as AF_PACKET type!");
    }

    MACAddress mac;
    mac.setAddressBytes(src.mac.sll_addr);
    return mac;
};


/*****************************************************************************
 *
 */
MACAddress
CcnContext::getDstAddress802() {
    if (dst.sa.sa_family != AF_PACKET) {
        DBGPRT(AUX, Err, "CcnContext::getDstAddress802()")
                << " CcnContext dst.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Error: failed attempt to access dst address type in CcnContext as AF_PACKET type!");
    }

    MACAddress macAddr;
    macAddr.setAddressBytes (dst.mac.sll_addr);
    return macAddr;
};


/*****************************************************************************
 *
 */
IPvXAddress
CcnContext::getSrcAddressInet() {

    IPvXAddress inAddr;

    if (src.sa.sa_family == AF_INET)
    {
        /* sockaddr_in always in network byte order, so we need to care about endianess here
         */
        inAddr.set( IPv4Address( ntohl(src.in4.sin_addr.s_addr) ) );
    }
    else if (src.sa.sa_family == AF_INET6)
    {
        /* Convert groups of 4 chars (in network byte order) to 1 long.
         * They get stored automatically in host byte order.
         */
        unsigned long int seg0 =
                (src.in6.sin6_addr.s6_addr[0] << 24) |
                (src.in6.sin6_addr.s6_addr[1] << 16) |
                (src.in6.sin6_addr.s6_addr[2] << 8) |
                src.in6.sin6_addr.s6_addr[3];
        unsigned long int seg1 =
                (src.in6.sin6_addr.s6_addr[4] << 24) |
                (src.in6.sin6_addr.s6_addr[5] << 16) |
                (src.in6.sin6_addr.s6_addr[6] << 8) |
                src.in6.sin6_addr.s6_addr[7];
        unsigned long int seg2 =
                (src.in6.sin6_addr.s6_addr[8] << 24) |
                (src.in6.sin6_addr.s6_addr[9] << 16) |
                (src.in6.sin6_addr.s6_addr[10] << 8) |
                src.in6.sin6_addr.s6_addr[11];
        unsigned long int seg3 =
                (src.in6.sin6_addr.s6_addr[12] << 24) |
                (src.in6.sin6_addr.s6_addr[13] << 16) |
                (src.in6.sin6_addr.s6_addr[14] << 8) |
                src.in6.sin6_addr.s6_addr[15];

        /* create temp IPv6Address and push it to IPvXAddress
         */
        inAddr.set ( IPv6Address(seg0, seg1, seg2, seg3) );
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::getSrcAddressInet()")
                << " CcnContext src.sa.sa_family != AF_INET|AF_INET6 " << std::endl;
        opp_error("Error: failed attempt to access src address type in CcnContext as AF_INET(6) type!");
    }

    return inAddr;
};


/*****************************************************************************
 *
 */
IPvXAddress
CcnContext::getDstAddressInet() {
    IPvXAddress inAddr;

    if (dst.sa.sa_family == AF_INET)
    {
        /* sockaddr_in always in network byte order, so we need to care about endianess here
         */
        inAddr.set( IPv4Address( ntohl(dst.in4.sin_addr.s_addr) ) );
    }
    else if (dst.sa.sa_family == AF_INET6)
    {
        /* Convert groups of 4 chars (in network byte order) to 1 long.
         * They get stored automatically in host byte order.
         */
        unsigned long seg0 =
                (dst.in6.sin6_addr.s6_addr[0] << 24) |
                (dst.in6.sin6_addr.s6_addr[1] << 16) |
                (dst.in6.sin6_addr.s6_addr[2] << 8) |
                dst.in6.sin6_addr.s6_addr[3];
        unsigned long seg1 =
                (dst.in6.sin6_addr.s6_addr[4] << 24) |
                (dst.in6.sin6_addr.s6_addr[5] << 16) |
                (dst.in6.sin6_addr.s6_addr[6] << 8) |
                dst.in6.sin6_addr.s6_addr[7];
        unsigned long seg2 =
                (dst.in6.sin6_addr.s6_addr[8] << 24) |
                (dst.in6.sin6_addr.s6_addr[9] << 16) |
                (dst.in6.sin6_addr.s6_addr[10] << 8) |
                dst.in6.sin6_addr.s6_addr[11];
        unsigned long seg3 =
                (dst.in6.sin6_addr.s6_addr[12] << 24) |
                (dst.in6.sin6_addr.s6_addr[13] << 16) |
                (dst.in6.sin6_addr.s6_addr[14] << 8) |
                dst.in6.sin6_addr.s6_addr[15];

        /* create temp IPv6Address and push it to IPvXAddress
         */
        inAddr.set ( IPv6Address(seg0, seg1, seg2, seg3) );
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::getDstAddressInet()")
                << " CcnContext dst.sa.sa_family != AF_INET|AF_INET6 " << std::endl;
        opp_error("Error: failed attempt to access dst address type in CcnContext as AF_INET(6) type!");
    }

    return inAddr;
};


/*****************************************************************************
 *
 */
int
CcnContext::getSrcPort() {
    if (src.sa.sa_family == AF_INET)
    {
        //return src.in4.sin_port;
        return ntohs(src.in4.sin_port);
    }
    else if (src.sa.sa_family == AF_INET6)
    {
        //return src.in6.sin6_port;
        return ntohs(src.in4.sin_port);
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::getSrcPort()")
                << " CcnContext src.sa.sa_family != AF_INET|AF_INET6 " << std::endl;
        opp_error("Error: failed attempt to access src address type in CcnContext as AF_INET(6) socket type!");
    }

    return -1;
};


/*****************************************************************************
 *
 */
int
CcnContext::getDstPort() {
    if (dst.sa.sa_family == AF_INET)
    {
        return ntohs(dst.in4.sin_port);
    }
    else if (dst.sa.sa_family == AF_INET6)
    {
        return ntohs(dst.in6.sin6_port);
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::getDstPort()")
                << " CcnContext dst.sa.sa_family != AF_INET|AF_INET6 " << std::endl;
        opp_error("Error: failed attempt to access dst address type in CcnContext as AF_INET(6) socket type!");
    }

    return -1;
};


/*****************************************************************************
 *
 */
void
CcnContext::set802Info(const MACAddress &srcMac, const MACAddress &dstMac)
{
    if ( (src.sa.sa_family != AF_PACKET) || (dst.sa.sa_family != AF_PACKET) )
    {
        DBGPRT(AUX, Err, "CcnContext::set802Info()")
                << " Pre-initialised CcnContext for address family != AF_PACKET " << std::endl;
        opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_PACKET type!");
    }

    srcMac.getAddressBytes(src.mac.sll_addr);
    src.mac.sll_halen = 6;
    src.mac.sll_protocol = ETH_P_ALL;
    dstMac.getAddressBytes(dst.mac.sll_addr);
    dst.mac.sll_halen = 6;
    dst.mac.sll_protocol = ETH_P_ALL;

    return;
};


/*****************************************************************************
 *
 */
void
CcnContext::set802Info(const struct sockaddr_ll &srcSockLL, const struct sockaddr_ll &dstSockLL)
{
    if ( (src.sa.sa_family != AF_PACKET) || (dst.sa.sa_family != AF_PACKET) )
    {
        DBGPRT(AUX, Err, "CcnContext::set802Info()")
                << " Pre-initialised CcnContext for address family != AF_PACKET " << std::endl;
        opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_PACKET type!");
    }

    if ( (srcSockLL.sll_family != AF_PACKET) || (dstSockLL.sll_family != AF_PACKET) )
    {
        DBGPRT(AUX, Err, "CcnContext::set802Info()")
                << " The provided CcnContext address information is invalid ( != AF_PACKET type)" << std::endl;
        opp_error("Error: trying to set CcnContext with invalid address information (!= AF_PACKET type) !");
    }

    memset (&(src), 0 , sizeof(CcnContext::EndPoint) );
    memset (&(dst), 0 , sizeof(CcnContext::EndPoint) );

    memcpy (&(src.mac), &srcSockLL, sizeof(struct sockaddr_ll));
    memcpy (&(dst.mac), &dstSockLL, sizeof(struct sockaddr_ll));

    return;
};


/*****************************************************************************
 *
 */
void
CcnContext::set802Info(const char  *srcMac, const char *dstMac)
{
    if ( (src.sa.sa_family != AF_PACKET) || (dst.sa.sa_family != AF_PACKET) )
    {
        DBGPRT(AUX, Err, "CcnContext::set802Info()")
                << " Pre-initialised CcnContext for address family != AF_PACKET " << std::endl;
        opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_PACKET type!");
    }

    // FIXME: missing sanity check for the params correctness

    memset(src.mac.sll_addr, 0, 8*sizeof(char));
    memset(dst.mac.sll_addr, 0, 8*sizeof(char));

    memcpy (src.mac.sll_addr, srcMac, 6);
    memcpy (dst.mac.sll_addr, dstMac, 6);

    return;
};


/*****************************************************************************
 *
 */
void
CcnContext::setInetInfo(const IPvXAddress &srcIp, const IPvXAddress &dstIp, int srcPort, int dstPort)
{
    uint32_t *words;

    if (
            !( (src.sa.sa_family == AF_INET) && (dst.sa.sa_family == AF_INET) ) ||
            !( (src.sa.sa_family == AF_INET6) && (dst.sa.sa_family == AF_INET6) )
        )
    {
        DBGPRT(AUX, Err, "CcnContext::setInetInfo()")
                << " Pre-initialised CcnContext for address family != AF_INET(6) " << std::endl;
        opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_INET(6) type!");
    }


    if ( !srcIp.isIPv6() && !dstIp.isIPv6() )
    {
        src.in4.sin_addr.s_addr = htonl( srcIp.get4().getInt() );
        dst.in4.sin_addr.s_addr = htonl( dstIp.get4().getInt() );
        src.in4.sin_port = htons( srcPort );
        dst.in4.sin_port = htons( dstPort );
    }
    else if ( srcIp.isIPv6() && dstIp.isIPv6() )
    {
        words = srcIp.get6().words();

        // network byte order is respected
        *((uint32_t *) (src.in6.sin6_addr.s6_addr)) = htonl (words[0]);
        *((uint32_t *) (src.in6.sin6_addr.s6_addr + 8)) = htonl (words[1]);
        *((uint32_t *) (src.in6.sin6_addr.s6_addr + 16)) = htonl (words[2]);
        *((uint32_t *) (src.in6.sin6_addr.s6_addr + 24)) = htonl (words[3]);

        src.in6.sin6_port = htons( srcPort );

        words = dstIp.get6().words();

        // network byte order is respected
        *((uint32_t *) (dst.in6.sin6_addr.s6_addr)) = htonl (words[0]);
        *((uint32_t *) (dst.in6.sin6_addr.s6_addr + 8)) = htonl (words[1]);
        *((uint32_t *) (dst.in6.sin6_addr.s6_addr + 16)) = htonl (words[2]);
        *((uint32_t *) (dst.in6.sin6_addr.s6_addr + 24)) = htonl (words[3]);

        dst.in6.sin6_port = htons( dstPort );
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::setInetInfo()")
                << " The provided CcnContext address information is invalid (failed to identify address family as AF_INET|AF_INET6)" << std::endl;
        opp_error("Error: trying to set CcnContext with invalid address information (failed to identify address family)!");
    }

    return;
};


/*****************************************************************************
 *
 */
void
CcnContext::setInetInfo(const struct sockaddr &srcSockInet, const struct sockaddr &dstSockInet)
{
    if ( (srcSockInet.sa_family == AF_INET) && (dstSockInet.sa_family == AF_INET) )
    {
        if ( (src.sa.sa_family != AF_INET) || (dst.sa.sa_family != AF_INET) )
        {
            DBGPRT(AUX, Err, "CcnContext::setInetInfo()")
                    << " Pre-initialised CcnContext for address family != AF_INET " << std::endl;
            opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_INET type!");
        }

        memset (&(src.in4) , 0, sizeof (sockaddr_in));
        memset (&(dst.in4) , 0, sizeof (sockaddr_in));

        memcpy (&(src.in4), &srcSockInet, sizeof(sockaddr_in) );
        memcpy (&(dst.in4), &dstSockInet, sizeof(sockaddr_in) );
    }
    else if ( (srcSockInet.sa_family == AF_INET6) && (dstSockInet.sa_family == AF_INET6) )
    {
        if ( (src.sa.sa_family != AF_INET6) || (dst.sa.sa_family != AF_INET6) )
        {
            DBGPRT(AUX, Err, "CcnContext::setInetInfo()")
                    << " Pre-initialised CcnContext for address family != AF_INET6 " << std::endl;
            opp_error("Error: failed attempt to set address information in CcnContext previously initialised as AF_INET6 type!");
        }

        memset (&(src.in6) , 0, sizeof (sockaddr_in6));
        memset (&(dst.in6) , 0, sizeof (sockaddr_in6));

        memcpy (&(src.in6), &srcSockInet, sizeof(sockaddr_in6) );
        memcpy (&(dst.in6), &dstSockInet, sizeof(sockaddr_in6) );
    }
    else
    {
        DBGPRT(AUX, Err, "CcnContext::setInetInfo()")
                << " The provided CcnContext address information is invalid (failed to identify address family as AF_INET|AF_INET6)" << std::endl;
        opp_error("Error: trying to set CcnContext with invalid address information (failed to identify address family)!");
    }

    return;
};







/*****************************************************************************
 * Class CcnInet
 *
 * TODO: UDP tunnel support is provided but is not being used and has not been
 * tested
 *****************************************************************************/

/*****************************************************************************
 * Module initialisation
 */
void
CcnInet::initialize()
{
    bool iSetGlobalDbgLvl = false;


    /* QueueBase class initialisation
     */
    QueueBase::initialize();


    /* Immediately set my debug level (and the global default if not set)
     * so that I can use the DBGPRT() macro
     */
    cModule *netTopology = getParentModule()->getParentModule();

    if (::defaultDebugLevel == 0) {
        if (netTopology->par("defaultDebugLevel").isSet())
            ::defaultDebugLevel = netTopology->par("defaultDebugLevel");
        else
            ::defaultDebugLevel = CCNL_DEFAULT_DBG_LVL;
        iSetGlobalDbgLvl = true;
    }

    (netTopology->par("auxDebug").isSet() && (netTopology->par("auxDebug").boolValue() == true)) ? DebugOutput::Init(std::cerr) : DebugOutput::Init() ;
    debugLevel = (par("debugLevel").isSet()) ? par("debugLevel") : ::defaultDebugLevel;

    if ( iSetGlobalDbgLvl ) {
        DBGPRT(EVAUX, Info, this->getFullPath())
        << "Global default debug level is set to: " << ::defaultDebugLevel
        << std::endl;
    }

    DBGPRT(EVAUX, Info, this->getFullPath())
        << " Set my debug level to " << debugLevel
        << std::endl;


    /* Set up access to NICs (Interface Table of INET framework)
     */
    nicTbl = InterfaceTableAccess().get();


    /* Create UDP-tunnels for socket Faces (not used or tested atm)
     */
    numUdpFaces = 0;
    basePortUdpFaces = 0;
    udpSockTbl = NULL;

    if ( 0 /*TODO: check that we have UDP/IP layer below */ )
    {
        numUdpFaces = par("numUdpFaces");
        basePortUdpFaces = par("basePortUdpFaces");

        udpSockTbl = new struct CcnInet::UdpSocketInfo [numUdpFaces];
        for (int i = 0 ; i< numUdpFaces ; i++)
        {
            udpSockTbl[i].socket.setOutputGate(gate("udpOut"));
            udpSockTbl[i].socket.bind(basePortUdpFaces + i);
        }

        DBGPRT(EVAUX, Info, this->getFullPath()) << " Initialised UDP tunnels for socket Faces" << std::endl;
    }


    /* stat counters reset
     */
    inPktsDropped = 0;
    outPktsDropped = 0;
    inPkts = 0;
    outPkts = 0;

    return;
};



/*****************************************************************************
 * On termination of simulation handling
 */
void
CcnInet::finish() {

    for (int i = 0 ; i< numUdpFaces ; i++)
    {
        udpSockTbl[i].socket.close();
    }

    QueueBase::finish();
};



/*****************************************************************************
 * D'tor
 */
CcnInet::~CcnInet() {
    delete udpSockTbl;
};



/*****************************************************************************
 * Main dispatcher for all incoming messages. Overide default from
 * AbstracQueue class so that self messages bypass the queueuing discipline
 */
void
CcnInet::handleMessage(cMessage *msg)
{
    /**
     * Normally the INET interface module does not use timers,
     * (not considered by AbstractQueue) but for this test we enable them
     */
    if ( msg->isSelfMessage() && (strcmp (msg->getName(), "end-service") !=0) )
    {
        handleSelfMessage(msg);
    }
    else
        AbstractQueue::handleMessage(msg);
}



/*****************************************************************************
 * Given a local MAC address find the InterfaceEntry object that has been
 * configured with it
 */
InterfaceEntry *
CcnInet::netIfFromMac (const MACAddress &macAddr)
{
    int             numNics = nicTbl->getNumInterfaces();
    InterfaceEntry  *nic = 0;

    if ( (numNics < 1) || (gateSize("ifOut")==0) ) {
        DBGPRT(AUX, Err, this->getFullPath()) << " No Gates to the Link Layer" << std::endl;
        return 0;
    }

    for (int i=0 ; i < numNics ; i++) {
        nic = nicTbl->getInterface(i);
        if (nic->getMacAddress() == macAddr)
            return nic;
    }

    DBGPRT(AUX, Err, this->getFullPath())
            << " Could not resolve MAC Address " << macAddr.str()
            << " to an existing NIC (InterfaceEntry *)"
            << std::endl;

    return 0;
}


/*****************************************************************************
 * Given a local MAC address, find the index of the interface it belongs to
 */
int
CcnInet::netIfIndexFromMac(const MACAddress &macAddr)
{
    InterfaceEntry  *nic = 0;

    if ( ! (nic = netIfFromMac(macAddr)) )
        return -1;

    return nic->getNetworkLayerGateIndex();
}



/*****************************************************************************
 * Given a local MAC address, find the right gate ID for sending out
 */
int
CcnInet::gateIdFromMac(const MACAddress &macAddr)
{
    int             gateId = -1;
    InterfaceEntry  *nic = 0;

    if ( ! (nic = netIfFromMac(macAddr)) )
        return -1;

    gateId = gate("ifOut", 0)->getId() + nic->getNetworkLayerGateIndex();

    DBGPRT(AUX, Info, this->getFullPath())
            << "The gate for MAC addr "<< macAddr.str() << " is " << gateId
            << std::endl;

    return gateId;
}



/*****************************************************************************
 * Given an IP addr and port no find the corresponding opened UDP socket
 */
UDPSocket *
CcnInet::whichUdpSocket (const IPvXAddress &address, const int port)
{
    for (int i =0 ; i < numUdpFaces ; i++) {
        if ( (udpSockTbl[i].addr == address) && (udpSockTbl[i].port == port) )
            return &(udpSockTbl[i].socket);
    }

    DBGPRT(AUX, Warn, this->getFullPath())
        << "Could not resolve IPvX Address to an open UDP socket"
        << std::endl;

    return NULL;
}



/*****************************************************************************
 * Update popup information presented at tkenv
 */
void
CcnInet::updateDisplayString()
{
    char buf[100] = "";
    if (inPktsDropped > 0) sprintf( buf+strlen(buf), "(In)DROPPED:%d ", inPktsDropped);
    if (outPktsDropped > 0) sprintf(buf+strlen(buf), "(Out)DROPPED:%d ", outPktsDropped);
    if (inPkts > 0) sprintf(buf+strlen(buf), "RECEIVED:%d ", inPkts);
    if (outPkts >0) sprintf(buf+strlen(buf), "SENT:%d ", outPkts);
    getDisplayString().setTagArg("t", 0, buf);
}




/*****************************************************************************
 * Dispatch downward going packets to the right interface
 */
void
CcnInet::toLowerLayer(cMessage *msg)
{
    CcnContext  *ccnCtx = NULL;
    Ieee802Ctrl *macCtx = NULL;
    CcnPacket   *ccnPkt = NULL;
    int         nicGateId = 0;
    bool        msgDrop = false;


    /**
     * Check that we have a valid CCN packet and extract CCN context info
     */
    if ( ! ((msg->getKind() == PROT_CCN) && (ccnPkt = dynamic_cast<CcnPacket *>(msg))) ) {

        DBGPRT(EVAUX, Err, this->getFullPath())
                << "Message for layer below not identified as a CcnPacket message. Drop silently"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "\t dynamic cast on CcnPacket failed or getKind() not set to PROT_CCN"
                << std::endl;

        outPktsDropped++;
        msgDrop = true;
        goto CLEANUP_AND_EXIT;
    }

    if ( ! (ccnCtx = (CcnContext *) ccnPkt->getContextPointer() ) )   {

        DBGPRT(EVAUX, Err, this->getFullPath())
                << "CcnPacket message without CcnContext. Drop silently"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "\t getContextPointer() on CcnPacket returned 0"
                << std::endl;

        outPktsDropped++;
        msgDrop = true;
        goto CLEANUP_AND_EXIT;
    };


    /**
     * Check the type of destination address and decide how to send it
     */
    if ( ccnCtx->isAddressFamily(AF_INET) || ccnCtx->isAddressFamily(AF_INET6)) {

        /**
         * Use UDP sockets (TODO: not used or tested)
         */
        UDPSocket *s = whichUdpSocket( ccnCtx->getSrcAddressInet(), ccnCtx->getSrcPort() );

        if (s) {
            s->sendTo (ccnPkt, ccnCtx->getDstAddressInet(), ccnCtx->getDstPort() );

            DBGPRT(EVAUX, Info, this->getFullPath()) << "Sending CcnPacket over UDP socket" << std::endl;

            outPkts++;
            goto CLEANUP_AND_EXIT;

        } else {

            DBGPRT(EVAUX, Err, this->getFullPath())
                    << "CcnPacket could not be sent via UDP. Drop silently"
                    << std::endl;
            DBGPRT(AUX, Detail, this->getFullPath())
                    << "\t whichUdpSocket() failed to find an open socket to "
                    << ccnCtx->getSrcAddressInet() << ":" << ccnCtx->getSrcPort()
                    << std::endl;

            outPktsDropped++;
            msgDrop = true;
            goto CLEANUP_AND_EXIT;
        }

    } else if (ccnCtx->isAddressFamily(AF_PACKET) ) {   // prepare for ieee 802 encap

        /**
         * Use the LL NIC interfaces
         */

        /* Prepare MAC control info, update type field and the msg kind
         */
        macCtx = new Ieee802Ctrl();
        macCtx->setDest( ccnCtx->getDstAddress802() );
        //macCtx->setEtherType( ETHERTYPE_CCN );  //TODO: Add this to the known ethertypes?
        macCtx->setEtherType( ETHERTYPE_IPv4 );  //For now mask ourselves as IPv4 traffic
        ccnPkt->setControlInfo( macCtx );
        ccnPkt->setKind(IEEE802CTRL_DATA);


        /* If source mac is set to broadcast, send to all interfaces
         * and bother no more
         */
        if ( ccnCtx->getSrcAddress802().isBroadcast() ) {

            int outGateBaseId = gateBaseId("ifOut");
            int outGateSize = gateSize("ifOut");

            int numNICs = nicTbl->getNumInterfaces();

            if ( (numNICs < 1) || (outGateSize==0) ) {
                DBGPRT(AUX, Err, this->getFullPath()) << "No Gates to the Link Layer" << std::endl;

                outPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;
            }

            if (numNICs != outGateSize) {
                DBGPRT(AUX, Warn, this->getFullPath())
                        << "Number of NICs = " << numNICs
                        << ", size of eth[] vector = "<< outGateSize
                        <<". This might create issues!"
                        << std::endl;
            }

            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "Broadcasting CcnPacket for dest MAC addr " << ccnCtx->getDstAddress802().str()
                    << " from all local gates (NICs)"
                    << std::endl;

            /* broadcast the msg
             */
            for (int i=0; i<outGateSize; i++) {

                if (i == outGateSize-1)
                    send (msg, outGateBaseId + i);
                else {
                    cMessage *m = msg->dup();
                    m->setControlInfo(macCtx->dup());
                    send (m, outGateBaseId + i);
                }

                outPkts++;
            }

            goto CLEANUP_AND_EXIT;
        }


        /* If we have locally configured the source mac address, find the right gate to send to
         */
        nicGateId = gateIdFromMac( ccnCtx->getSrcAddress802() );
        if (nicGateId != -1)
        {
            /**
             * Found NIC to use
             */
            send(ccnPkt, nicGateId);

            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "Sending CcnPacket via Link Layer (NIC gate ID " << nicGateId
                    << "), using src (local) MAC addr " << ccnCtx->getSrcAddress802().str()
                    << " to dest MAC addr " << ccnCtx->getDstAddress802().str()
                    << std::endl;
            outPkts++;
            goto CLEANUP_AND_EXIT;

        } else {

            /**
             * The src MAC does not match one of my NICs
             */
            DBGPRT(EVAUX, Err, this->getFullPath())
                    << "CcnPacket not sent via MAC Layer (src MAC addr to NIC resolution failed). Drop silently"
                    << std::endl;

            outPktsDropped++;
            msgDrop = true;
            goto CLEANUP_AND_EXIT;
        }

    } else {

        /**
         * Missing the right address information to determine how to send
         */
        DBGPRT(EVAUX, Err, this->getFullPath())
                << "Invalid CcnContext. Drop silently"
                << std::endl;
        DBGPRT(AUX, Detail, this->getFullPath())
                << "CcnContext not configured with AF_INET or AF_PACKET address family."
                << " Cannot decide how to forward the packet"
                << std::endl;

        outPktsDropped++;
        goto CLEANUP_AND_EXIT;
    }


CLEANUP_AND_EXIT:
    if (ccnCtx) delete ccnCtx;
    if (msgDrop) delete msg;
    if (ev.isGUI()) updateDisplayString();

    return;
};



/*****************************************************************************
 * Dispatch upward going packets to the right interface
 */
void
CcnInet::toUpperLayer(cMessage *msg)
{
    if ( ! dynamic_cast<CcnAppMessage *>(msg) ) {
        delete msg;
        DBGPRT(EVAUX, Err, this->getFullPath())
                << "Not a CcnAppMessage sent to layer above. Caught and sent to /dev/null."
                << std::endl;
    }

    if (hasGate("upperOut") && gate("upperOut")->isConnectedOutside() )
    {
        send(msg, "upperOut");
        DBGPRT(EVAUX, Info, this->getFullPath())
                << "Passing CcnAppMessage to layer above."
                << std::endl;
    }
    else {
        delete msg;
        DBGPRT(EVAUX, Warn, this->getFullPath())
                << "Sending CcnAppMessage to /dev/null (no connected layer above)."
                << std::endl;
    }

    return;
}



/*****************************************************************************
 * Dispatch packets from the Queue to the right service method
 */
void
CcnInet::endService(cPacket *msg)
{
    CcnContext          *ccnCtx = NULL;
    bool                msgDrop = false;
    UDPErrorIndication  *errInfo = NULL;
    UDPDataIndication   *udpCtx = NULL;


    /**
     * Where did packet come from ?
     */
    if (msg->getArrivalGate()->isName("upperIn"))
    {
        /** from layer above, pass to respective service method */

        if ( dynamic_cast<CcnAppMessage *>(msg) )
            fromUpperLayer ( msg );
        else {
            DBGPRT(EVAUX, Warn, this->getFullPath())
                    << "Received from upper layer message, which is not a CcnAppMessage. Drop silently"
                    << std::endl;
            msgDrop = true;
        }

    }
    else if (dynamic_cast<CcnPacket *>(msg))
    {
        /**
         * CcnPacket from the layer below
         *
         * We don't care whether the packet is one chunk or one fragment
         * of a chunk, this should be handled by a derived class that
         * implements fromLowerLayer()
         */

        /**
         * Check if it came from a UDP socket or from the wire directly
         */

        if (msg->arrivedOn("ifIn") ) {

            /* MTU sanity check
             */
            cGate *inGate = msg->getArrivalGate();
            InterfaceEntry *intf = inGate ? nicTbl->getInterfaceByNetworkLayerGateIndex(inGate->getIndex()) : NULL;

            if (intf->getMTU() < msg->getByteLength() )  {

                DBGPRT(EVAUX, Err, this->getFullPath())
                        << "Received a too big CcnPacket from layer below. Drop silently" << std::endl;
                DBGPRT(AUX, Detail, this->getFullPath())
                        << this->getFullPath()
                        << "\t CcnPacket arrived at a NIC with MTU smaller that the packet size"
                        << std::endl;

                inPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;
            }

            /* Create CcnContext and attach it to msg
             */
            Ieee802Ctrl *macCtx = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());

            if ( ! (ccnCtx = new CcnContext(AF_PACKET)) ) {

                DBGPRT(AUX, Err, this->getFullPath())
                        << "Memory allocation for CcnContext failed. Aborting incoming pkt processing"
                        << std::endl;

                inPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;
            };

            ccnCtx->set802Info(macCtx->getSrc(), macCtx->getDest());
            msg->setContextPointer((void *) ccnCtx);
            msg->setKind(PROT_CCN);

            /* Deliver to service method
             */
            DBGPRT(EVAUX, Info, this->getFullPath())
                    << "Received CcnPacket packet over Ethernet from "
                    <<  macCtx->getSrc() << " at " << macCtx->getDest()
                    << std::endl;

            inPkts++;
            fromLowerLayer ( msg );

        }
        else if ( msg->arrivedOn("udpIn") ) {

            switch (msg->getKind())
            {
            case UDP_I_ERROR:

                /**
                 * ICMP Error
                 */
                errInfo = check_and_cast<UDPErrorIndication *>(msg->removeControlInfo());

                DBGPRT(AUX, Warn, this->getFullPath())
                        << "Received UDP message with getKind() = UDP_I_ERROR"
                        << std::endl;
                // TODO: decide what to do with ICMP error report

                delete errInfo;

                inPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;

                break;

            case UDP_I_DATA:

                /**
                 * Valid UDP packet encapsulating a CcnPacket
                 */
                udpCtx = check_and_cast<UDPDataIndication *>(msg->removeControlInfo());


                /* Create CcnContext with endpoint information and attach to message
                 */
                if ( udpCtx->getSrcAddr().isIPv6() )
                    ccnCtx = new CcnContext(AF_INET6);
                else
                    ccnCtx = new CcnContext(AF_INET);

                if (!ccnCtx)
                {
                    DBGPRT(AUX, Err, this->getFullPath())
                            << "Memory allocation for CcnContext failed. Aborting incoming pkt processing"
                            << std::endl;

                    inPktsDropped++;
                    msgDrop = true;
                    goto CLEANUP_AND_EXIT;
                }

                ccnCtx->setInetInfo(udpCtx->getSrcAddr(), udpCtx->getDestAddr(), udpCtx->getSrcPort(), udpCtx->getDestPort());
                delete udpCtx;

                msg->setContextPointer((void *)ccnCtx);
                msg->setKind(PROT_CCN);


                /* Deliver to service method
                 */
                DBGPRT(EVAUX, Info, this->getFullPath())
                        << "Received CcnPacket from UDP socket with endpoints (src,dst): "
                        <<  udpCtx->getSrcAddr() << "," << udpCtx->getDestAddr()
                        << std::endl;

                inPkts++;
                fromLowerLayer ( msg );

                break;

            default:

                /**
                 * Invalid packet received at a UDP gate
                 */
                DBGPRT(EVAUX, Info, this->getFullPath())
                        << "Invalid incoming UDP Packet. Drop silently" << std::endl;
                DBGPRT(AUX, Detail, this->getFullPath())
                        << "\t received message, has invalid code: getKind()=" << msg->getKind()
                        << std::endl;

                inPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;

                break;
            }

        }
    }
    else
    {
        /**
         * A non CcnPacket from the layer below
         */

        // TODO: Error report
        inPktsDropped++;
        msgDrop = true;
        goto CLEANUP_AND_EXIT;
    }


CLEANUP_AND_EXIT:
    if (msgDrop) delete msg;
    if (ev.isGUI()) updateDisplayString();

    return;
};

