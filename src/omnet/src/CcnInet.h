/*
 * CcnInet.h
 *
 * Abstraction layer from INET framework structure (Queueing, Message handling and other INET functionality)
 *
 *  Created on: Sep 18, 2012
 *      Author: manolis sifalakis
 */

#ifndef CCN_INET_H_
#define CCN_INET_H_


#include <netinet/in.h>         // sockaddr, sockaddr_in, sockaddr_in6
#include <linux/if_packet.h>    // sockaddr_ll
#include <linux/if_ether.h>

#include "shared.h"
#include "INETDefs.h"
#include "QueueBase.h"
#include "AbstractQueue.h"
#include "IPvXAddress.h"
#include "MACAddress.h"
#include "IInterfaceTable.h"
#include "UDPSocket.h"








/*****************************************************************************
 * Context information for correct handling of CCN packets passed by ccn-lite
 * to omnet
 *****************************************************************************/

class CcnContext
{
private:

    /* The following does not seem to work. C++ does not allow non-PODs
     * (multiple ctors) in unions :( So we will represent internally in
     * sockaddr structs.
     *
    union {
        struct {
            MACAddress      srcAddr;
            MACAddress      dstAddr;
        } mac;
        struct {
            IPvXAddress     srcAddr;
            IPvXAddress     dstAddr;
            unsigned short  srcPort;
            unsigned short  dstPort;
        } inet;
    } ePts;
    */

    union EndPoint {
        struct      sockaddr_in in4;
        struct      sockaddr_in6 in6;
        struct      sockaddr_ll mac;
        struct      sockaddr sa;
    } src, dst;


    // TODO: check if these are needed
    char            senderId;
    unsigned int    pktLen;
    char            *ccnPkt;


    unsigned int    debugLevel;


    CcnContext () {return; };

public:

    /**
     * Ctor and Dtor
     * Ctor sets the address family
     */
    CcnContext ( unsigned short addrFamily );
    ~CcnContext () {return; };


    /**
     * Methods to access the stored context information
     */
    bool        isAddressFamily( unsigned short addrFamily);
    MACAddress  getSrcAddress802();
    MACAddress  getDstAddress802();
    IPvXAddress getSrcAddressInet();
    IPvXAddress getDstAddressInet();
    int         getSrcPort();
    int         getDstPort();


    /**
     * The idea in the following methods is that the address family has
     * been set in advance with the constructor of the class. This
     * allows each of them to check if the arguments provided are
     * compliant to the pre-set address family.
     * If any of these methods fail to match the address family it
     * raises an exception rather than returning a code
     */
    void        set802Info(const MACAddress &srcMac, const MACAddress &dstMac);
    void        set802Info(const struct sockaddr_ll &srcSockLL, const struct sockaddr_ll &dstSockLL);
    void        set802Info(const char *srcMac, const char *dstMac);
    void        setInetInfo(const IPvXAddress &srcIp, const IPvXAddress &dstIp, int srcPort, int dstPort);
    void        setInetInfo(const struct sockaddr &srcSockInet, const struct sockaddr &dstSockInet);

};







/*****************************************************************************
 * Class CcnInet is our interface to the INET framework and its services. The
 * Ccn omnet module subclasses from CcnInet
 *****************************************************************************/

/**
 * At the moment we are inheriting from QueueBase, which implements a simple single queue
 * discipline on the AbstractQueue abstract class. The main reason for this simplicity is
 * because we assume the CCN core internally has its own queueing and scheduling.
 * Nevertheless one could replace the QueueBase with some other queueing framework here that
 * subclasses from AbstractQueue (e.g. chemical queues) to try out things independently of the
 * CCN core or before bringing them inside the core.
 */
class INET_API CcnInet : public QueueBase
{
private:

    /**
     * general state we keep about interfaces and sockets
     * that implement CCN faces
     */
    IInterfaceTable *nicTbl;    // for Faces facing NICs

    struct UdpSocketInfo {
        IPvXAddress addr;
        int         port;
        UDPSocket   socket;
    }               *udpSockTbl;   // for Faces facing UDP sockets (currently not implemented functionality)


    // TODO: implement UDP trasport in omnet for ccn-lite
    int             numUdpFaces;        // number of faces implemented by sockets
    int             basePortUdpFaces;   // first port no to be used for socket faces


    /**
     * accounting statistics
     */
    int     inPktsDropped;
    int     outPktsDropped;
    int     inPkts;
    int     outPkts;


    /**
     * local utility methods
     */
    InterfaceEntry *    netIfFromMac (const MACAddress &macAddr);
    int                 gateIdFromMac (const MACAddress& macAddr);
    UDPSocket *         whichUdpSocket (const IPvXAddress &ip, const int port);
    void                updateDisplayString();


protected:

    unsigned int     debugLevel;        // level for debugging information by this module


    /**
     * Methods that override the Queue API we inherit from INET
     */
    /** @brief Initialises the INET part of the module */
    virtual void    initialize();

    /** @brief House-keeping and reporting of the INET part of the module */
    virtual void    finish();

    /** @brief Msg dispatch loop for messages in the queue */
    virtual void    endService(cPacket *msg);

    /** @brief Main msg dispatch loop for all incoming messages */
    virtual void    handleMessage(cMessage *msg);



    /**
     * Communication with other layers (service we provide to derived class)
     * Derived classes if wished MAY extend the base functionality we
     * provide by overriding these.
     */
    virtual void toLowerLayer(cMessage* msg);
    virtual void toUpperLayer(cMessage* msg);
    int          netIfIndexFromMac(const MACAddress &macAddr);


    /**
     * Virtual methods that MUST be implemented by the derived class for servicing
     * the incoming packets from the network or the layer above, as well as timers
     */
    virtual void    fromUpperLayer(cMessage* msg) { return; }; //= 0;
    virtual void    fromLowerLayer(cMessage* msg) { return; }; //= 0;
    virtual void    handleSelfMessage(cMessage* msg) {return; }; //=0;

public:

    CcnInet ():
        QueueBase(),
        nicTbl(NULL),
        udpSockTbl(NULL),
        numUdpFaces(0),
        basePortUdpFaces(0),
        inPktsDropped(0),
        outPktsDropped(0),
        inPkts(0),
        outPkts(0),
        debugLevel(0)
    {return; };

    ~CcnInet ();

};




#endif /* CCN_INET_H_ */
