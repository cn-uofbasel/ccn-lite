/*
 * krivine.c
 * a "Krivine lambda expression resolver" for CCN
 *
 * (C) 2014 <christian.tschudin@unibas.ch>
 *
 * 2014-03-14 created <christopher.scherb@unibas.ch>
 */

#include "ccnl.h"
#include "ccnx.h"
#include "ccnl-core.h"

#include "ccnl-pdu.c"

int
hex2int(char c)
{
    if (c >= '0' && c <= '9')
	return c - '0';
    c = tolower(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 0x0a;
    return 0;
}

int
unescape_component(unsigned char *comp) // inplace, returns len after shrinking
{
    unsigned char *in = comp, *out = comp;
    int len;

    for (len = 0; *in; len++) {
	if (in[0] != '%' || !in[1] || !in[2]) {
	    *out++ = *in++;
	    continue;
	}
	*out++ = hex2int(in[1])*16 + hex2int(in[2]);
	in += 3;
    }
    return len;
}

//FIXME!!!!!
int
splitComponents(char* namecomp, char **prefix)
{ 
    int i;
    unsigned char *cp;
    cp = strtok(namecomp, "/");
    
    while (i < (CCNL_MAX_NAME_COMP - 1) && cp) {
	prefix[i++] = cp;
	cp = strtok(NULL, "/");
    }
    prefix[i] = NULL;
    return i;
}

int
mkInterestCompute(char **namecomp, char *computation, int computationlen, int thunk, char *out)
{
    
    int len = 0, k, i;
    unsigned char *cp;
    
    len += mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	cp = (unsigned char*) strdup(*namecomp);
	k = unescape_component(cp);
	//	k = strlen(*namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, cp, k);
	len += k;
	out[len++] = 0; // end-of-component
	free(cp);
	namecomp++;
    }
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, computation, computationlen);
    if(thunk) mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "THUNK");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "NFN");
    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

    return len;  
}