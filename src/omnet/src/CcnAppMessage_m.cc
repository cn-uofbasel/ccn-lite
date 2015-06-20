//
// Generated file, do not edit! Created by opp_msgc 4.5 from CcnAppMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "CcnAppMessage_m.h"

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
    cEnum *e = cEnum::find("CcnAppMessageType");
    if (!e) enums.getInstance()->add(e = new cEnum("CcnAppMessageType"));
    e->insert(CCN_APP_INTEREST, "CCN_APP_INTEREST");
    e->insert(CCN_APP_CONTENT, "CCN_APP_CONTENT");
);

EXECUTE_ON_STARTUP(
    cEnum *e = cEnum::find("CcnRelayAction");
    if (!e) enums.getInstance()->add(e = new cEnum("CcnRelayAction"));
    e->insert(CONTENT_REGISTER, "CONTENT_REGISTER");
    e->insert(CONTENT_UNREGISTER, "CONTENT_UNREGISTER");
    e->insert(CONTENT_DELIVER, "CONTENT_DELIVER");
);

Register_Class(CcnAppMessage);

CcnAppMessage::CcnAppMessage(const char *name, int kind) : ::ByteArrayMessage(name,kind)
{
    this->type_var = 0;
    this->action_var = 0;
    this->contentName_var = 0;
    this->seqNr_var = 0;
    this->numChunks_var = 0;
    this->chunkSize_var = 0;
}

CcnAppMessage::CcnAppMessage(const CcnAppMessage& other) : ::ByteArrayMessage(other)
{
    copy(other);
}

CcnAppMessage::~CcnAppMessage()
{
}

CcnAppMessage& CcnAppMessage::operator=(const CcnAppMessage& other)
{
    if (this==&other) return *this;
    ::ByteArrayMessage::operator=(other);
    copy(other);
    return *this;
}

void CcnAppMessage::copy(const CcnAppMessage& other)
{
    this->type_var = other.type_var;
    this->action_var = other.action_var;
    this->contentName_var = other.contentName_var;
    this->seqNr_var = other.seqNr_var;
    this->numChunks_var = other.numChunks_var;
    this->chunkSize_var = other.chunkSize_var;
}

void CcnAppMessage::parsimPack(cCommBuffer *b)
{
    ::ByteArrayMessage::parsimPack(b);
    doPacking(b,this->type_var);
    doPacking(b,this->action_var);
    doPacking(b,this->contentName_var);
    doPacking(b,this->seqNr_var);
    doPacking(b,this->numChunks_var);
    doPacking(b,this->chunkSize_var);
}

void CcnAppMessage::parsimUnpack(cCommBuffer *b)
{
    ::ByteArrayMessage::parsimUnpack(b);
    doUnpacking(b,this->type_var);
    doUnpacking(b,this->action_var);
    doUnpacking(b,this->contentName_var);
    doUnpacking(b,this->seqNr_var);
    doUnpacking(b,this->numChunks_var);
    doUnpacking(b,this->chunkSize_var);
}

int CcnAppMessage::getType() const
{
    return type_var;
}

void CcnAppMessage::setType(int type)
{
    this->type_var = type;
}

int CcnAppMessage::getAction() const
{
    return action_var;
}

void CcnAppMessage::setAction(int action)
{
    this->action_var = action;
}

const char * CcnAppMessage::getContentName() const
{
    return contentName_var.c_str();
}

void CcnAppMessage::setContentName(const char * contentName)
{
    this->contentName_var = contentName;
}

int CcnAppMessage::getSeqNr() const
{
    return seqNr_var;
}

void CcnAppMessage::setSeqNr(int seqNr)
{
    this->seqNr_var = seqNr;
}

int CcnAppMessage::getNumChunks() const
{
    return numChunks_var;
}

void CcnAppMessage::setNumChunks(int numChunks)
{
    this->numChunks_var = numChunks;
}

int CcnAppMessage::getChunkSize() const
{
    return chunkSize_var;
}

void CcnAppMessage::setChunkSize(int chunkSize)
{
    this->chunkSize_var = chunkSize;
}

class CcnAppMessageDescriptor : public cClassDescriptor
{
  public:
    CcnAppMessageDescriptor();
    virtual ~CcnAppMessageDescriptor();

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

Register_ClassDescriptor(CcnAppMessageDescriptor);

CcnAppMessageDescriptor::CcnAppMessageDescriptor() : cClassDescriptor("CcnAppMessage", "ByteArrayMessage")
{
}

CcnAppMessageDescriptor::~CcnAppMessageDescriptor()
{
}

bool CcnAppMessageDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<CcnAppMessage *>(obj)!=NULL;
}

const char *CcnAppMessageDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int CcnAppMessageDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 6+basedesc->getFieldCount(object) : 6;
}

unsigned int CcnAppMessageDescriptor::getFieldTypeFlags(void *object, int field) const
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
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<6) ? fieldTypeFlags[field] : 0;
}

const char *CcnAppMessageDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "type",
        "action",
        "contentName",
        "seqNr",
        "numChunks",
        "chunkSize",
    };
    return (field>=0 && field<6) ? fieldNames[field] : NULL;
}

int CcnAppMessageDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='t' && strcmp(fieldName, "type")==0) return base+0;
    if (fieldName[0]=='a' && strcmp(fieldName, "action")==0) return base+1;
    if (fieldName[0]=='c' && strcmp(fieldName, "contentName")==0) return base+2;
    if (fieldName[0]=='s' && strcmp(fieldName, "seqNr")==0) return base+3;
    if (fieldName[0]=='n' && strcmp(fieldName, "numChunks")==0) return base+4;
    if (fieldName[0]=='c' && strcmp(fieldName, "chunkSize")==0) return base+5;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *CcnAppMessageDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "int",
        "int",
        "string",
        "int",
        "int",
        "int",
    };
    return (field>=0 && field<6) ? fieldTypeStrings[field] : NULL;
}

const char *CcnAppMessageDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0:
            if (!strcmp(propertyname,"enum")) return "CcnAppMessageType";
            return NULL;
        case 1:
            if (!strcmp(propertyname,"enum")) return "CcnRelayAction";
            return NULL;
        default: return NULL;
    }
}

int CcnAppMessageDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    CcnAppMessage *pp = (CcnAppMessage *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string CcnAppMessageDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    CcnAppMessage *pp = (CcnAppMessage *)object; (void)pp;
    switch (field) {
        case 0: return long2string(pp->getType());
        case 1: return long2string(pp->getAction());
        case 2: return oppstring2string(pp->getContentName());
        case 3: return long2string(pp->getSeqNr());
        case 4: return long2string(pp->getNumChunks());
        case 5: return long2string(pp->getChunkSize());
        default: return "";
    }
}

bool CcnAppMessageDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    CcnAppMessage *pp = (CcnAppMessage *)object; (void)pp;
    switch (field) {
        case 0: pp->setType(string2long(value)); return true;
        case 1: pp->setAction(string2long(value)); return true;
        case 2: pp->setContentName((value)); return true;
        case 3: pp->setSeqNr(string2long(value)); return true;
        case 4: pp->setNumChunks(string2long(value)); return true;
        case 5: pp->setChunkSize(string2long(value)); return true;
        default: return false;
    }
}

const char *CcnAppMessageDescriptor::getFieldStructName(void *object, int field) const
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

void *CcnAppMessageDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    CcnAppMessage *pp = (CcnAppMessage *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


