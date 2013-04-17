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
 *
 * Class CcnContext
 *
 *****************************************************************************/
//
// ---- For reference -----
//
//#include <netinet/in.h>
//
//struct sockaddr {
//    unsigned short    sa_family;    // address family, AF_xxx
//    char              sa_data[14];  // 14 bytes of protocol address
//};
//-------------------------
//#include <netinet/in.h>
//
//struct sockaddr_in {
//    short            sin_family;   // e.g. AF_INET, AF_INET6
//    unsigned short   sin_port;     // e.g. htons(3490)
//    struct in_addr   sin_addr;     // see struct in_addr, below
//    char             sin_zero[8];  // zero this if you want to
//};
//
//struct in_addr {
//    unsigned long s_addr;          // load with inet_pton()
//};
//-------------------------
//#include <netinet/in.h>
//
//struct sockaddr_in6 {
//    u_int16_t       sin6_family;   // address family, AF_INET6
//    u_int16_t       sin6_port;     // port number, Network Byte Order
//    u_int32_t       sin6_flowinfo; // IPv6 flow information
//    struct in6_addr sin6_addr;     // IPv6 address
//    u_int32_t       sin6_scope_id; // Scope ID
//};
//
//struct in6_addr {
//    unsigned char   s6_addr[16];   // load with inet_pton()
//};
//-------------------------
//#include <linux/if_ether.h>
//
//struct sockaddr_ll {
//   unsigned short  sll_family;    /* Always AF_PACKET */
//   unsigned short  sll_protocol;  /* Physical layer protocol */
//   int             sll_ifindex;   /* Interface number */
//   unsigned short  sll_hatype;    /* Header type */
//   unsigned char   sll_pkttype;   /* Packet type */
//   unsigned char   sll_halen;     /* Length of address */
//   unsigned char   sll_addr[8];   /* Physical layer address */
//};
//
//


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
        DBG(Err)
                << "CcnContext::getSrcAddress802() \n"
                << " - src.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Address type (src MAC) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::getSrcAddress802() \n"
                << " - dst.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Address type (dst MAC) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::getSrcAddressInet() \n"
                << " - src.sa.sa_family != AF_INET|AF_INET6" << std::endl;
        opp_error("Address type (src IP) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::getSrcAddressInet() \n"
                << " - dst.sa.sa_family != AF_INET|AF_INET6" << std::endl;
        opp_error("Address type (dst IP) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::getSrcPort() \n"
                << " - src.sa.sa_family != AF_INET|AF_INET6" << std::endl;
        opp_error("Address type (src port) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::getDstPort() \n"
                << " - dst.sa.sa_family != AF_INET|AF_INET6" << std::endl;
        opp_error("Address type (dst port) requested does not match what is stored in CcnContext");
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
        DBG(Err)
                << "CcnContext::set802Info() \n "
                <<" - src/dst.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Cannot set CcnContext (MACAddress), failed to match address family");
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
        DBG(Err)
                << "CcnContext::set802Info() \n"
                << " - src/dst.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Cannot set CcnContext (struct sockaddr_ll), failed to match address family");
    }

    if ( (srcSockLL.sll_family != AF_PACKET) || (dstSockLL.sll_family != AF_PACKET) )
    {
        DBG(Err)
                << "CcnContext::set802Info() \n"
                << " - in provided struct sockaddr_ll, sll_family != AF_PACKET" << std::endl;
        opp_error("Cannot set CcnContext (struct sockaddr_ll), failed to match address family");
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
        DBG(Err)
                << "CcnContext::set802Info() \n"
                << " - src/dst.sa.sa_family != AF_PACKET" << std::endl;
        opp_error("Cannot set CcnContext (char * src/dst), failed to match address family");
    }

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
            ( (src.sa.sa_family != AF_INET) && (src.sa.sa_family != AF_INET6) ) ||
            ( (dst.sa.sa_family != AF_INET) && (dst.sa.sa_family != AF_INET6) )
        )
    {
        DBG(Err)
                << "CcnContext::setInetInfo() \n"
                << " - src/dst.sa.sa_family != AF_INET|AF_INET6" << std::endl;
        opp_error("Cannot set CcnContext (IPvXAddress, port), failed to match address family");
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
        DBG(Err)
                << "CcnContext::setInetInfo() \n"
                << " - provided src and dst (IPvXAddress) are not of same address family (AF_INET|AF_INET6)" << std::endl;
        opp_error("Cannot set CcnContext (IPvXAddress, port), failed to determine the address family of the provided endpoints");
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
            DBG(Err)
                    << "CcnContext::setInetInfo() \n"
                    << " - src|dst.sa.sa_family != AF_INET" << std::endl;
            opp_error("Cannot set CcnContext (struct sockaddr_in), failed to match address family");
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
            DBG(Err)
                    << "CcnContext::setInetInfo() \n"
                    << " - src|dst.sa.sa_family != AF_INET6" << std::endl;
            opp_error("Cannot set CcnContext (struct sockaddr_in6), failed to match address family");
        }

        memset (&(src.in6) , 0, sizeof (sockaddr_in6));
        memset (&(dst.in6) , 0, sizeof (sockaddr_in6));

        memcpy (&(src.in6), &srcSockInet, sizeof(sockaddr_in6) );
        memcpy (&(dst.in6), &dstSockInet, sizeof(sockaddr_in6) );
    }
    else
    {
        DBG(Err)
                << "CcnContext::setInetInfo() \n"
                << " - provided src and dst (struct sockaddr) are not of same address family (AF_INET|AF_INET6)" << std::endl;
        opp_error("Cannot set CcnContext (struct sockaddr), failed to determine the address family of the provided endpoints");
    }

    return;
};







