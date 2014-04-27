#include <string.h>
#include <omnetpp.h>
#include <simtime.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include "interestto_m.h"
#include "contentto_m.h"
#include "lib/rapidjson/include/rapidjson/rapidjson.h"
#include "lib/rapidjson/include/rapidjson/document.h"

using namespace std;
using namespace rapidjson;


const short INTEREST_TO = 1;
const short CONTENT_TO = 2;

class Node : public cSimpleModule
{
  protected:
    // The following redefined virtual function holds the algorithm.
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void scheduleInterestToMessage(string host, int port, string toPrefix, string name, int timeMillis);
    virtual void scheduleContentToMessage(string host, int port, string toPrefix, string name, string data, int timeMillis);
};

// The module class needs to be registered with OMNeT++
Define_Module(Node
);

void Node::initialize()
{
    // const char json[] = "{\"packets\":[{\"type\":\"interest\",\"from\":{\"host\":\"localhost\",\"port\":1,\"prefix\":\"docRepo1\",\"typ\":\"ComputeNode\"},\"to\":{\"host\":\"localhost\",\"port\":2,\"prefix\":\"docRepo2\",\"typ\":\"ComputeNode\"},\"timeMillis\":160,\"transmissionTime\":0,\"packet\":{\"name\":\"/interest/name\"}},{\"type\":\"content\",\"from\":{\"host\":\"localhost\",\"port\":2,\"prefix\":\"docRepo2\",\"typ\":\"ComputeNode\"},\"to\":{\"host\":\"localhost\",\"port\":1,\"prefix\":\"docRepo1\",\"typ\":\"ComputeNode\"},\"timeMillis\":161,\"transmissionTime\":0,\"packet\":{\"name\":\"/interest/name\",\"data\":\"testcontent\"}}]}";

    std::ifstream t("transmittedPackets.json");
    std::stringstream buffer;
    buffer << t.rdbuf();
    string json = buffer.str();

    rapidjson::Document d;
    d.Parse<0>(json.c_str());

    const Value& packets = d["packets"]; // Using a reference for consecutive access is handy and faster.
    assert(packets.IsArray());
    for (SizeType i = 0; i < packets.Size(); i++) {
        const Value& packet = packets[i];
        assert(packet["type"].IsString());
        string type = packet["type"].GetString();

        assert(packet.HasMember("from"));
        assert(packet["from"].IsObject());
        const Value& from = packet["from"];

        assert(from.HasMember("host"));
        assert(from["host"].IsString());
        string fromHost = from["host"].GetString();

        assert(from.HasMember("port"));
        assert(from["port"].IsNumber());
        assert(from["port"].IsInt());
        int fromPort = from["port"].GetInt();

        assert(from.HasMember("prefix"));
        assert(from["prefix"].IsString());
        string fromPrefix = from["prefix"].GetString();

        assert(packet.HasMember("to"));
        assert(packet["to"].IsObject());
        const Value& to = packet["to"];

        assert(to.HasMember("host"));
        assert(to["host"].IsString());
        string toHost = to["host"].GetString();

        assert(to.HasMember("port"));
        assert(to["port"].IsNumber());
        assert(to["port"].IsInt());
        int toPort = to["port"].GetInt();

        assert(to.HasMember("prefix"));
        assert(to["prefix"].IsString());
        string toPrefix = to["prefix"].GetString();

        assert(packet.HasMember("timeMillis"));
        assert(packet["timeMillis"].IsNumber());
        assert(packet["timeMillis"].IsInt());
        int timeMillis = packet["timeMillis"].GetInt();

        assert(packet.HasMember("transmissionTime"));
        assert(packet["transmissionTime"].IsNumber());
        assert(packet["transmissionTime"].IsInt());
        int transmissionTime = packet["transmissionTime"].GetInt();

        assert(packet.HasMember("packet"));
        assert(packet["packet"].IsObject());
        const Value& ccnPacket = packet["packet"];

        assert(ccnPacket.HasMember("name"));
        assert(ccnPacket["name"].IsString());
        string name = ccnPacket["name"].GetString();

        if(fromPrefix == getName()) {
            if(type == "interest") {
                EV << "host: " << toHost << "\n";
                EV << "port: " << toPort << "\n";
                EV << "prefix: " << toPrefix << "\n";
                EV << "name: " << name << "\n";
                EV << "time: " << timeMillis << "\n";
                scheduleInterestToMessage(toHost, toPort, toPrefix, name, timeMillis);
            } else {
                EV << "scheduleContentTo" << timeMillis << "\n";
                assert(ccnPacket.HasMember("data"));
                assert(ccnPacket["data"].IsString());
                string data = ccnPacket["data"].GetString();
                scheduleContentToMessage(toHost, toPort, toPrefix, name, data, timeMillis);
            }
            EV << "STARTTIME" << timeMillis << "\n";
        }

    }
}

