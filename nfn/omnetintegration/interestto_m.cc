//
// Generated file, do not edit! Created by opp_msgc 4.4 from interestto.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "interestto_m.h"

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




Register_Class(InterestTo);

InterestTo::InterestTo(const char *name, int kind) : ::cMessage(name,kind)
{
    this->host_var = 0;
    this->port_var = 0;
    this->name_var = 0;
    this->toPrefix_var = 0;
    this->isSend_var = 0;
}

InterestTo::InterestTo(const InterestTo& other) : ::cMessage(other)
{
    copy(other);
}

InterestTo::~InterestTo()
{
}

InterestTo& InterestTo::operator=(const InterestTo& other)
{
    if (this==&other) return *this;
    ::cMessage::operator=(other);
    copy(other);
    return *this;
}

void InterestTo::copy(const InterestTo& other)
{
    this->host_var = other.host_var;
    this->port_var = other.port_var;
    this->name_var = other.name_var;
    this->toPrefix_var = other.toPrefix_var;
    this->isSend_var = other.isSend_var;
}

void InterestTo::parsimPack(cCommBuffer *b)
{
    ::cMessage::parsimPack(b);
    doPacking(b,this->host_var);
    doPacking(b,this->port_var);
    doPacking(b,this->name_var);
    doPacking(b,this->toPrefix_var);
    doPacking(b,this->isSend_var);
}

void InterestTo::parsimUnpack(cCommBuffer *b)
{
    ::cMessage::parsimUnpack(b);
    doUnpacking(b,this->host_var);
    doUnpacking(b,this->port_var);
    doUnpacking(b,this->name_var);
    doUnpacking(b,this->toPrefix_var);
    doUnpacking(b,this->isSend_var);
}

const char * InterestTo::getHost() const
{
    return host_var.c_str();
}

void InterestTo::setHost(const char * host)
{
    this->host_var = host;
}

int InterestTo::getPort() const
{
    return port_var;
}

void InterestTo::setPort(int port)
{
    this->port_var = port;
}

const char * InterestTo::getName() const
{
    return name_var.c_str();
}

void InterestTo::setName(const char * name)
{
    this->name_var = name;
}

const char * InterestTo::getToPrefix() const
{
    return toPrefix_var.c_str();
}

void InterestTo::setToPrefix(const char * toPrefix)
{
    this->toPrefix_var = toPrefix;
}

bool InterestTo::getIsSend() const
{
    return isSend_var;
}

void InterestTo::setIsSend(bool isSend)
{
    this->isSend_var = isSend;
}

class InterestToDescriptor : public cClassDescriptor
{
  public:
    InterestToDescriptor();
    virtual ~InterestToDescriptor();

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

Register_ClassDescriptor(InterestToDescriptor);

InterestToDescriptor::InterestToDescriptor() : cClassDescriptor("InterestTo", "cMessage")
{
}

InterestToDescriptor::~InterestToDescriptor()
{
}

bool InterestToDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<InterestTo *>(obj)!=NULL;
}

const char *InterestToDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int InterestToDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 5+basedesc->getFieldCount(object) : 5;
}

unsigned int InterestToDescriptor::getFieldTypeFlags(void *object, int field) const
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
    };
    return (field>=0 && field<5) ? fieldTypeFlags[field] : 0;
}

const char *InterestToDescriptor::getFieldName(void *object, int field) const
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
        "toPrefix",
        "isSend",
    };
    return (field>=0 && field<5) ? fieldNames[field] : NULL;
}

int InterestToDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='h' && strcmp(fieldName, "host")==0) return base+0;
    if (fieldName[0]=='p' && strcmp(fieldName, "port")==0) return base+1;
    if (fieldName[0]=='n' && strcmp(fieldName, "name")==0) return base+2;
    if (fieldName[0]=='t' && strcmp(fieldName, "toPrefix")==0) return base+3;
    if (fieldName[0]=='i' && strcmp(fieldName, "isSend")==0) return base+4;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *InterestToDescriptor::getFieldTypeString(void *object, int field) const
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
        "bool",
    };
    return (field>=0 && field<5) ? fieldTypeStrings[field] : NULL;
}

const char *InterestToDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
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

int InterestToDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    InterestTo *pp = (InterestTo *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string InterestToDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    InterestTo *pp = (InterestTo *)object; (void)pp;
    switch (field) {
        case 0: return oppstring2string(pp->getHost());
        case 1: return long2string(pp->getPort());
        case 2: return oppstring2string(pp->getName());
        case 3: return oppstring2string(pp->getToPrefix());
        case 4: return bool2string(pp->getIsSend());
        default: return "";
    }
}

bool InterestToDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    InterestTo *pp = (InterestTo *)object; (void)pp;
    switch (field) {
        case 0: pp->setHost((value)); return true;
        case 1: pp->setPort(string2long(value)); return true;
        case 2: pp->setName((value)); return true;
        case 3: pp->setToPrefix((value)); return true;
        case 4: pp->setIsSend(string2bool(value)); return true;
        default: return false;
    }
}

const char *InterestToDescriptor::getFieldStructName(void *object, int field) const
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
    };
    return (field>=0 && field<5) ? fieldStructNames[field] : NULL;
}

void *InterestToDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    InterestTo *pp = (InterestTo *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


