//
// Generated file, do not edit! Created by opp_msgc 4.5 from CcnPacket.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "CcnPacket_m.h"

USING_NAMESPACE


// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




// Template rule for outputting std::vector<T> types
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');
    
    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

EXECUTE_ON_STARTUP(
    cEnum *e = cEnum::find("CcnPayloadType");
    if (!e) enums.getInstance()->add(e = new cEnum("CcnPayloadType"));
    e->insert(MSG_TYPE_INTEREST, "MSG_TYPE_INTEREST");
    e->insert(MSG_TYPE_CONTENT, "MSG_TYPE_CONTENT");
    e->insert(MSG_TYPE_DATA, "MSG_TYPE_DATA");
);

EXECUTE_ON_STARTUP(
    cEnum *e = cEnum::find("CcnSuiteType");
    if (!e) enums.getInstance()->add(e = new cEnum("CcnSuiteType"));
    e->insert(SUITE_CCNx0, "SUITE_CCNx0");
    e->insert(SUITE_CCNx1, "SUITE_CCNx1");
    e->insert(SUITE_NDNx0, "SUITE_NDNx0");
);

Register_Class(CcnPacket);

CcnPacket::CcnPacket(const char *name, int kind) : ::ByteArrayMessage(name,kind)
{
    this->setByteLength(6);

    this->cacheId_var = 0;
    this->suiteType_var = 0;
    this->msgPayloadType_var = 0;
}

CcnPacket::CcnPacket(const CcnPacket& other) : ::ByteArrayMessage(other)
{
    copy(other);
}

CcnPacket::~CcnPacket()
{
}

CcnPacket& CcnPacket::operator=(const CcnPacket& other)
{
    if (this==&other) return *this;
    ::ByteArrayMessage::operator=(other);
    copy(other);
    return *this;
}

void CcnPacket::copy(const CcnPacket& other)
{
    this->cacheId_var = other.cacheId_var;
    this->suiteType_var = other.suiteType_var;
    this->msgPayloadType_var = other.msgPayloadType_var;
}

void CcnPacket::parsimPack(cCommBuffer *b)
{
    ::ByteArrayMessage::parsimPack(b);
    doPacking(b,this->cacheId_var);
    doPacking(b,this->suiteType_var);
    doPacking(b,this->msgPayloadType_var);
}

void CcnPacket::parsimUnpack(cCommBuffer *b)
{
    ::ByteArrayMessage::parsimUnpack(b);
    doUnpacking(b,this->cacheId_var);
    doUnpacking(b,this->suiteType_var);
    doUnpacking(b,this->msgPayloadType_var);
}

long CcnPacket::getCacheId() const
{
    return cacheId_var;
}

void CcnPacket::setCacheId(long cacheId)
{
    this->cacheId_var = cacheId;
}

char CcnPacket::getSuiteType() const
{
    return suiteType_var;
}

void CcnPacket::setSuiteType(char suiteType)
{
    this->suiteType_var = suiteType;
}

char CcnPacket::getMsgPayloadType() const
{
    return msgPayloadType_var;
}

void CcnPacket::setMsgPayloadType(char msgPayloadType)
{
    this->msgPayloadType_var = msgPayloadType;
}

class CcnPacketDescriptor : public cClassDescriptor
{
  public:
    CcnPacketDescriptor();
    virtual ~CcnPacketDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual int findField(void *object, const char *fieldName) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual std::string getFieldAsString(void *object, int field, int i) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(CcnPacketDescriptor);

CcnPacketDescriptor::CcnPacketDescriptor() : cClassDescriptor("CcnPacket", "ByteArrayMessage")
{
}

CcnPacketDescriptor::~CcnPacketDescriptor()
{
}

bool CcnPacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<CcnPacket *>(obj)!=NULL;
}

const char *CcnPacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int CcnPacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 3+basedesc->getFieldCount(object) : 3;
}

unsigned int CcnPacketDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<3) ? fieldTypeFlags[field] : 0;
}

const char *CcnPacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "cacheId",
        "suiteType",
        "msgPayloadType",
    };
    return (field>=0 && field<3) ? fieldNames[field] : NULL;
}

int CcnPacketDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='c' && strcmp(fieldName, "cacheId")==0) return base+0;
    if (fieldName[0]=='s' && strcmp(fieldName, "suiteType")==0) return base+1;
    if (fieldName[0]=='m' && strcmp(fieldName, "msgPayloadType")==0) return base+2;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *CcnPacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "long",
        "char",
        "char",
    };
    return (field>=0 && field<3) ? fieldTypeStrings[field] : NULL;
}

const char *CcnPacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 1:
            if (!strcmp(propertyname,"enum")) return "CcnSuiteType";
            return NULL;
        case 2:
            if (!strcmp(propertyname,"enum")) return "CcnPayloadType";
            return NULL;
        default: return NULL;
    }
}

int CcnPacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    CcnPacket *pp = (CcnPacket *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string CcnPacketDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    CcnPacket *pp = (CcnPacket *)object; (void)pp;
    switch (field) {
        case 0: return long2string(pp->getCacheId());
        case 1: return long2string(pp->getSuiteType());
        case 2: return long2string(pp->getMsgPayloadType());
        default: return "";
    }
}

bool CcnPacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    CcnPacket *pp = (CcnPacket *)object; (void)pp;
    switch (field) {
        case 0: pp->setCacheId(string2long(value)); return true;
        case 1: pp->setSuiteType(string2long(value)); return true;
        case 2: pp->setMsgPayloadType(string2long(value)); return true;
        default: return false;
    }
}

const char *CcnPacketDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    };
}

void *CcnPacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    CcnPacket *pp = (CcnPacket *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