void Node::handleMessage(cMessage *msg)
{
    if (msg->getKind() == INTEREST_TO) {
        InterestTo *interestTo = check_and_cast<InterestTo *>(msg);
        if(interestTo->getIsSend()) {
            vector < const char *> gateNames = getGateNames();
            cGate* maybeGate = NULL;
            for(int i = 0; i < gateSize("out"); i++) {
                int curGateID = findGate("out", i);
                cGate* curGate = gate(curGateID);
                const char* otherEndGateName = curGate->getPathEndGate()->getName();
                if(strcmp(otherEndGateName, interestTo->getToPrefix())) {
                    maybeGate = curGate;
                    break;
                }
            }

            if(maybeGate != NULL) {
                interestTo->setIsSend(false);
                send(interestTo, maybeGate);
            } else {
                EV << "WARNING: Gate " << interestTo->getToPrefix() << " not found!\n";
            }
        } else {
            EV << "Received interst message " << interestTo->getName() << "\n";
            delete interestTo;
        }

    } else if(msg->getKind() == CONTENT_TO) {

        ContentTo *contentTo = check_and_cast<ContentTo *>(msg);
        if(contentTo->getIsSend()) {
            vector < const char *> gateNames = getGateNames();
            cGate* maybeGate = NULL;

            for(int i = 0; i < gateSize("out"); i++) {
                int curGateID = findGate("out", i);
                cGate* curGate = gate(curGateID);
                const char* otherEndGateName = curGate->getPathEndGate()->getOwnerModule()->getName();
                EV << "toPrefix: " << contentTo->getToPrefix() << " otherEndGateName: " << otherEndGateName << "\n";
                if(strcmp(otherEndGateName, contentTo->getToPrefix()) == 0) {
                    maybeGate = curGate;
                    break;
                }
            }

            if(maybeGate != NULL) {
                contentTo->setIsSend(false);
                send(contentTo, maybeGate);
            } else {
                EV << "WARNING: Gate " << contentTo->getToPrefix() << " not found!\n";
            }
        } else {
            EV << "Received content message " << contentTo->getName() << " -> " << contentTo->getData() << "\n";
            delete contentTo;
        }
    } else {
        EV << "ERROR: Unkown kind " << msg->getKind() << "\n";
    }
}

void Node::scheduleInterestToMessage(string host, int port, string toPrefix, string name, int timeMillis)
{
    char msgname[10000];
    sprintf(msgname, "interest(%s)", name.c_str());

    InterestTo *msg = new InterestTo(msgname);

    // Information of the original interest
    msg->setHost(host.c_str());
    msg->setPort(port);
    msg->setName(name.c_str());
    msg->setToPrefix(toPrefix.c_str());

    // Omnet simulation parameters
    msg->setKind(INTEREST_TO);
    msg->setIsSend(true);
    float simTime = ((float)timeMillis) / 1000.0f;
    scheduleAt(simTime, msg);
}
void Node::scheduleContentToMessage(string host, int port, string toPrefix, string name, string data, int timeMillis)
{
    char msgname[100];
    sprintf(msgname, "interest(%s)", name.c_str());

    ContentTo *msg = new ContentTo(msgname);

    // Information of the original interest
    msg->setHost(host.c_str());
    msg->setPort(port);
    msg->setName(name.c_str());
    msg->setData(data.c_str());
    msg->setToPrefix(toPrefix.c_str());

    // Omnet simulation parameters
    msg->setKind(CONTENT_TO);
    msg->setIsSend(true);
    float simTime = ((float)timeMillis) / 1000.0f;
    scheduleAt(simTime, msg);
}