/*****************************************************************************
 *
 * Class CcnInet
 *
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
     * so that I can use the DBG() macro
     */
    if (::defaultDebugLevel == 0) {
        cModule *netTopology = getParentModule()->getParentModule();

        if (netTopology->par("defaultDebugLevel").isSet())
            ::defaultDebugLevel = netTopology->par("defaultDebugLevel");
        else
            ::defaultDebugLevel = CCNL_DEFAULT_DBG_LVL;
        iSetGlobalDbgLvl = true;
    }

    if (par("debugLevel").isSet())
        debugLevel = par("debugLevel");
    else
        debugLevel = ::defaultDebugLevel;

    if ( iSetGlobalDbgLvl )
        DBG(Info) << "Global default debug level set to: " << ::defaultDebugLevel << std::endl;
    DBG(Info) << this->getFullPath() << " - debug level set to " << debugLevel << std::endl;



    /* Set up access to NICs (Interface Table of INET framework)
     */
    nicTbl = InterfaceTableAccess().get();


    /* Create UDP-bound Faces
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
            //udpSockTbl[i].socket.setTimeToLive(ttl);
            //udpSockTbl[i].socket.setTypeOfService(tos);
        }

        DBG(Info) << this->getFullName() << " - Finished initialising UDP faces" << std::endl;
    }



    /* stat counters reset
     */
    inPktsDropped = 0;
    outPktsDropped = 0;
    inPkts = 0;
    outPkts = 0;


#if defined(TEST1)
    runTest1();
#endif

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
     * Normally the Inet interface module does not use timers,
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
        DBG(Warn) << this->getFullPath() << " - Could not find a Gate to Link Layer" << std::endl;
        return 0;
    }

    for (int i=0 ; i < numNics ; i++) {
        nic = nicTbl->getInterface(i);
        if (nic->getMacAddress() == macAddr)
            return nic;
    }

    DBG(Warn) << this->getFullPath()
            << " - Could not resolve MAC Address " << macAddr.str()
            << " to a NIC (InterfaceEntry *)"
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
    {
        DBG(Warn) << this->getFullPath()
                << " - Could not find a NIC configured with MAC addr "
                << macAddr.str() <<
                std::endl;
        return -1;
    }

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
    {
        DBG(Warn) << this->getFullPath()
                << " - Could not find a NIC configured with MAC addr "
                << macAddr.str() <<
                std::endl;
        return -1;
    }

    if ( (gateSize("ifOut")==0) ) {
        DBG(Warn) << this->getFullPath()
                << " - Could not find a Gate to Link Layer"
                << std::endl;
        return -1;
    }
    else {
        gateId = gate("ifOut", 0)->getId() + nic->getNetworkLayerGateIndex();

        DBG(Warn) << this->getFullPath()
                << " - The gate for MAC addr "<< macAddr.str()
                << " is " << gateId
                << std::endl;

        return gateId;
    }
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

    DBG(Warn) << this->getFullName() << " - Could not resolve IPvX Address to an open UDP socket" << std::endl;
    DBG(Detail) << std::endl;   // TODO: more detailed info

    return NULL;
}



