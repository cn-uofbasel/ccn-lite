//
// Generated file, do not edit! Created by opp_msgc 4.4 from contentto.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "contentto_m.h"

USING_NAMESPACE

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




Register_Class(ContentTo);

ContentTo::ContentTo(const char *name, int kind) : ::cPacket(name,kind)
{
    this->host_var = 0;
    this->port_var = 0;
    this->name_var = 0;
    this->data_var = 0;
    this->toPrefix_var = 0;
    this->isSend_var = 0;
}

ContentTo::ContentTo(const ContentTo& other) : ::cPacket(other)
{
    copy(other);
}

ContentTo::~ContentTo()
{
}

ContentTo& ContentTo::operator=(const ContentTo& other)
{
    if (this==&other) return *this;
    ::cPacket::operator=(other);
    copy(other);
    return *this;
}

void ContentTo::copy(const ContentTo& other)
{
    this->host_var = other.host_var;
    this->port_var = other.port_var;
    this->name_var = other.name_var;
    this->data_var = other.data_var;
    this->toPrefix_var = other.toPrefix_var;
    this->isSend_var = other.isSend_var;
}

void ContentTo::parsimPack(cCommBuffer *b)
{
    ::cPacket::parsimPack(b);
    doPacking(b,this->host_var);
    doPacking(b,this->port_var);
    doPacking(b,this->name_var);
    doPacking(b,this->data_var);
    doPacking(b,this->toPrefix_var);
    doPacking(b,this->isSend_var);
}

void ContentTo::parsimUnpack(cCommBuffer *b)
{
    ::cPacket::parsimUnpack(b);
    doUnpacking(b,this->host_var);
    doUnpacking(b,this->port_var);
    doUnpacking(b,this->name_var);
    doUnpacking(b,this->data_var);
    doUnpacking(b,this->toPrefix_var);
    doUnpacking(b,this->isSend_var);
}

const char * ContentTo::getHost() const
{
    return host_var.c_str();
}

void ContentTo::setHost(const char * host)
{
    this->host_var = host;
}

int ContentTo::getPort() const
{
    return port_var;
}

void ContentTo::setPort(int port)
{
    this->port_var = port;
}

const char * ContentTo::getName() const
{
    return name_var.c_str();
}

void ContentTo::setName(const char * name)
{
    this->name_var = name;
}

const char * ContentTo::getData() const
{
    return data_var.c_str();
}

void ContentTo::setData(const char * data)
{
    this->data_var = data;
}

const char * ContentTo::getToPrefix() const
{
    return toPrefix_var.c_str();
}

void ContentTo::setToPrefix(const char * toPrefix)
{
    this->toPrefix_var = toPrefix;
}

bool ContentTo::getIsSend() const
{
    return isSend_var;
}

void ContentTo::setIsSend(bool isSend)
{
    this->isSend_var = isSend;
}

class ContentToDescriptor : public cClassDescriptor
{
  public:
    ContentToDescriptor();
    virtual ~ContentToDescriptor();

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

Register_ClassDescriptor(ContentToDescriptor);

ContentToDescriptor::ContentToDescriptor() : cClassDescriptor("ContentTo", "cPacket")
{
}

ContentToDescriptor::~ContentToDescriptor()
{
}

bool ContentToDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<ContentTo *>(obj)!=NULL;
}

const char *ContentToDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int ContentToDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 6+basedesc->getFieldCount(object) : 6;
}

unsigned int ContentToDescriptor::getFieldTypeFlags(void *object, int field) const
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

const char *ContentToDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "host",
        "port",
        "name",
        "data",
        "toPrefix",
        "isSend",
    };
    return (field>=0 && field<6) ? fieldNames[field] : NULL;
}

int ContentToDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='h' && strcmp(fieldName, "host")==0) return base+0;
    if (fieldName[0]=='p' && strcmp(fieldName, "port")==0) return base+1;
    if (fieldName[0]=='n' && strcmp(fieldName, "name")==0) return base+2;
    if (fieldName[0]=='d' && strcmp(fieldName, "data")==0) return base+3;
    if (fieldName[0]=='t' && strcmp(fieldName, "toPrefix")==0) return base+4;
    if (fieldName[0]=='i' && strcmp(fieldName, "isSend")==0) return base+5;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *ContentToDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "string",
        "int",
        "string",
        "string",
        "string",
        "bool",
    };
    return (field>=0 && field<6) ? fieldTypeStrings[field] : NULL;
}

const char *ContentToDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int ContentToDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    ContentTo *pp = (ContentTo *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string ContentToDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    ContentTo *pp = (ContentTo *)object; (void)pp;
    switch (field) {
        case 0: return oppstring2string(pp->getHost());
        case 1: return long2string(pp->getPort());
        case 2: return oppstring2string(pp->getName());
        case 3: return oppstring2string(pp->getData());
        case 4: return oppstring2string(pp->getToPrefix());
        case 5: return bool2string(pp->getIsSend());
        default: return "";
    }
}

bool ContentToDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    ContentTo *pp = (ContentTo *)object; (void)pp;
    switch (field) {
        case 0: pp->setHost((value)); return true;
        case 1: pp->setPort(string2long(value)); return true;
        case 2: pp->setName((value)); return true;
        case 3: pp->setData((value)); return true;
        case 4: pp->setToPrefix((value)); return true;
        case 5: pp->setIsSend(string2bool(value)); return true;
        default: return false;
    }
}

const char *ContentToDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldStructNames[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };
    return (field>=0 && field<6) ? fieldStructNames[field] : NULL;
}

void *ContentToDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    ContentTo *pp = (ContentTo *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