/*****************************************************************************
 * Update the popup information presented at tkenv
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

        DBG(Detail)
                << this->getFullName()
                << " - Not a CcnPacket (dynamic cast failed) or getKind() not set to PROT_CCN."
                << std::endl;

        EV << "Message heading South not identified as a CcnPacket message. Drop silently" << std::endl;
        outPktsDropped++;
        msgDrop = true;
        goto CLEANUP_AND_EXIT;
    }

    if ( ! (ccnCtx = (CcnContext *) ccnPkt->getContextPointer() ) )   {

        DBG(Err)
                << this->getFullName()
                << " - getContextPointer() on CcnPacket returned 0"
                << std::endl;

        EV << "CcnPacket without CcnContext -- Drop silently" << std::endl;
        outPktsDropped++;
        msgDrop = true;
        goto CLEANUP_AND_EXIT;
    };


    /**
     * Check the type of destination address and decide how to send it
     */
    if ( ccnCtx->isAddressFamily(AF_INET) || ccnCtx->isAddressFamily(AF_INET6)) {

        /**
         * Use UDP sockets
         */

        /*
         * TODO: delete this
         * prepare for UDP/IPvX encap
         *
        IPv4ControlInfo *ipCtx = new IPv4ControlInfo();
        ipCtx->setProtocol(IP_PROT_CCN);
        //ipCtx->setSrcAddr( ccnCtx->getSrcAddressInet4() );
        ipCtx->setDestAddr( ccnCtx->getDstAddressInet4() );
        ipCtx->setTimeToLive( CCNoIP_TTL );
        ipCtx->setTypeOfService( CCNoIP_TOS );
        ipCtx->setDontFragment(false);
        ccnPkt->setControlInfo(ipCtx);
        -------------
        IPv6ControlInfo *ip6Ctx = new IPv6ControlInfo();
        ip6Ctx->setProtocol(IP_PROT_CCN);
        //ip6Ctx->setSrcAddr( ccnCtx->getSrcAddressInet6() );
        ip6Ctx->setDestAddr( ccnCtx->getDstAddressInet6() );
        ip6Ctx->setHopLimit( CCNoIP_TTL );
        ip6Ctx->setTrafficClass( CCNoIP_TOS );
        ccnPkt->setControlInfo(ip6Ctx);
         */

        UDPSocket *s = whichUdpSocket( ccnCtx->getSrcAddressInet(), ccnCtx->getSrcPort() );

        if (s) {
            s->sendTo (ccnPkt, ccnCtx->getDstAddressInet(), ccnCtx->getDstPort() );

            EV << "Sending CcnPacket via UDP socket" << std::endl;
            outPkts++;
            goto CLEANUP_AND_EXIT;

        } else {

            DBG(Detail)
                    << this->getFullName()
                    << " - whichUdpSocket() failed to provide an open socket bound to "
                    << ccnCtx->getSrcAddressInet() << ":" << ccnCtx->getSrcPort()
                    << std::endl;

            EV << "CcnPacket could not be sent via UDP -- Drop silently" << std::endl;
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
        macCtx->setEtherType( ETHERTYPE_IPv4 );  //Mask ourselves as IP traffic
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
                DBG(Warn) << this->getFullPath()
                        << " - Could not find an active Gate to Link Layer"
                        << std::endl;
                outPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;
            }

            if (numNICs != outGateSize) {
                DBG(Warn) << this->getFullPath()
                        << " - Number of NICs = "<< numNICs
                        << ", size of eth[] vector = "<< outGateSize
                        <<". Might create issues!"
                        << std::endl;
            }

            /* broadcast
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

            EV << "Broadcasting CcnPacket via all Link Layer NICs" << std::endl;
            goto CLEANUP_AND_EXIT;
        }


        /* If we have a source mac address, find the right interface to send to
         */
        nicGateId = gateIdFromMac( ccnCtx->getSrcAddress802() );
        if (nicGateId != -1)
        {
            /**
             * Found NIC to use
             */
            send(ccnPkt, nicGateId);

            EV << "Sending CcnPacket via Link Layer NIC" << std::endl;
            outPkts++;
            goto CLEANUP_AND_EXIT;

        } else {

            /**
             * The src MAC does not match one of my NICs
             */

            DBG(Detail)
                    << this->getFullPath()
                    << " - gateIdFromMac() failed to find a Gate ID for a NIC on which MAC Address "
                    << ccnCtx->getSrcAddress802() << " is configured"
                    << std::endl;

            EV << "CcnPacket not sent via MAC Layer (invalid src MAC addr). Drop silently" << std::endl;

            outPktsDropped++;
            msgDrop = true;
            goto CLEANUP_AND_EXIT;
        }

    } else {

        /**
         * Missing the right address information to determine how to send
         */

        DBG(Err)
                << this->getFullPath()
                << " - CcnContext is has not been configured with AF_INET or AF_PACKET address family."
                << " Cannot decide how to forward the packet"
                << std::endl;

        EV << "Invalid CcnContext -- Drop silently" << std::endl;

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
        EV << getFullPath()
                << " - Not a CcnAppMessage heading North. Caught and sent to /dev/null."
                << std::endl;
    }

    if (hasGate("upperOut") && gate("upperOut")->isConnectedOutside() )
    {
        send(msg, "upperOut");
        EV << getFullPath()
                << " - Passing CcnAppMessage to layer above."
                << std::endl;
    }
    else {
        delete msg;
        EV << getFullPath()
                << " - Piping CcnAppMessage to /dev/null (no connected layer above)."
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
            EV << getFullName()
                    << " - Received from upper layer message that is not a CcnAppMessage. Ignore"
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

                DBG(Err)
                        << this->getFullName()
                        << " - CcnPacket received at a NIC, which has MTU smaller that the packet size"
                        << std::endl;

                EV << "CcnPacket too big -- Drop silently" << std::endl;

                inPktsDropped++;
                msgDrop = true;
                goto CLEANUP_AND_EXIT;
            }

            /* Create CcnContext and attach it to msg
             */
            Ieee802Ctrl *macCtx = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());

            if ( ! (ccnCtx = new CcnContext(AF_PACKET)) ) {

                DBG(Err)
                        << this->getFullName()
                        << " - Memory allocation for CcnContext failed"
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
            EV << "Received CcnPacket from the Link Layer with endpoints (src,dst) "
                    <<  macCtx->getSrc() << "," << macCtx->getDest()
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

                DBG(Warn)
                        << this->getFullName()
                        << " - Received UDP message with getKind()=UDP_I_ERROR"
                        << std::endl;
                DBG(Detail) << std::endl; // TODO: decide what to do with ICMP error report

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
                    DBG(Err)
                            << this->getFullName()
                            << " - Memory allocation for CcnContext failed"
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
                EV << "Received CcnPacket from UDP socket with endpoints (src,dst) "
                        <<  udpCtx->getSrcAddr() << "," << udpCtx->getDestAddr()
                        << std::endl;

                inPkts++;
                fromLowerLayer ( msg );

                break;

            default:

                /**
                 * Invalid packet received at a UDP gate
                 */
                DBG(Err)
                        << this->getFullName()
                        << " - Received message, which is supposed to be a UDP packet but has invalid code, getKind()="
                        << msg->getKind()
                        << std::endl;

                EV << "Invalid UDP Packet arrived at a UDP connected gate -- Drop silently" << std::endl;

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









/*****************************************************************************
 * TEST1 - Test code below this line
 */

#ifdef TEST1

#define START_TEST  11
#define STOP_TEST  10
#define TX_PING  12
#define ECHO_PONG  13

void
CcnInet::runTest1()
{
    /*
     * Reference code for accessing the omnet module hierarchy
     *
    // get my module id
    int myModuleId = getId();

    // get module object pointer by its ID
    cModule *module = simulation.getModule(myModuleId);

    // access the module one level up in the hierarchy
    cModule *myParentModule = getParentModule ();

    // get the ID of a submodule under me at the 5th pos of module vector "child" (i.e. child[5])
    int subModuleId = findSubmodule("child",5);

    // ..or directly the submodule under me at the 5th pos of module vector "child" (i.e. child[5])
    cModule *subModule = getSubmodule("child",5);

    // access a module using a path name in the module hierarchy of the simulation
    cModule *netModule = simulation.getModuleByPath("Ether_Pt2Pt_1c_1s.client1");

    // iterate and discover all modules names one level up in the module hierarchy
    for (cSubModIterator iter(*getParentModule()); !iter.end(); iter++)
    {
        ev << iter()->getFullName();
    }

     *
     */


    /* access the network topology Module
     */
    cModule *topo = simulation.getModuleByPath("eth_pt2pt_1cli_1srv");
    //cModule *topo = getParentModule()->getParentModule();

    DBG(Info) << "We use topology " << topo->getFullName() << std::endl;

    /* Check which host I am and if I m initiating a test
     */
    cModule *localhost = getParentModule();

    const char *pinger = topo->par ("pinger");

    if ( !pinger || ( strcmp (localhost->getName(), pinger) == 0 ) )
    {
        if (ev.isGUI())
            bubble("I m a pinger!");

        simtime_t   startTime = topo->par ("testStartTime");
        simtime_t   stopTime = topo->par ("testStopTime");

        /* Prepare timers for scheduling the beginning and end of the test
         */
        cMessage *testStartTimer = new cMessage("testStartTimer");
        cMessage *testStopTimer = new cMessage("testStopTimer");

        testStartTimer->setKind(START_TEST);
        testStopTimer->setKind(STOP_TEST);

        /* Store in Stop Timer the next scheduled transmission
         */
        testStopTimer->setContextPointer(testStartTimer);

        scheduleAt(startTime, testStartTimer);
        scheduleAt(stopTime, testStopTimer);

        DBG(Info) << "TEST1 Scheduled for execution " << std::endl;
    }

    return;
};


void
CcnInet::fromUpperLayer(cMessage* msg)
{
    return;
};


void
CcnInet::fromLowerLayer(cMessage* msg)
{
    char        dataBuffer [100];
    int         dataSize = 0;
    double      responseDelay = 0;
    char        pongb[100];

    CcnPacket   *pkt = check_and_cast<CcnPacket *> (msg);
    cModule     *localhost = getParentModule();
    cModule     *topo = localhost->getParentModule();


    /* Read/print CcnContext (sort of control info) attached to the packet
     */
    CcnContext *ctx = (CcnContext *) pkt->getContextPointer();
    DBG(Info) << "TEST1 -- Host " << localhost << " Received CcnPacket with src MAC: " << ctx->getSrcAddress802().str() << "and dst MAC: " << ctx->getDstAddress802().str() << std::endl;

    /* Read/print pkt data
     */
    dataSize = pkt->copyDataToBuffer(dataBuffer, 100);
    memset (dataBuffer+dataSize, '\0', 1);
    DBG(Info) << "TEST1 -- Host " << localhost << " CcnPacket data: [" << dataBuffer << "] CcnPacket len: [" << dataSize << "]" <<std::endl;

    if ( strstr(pkt->getName(), "PING") != 0)
    {
        /**
         * Received a ping, schedule a pong.
         */
        responseDelay = topo->par("responseInterval");

        /* prepare PONGx string
         */
        int cnt = strtol (pkt->getName()+4, (char **) NULL, 10);
        sprintf (pongb, "PONG%d", cnt);

        cMessage *pongTimer = new cMessage(pongb);
        pongTimer->setKind(ECHO_PONG);
        scheduleAt(simTime() + responseDelay, pongTimer);
        delete pkt;
        DBG(Info) << "TEST1 -- Host " << localhost << " Received PING, Scheduled " << pongb << std::endl;
    }
    else if ( strstr(pkt->getName(), "PONG") != 0 )
    {
        /**
         * Received a pong, do nothing
         */
        DBG(Info) << "TEST1 -- Host " << localhost << " Received PONG" << std::endl;
        delete pkt;
    }

    delete ctx;
    return;
};




void
CcnInet::sendPingPongMsg (const char *type)
{
    MACAddress myMacAddr;
    MACAddress dstMacAddr;
    cModule *host;
    cModule *topo;
    cModule *mac;
    const char *dstHost;
    char pktBuffer [] = "This is supposed to be a CCN Packet";

    int pktBufferLen = strlen(pktBuffer) + 1;


    /* Find my MAC address
     */
    host = getParentModule();    // local host module

    DBG(Info) << "Host " << host->getFullPath() << " will send message " << type << std::endl;

    if (!host)
        error("cannot resolve MAC address for '%s': not a valid module path name", host->getFullPath().c_str() );

    mac = host->getModuleByRelativePath("eth[0].mac"); // host->getSubmodule("mac");
    if (!mac)
        error("module '%s' has no 'mac' submodule", host->getFullPath().c_str() );
    myMacAddr.setAddress(mac->par("address"));



    /* Find the other host's MAC address
     */
    topo = getParentModule()->getParentModule();    // net topology module

    if ( strcmp (host->getName(), "client1") == 0 )
        dstHost = topo->par("client1SendsTo");
    else if ( strcmp (host->getName(), "client2") == 0 )
        dstHost = topo->par("client2SendsTo");

    if (dstHost[0])
    {
        host = simulation.getModuleByPath(dstHost);     // remote host module
        if (!host)
            error("cannot resolve MAC address for '%s': not a valid module path name", dstHost);

        cModule *mac = host->getModuleByRelativePath("eth[0].mac"); //host->getSubmodule("mac");
        if (!mac)
            error("module '%s' has no 'mac' submodule", dstHost);
        dstMacAddr.setAddress(mac->par("address"));
    }

    DBG(Info) << "Message will be transmitted to host " << host->getFullPath() << std::endl;

    /* Prepare and send message
     */
    CcnPacket *pkt = new CcnPacket(type);
    pkt->setKind(PROT_CCN);
    CcnContext *ctx = new CcnContext(AF_PACKET);
    ctx->set802Info(myMacAddr, dstMacAddr);
    pkt->setDataFromBuffer(pktBuffer, pktBufferLen);
    pkt->addByteLength(pktBufferLen);
    pkt->setContextPointer(ctx);

    DBG(Info) << "--sizeof(Message)=" << sizeof(pktBuffer) << std::endl;
    DBG(Info) << "--strlen(Message)=" << pktBufferLen << std::endl;
    DBG(Info) << "--pkt->getByteLength()=" << pkt->getByteLength() << std::endl;

    //if (ev.isGUI())
    //    bubble(type);

    toLowerLayer (pkt);

    return;
}




void
CcnInet::handleSelfMessage(cMessage* msg)
{
    cModule *topo = getParentModule()->getParentModule();

    static int nextPing;
    char pingb[100];

    /* Parse timer message
     */
    if (msg->isSelfMessage() && (msg->getKind() == START_TEST) )
    {
        /* Initiate message exchange (PING)
         */
        nextPing = 1;
        sprintf (pingb, "PING%d", nextPing);
        sendPingPongMsg (pingb);

        /* Schedule next transmission
         */
        simtime_t   delay = topo->par("sendInterval");
        msg->setKind(TX_PING);                      // using same message for the new timer
        nextPing++;
        sprintf (pingb, "PING%d", nextPing);
        msg->setName(pingb);
        scheduleAt(simTime() + delay, msg);
    }
    else if (msg->isSelfMessage() && (msg->getKind() == STOP_TEST) )
    {
        /* Delete next scheduled transmission
         */
        cMessage *timer = (cMessage *) msg->getContextPointer();
        if (timer->isScheduled())
            cancelEvent (timer);

        delete timer;
        delete msg;
    }
    else if (msg->isSelfMessage() && (msg->getKind() == TX_PING) )
    {
        /* Initiate a new message exchange (send ping)
         */
        sendPingPongMsg (msg->getName());

        /* Schedule next transmission
         */
        simtime_t   delay = topo->par("sendInterval");
        nextPing++;
        sprintf (pingb, "PING%d", nextPing);
        msg->setName(pingb);

        scheduleAt(simTime() + delay , msg);
    }
    else if (msg->isSelfMessage() && (msg->getKind() == ECHO_PONG) )
    {
        /* Respond to message exchange (send pong)
         */
        sendPingPongMsg (msg->getName());
        delete msg;
    }
    else
    {
        error("Unrecognized self message (%s)%s", msg->getClassName(), msg->getName());
    }

    return;
};

#endif // TEST1
