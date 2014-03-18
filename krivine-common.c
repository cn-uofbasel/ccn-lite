/*
 * krivine.c
 * a "Krivine lambda expression resolver" for CCN
 *
 * (C) 2014 <christian.tschudin@unibas.ch>
 *
 * 2014-03-14 created <christopher.scherb@unibas.ch>
 */

#ifndef KRIVINE_COMMON_C
#define KRIVINE_COMMON_C

#include "ccnl.h"
#include "ccnx.h"
#include "ccnl-core.h"

#include "ccnl-pdu.c"

#define NFN_FACE -1;

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
    int i = 0;
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

#ifndef USE_UTIL 

int
mkContent(char **namecomp,
	  unsigned char *publisher, int plen,
	  unsigned char *body, int blen,
	  unsigned char *out)
{
    int len = 0, k;

    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // interest

/*    // add signature
#ifdef USE_SIGNATURES
    if(private_key_path)
        len += add_signature(out+len, private_key_path, body, blen);  
#endif*/
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name
    while (*namecomp) {
	len += mkHeader(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG);  // comp
	k = unescape_component((unsigned char*) *namecomp);
	len += mkHeader(out+len, k, CCN_TT_BLOB);
	memcpy(out+len, *namecomp++, k);
	len += k;
	out[len++] = 0; // end-of-component
    }
    out[len++] = 0; // end-of-name

    if (publisher) {
	struct timeval t;
	unsigned char tstamp[6];
	uint32_t *sec;
	uint16_t *secfrac;

	gettimeofday(&t, NULL);
	sec = (uint32_t*)(tstamp + 0); // big endian
	*sec = htonl(t.tv_sec);
	secfrac = (uint16_t*)(tstamp + 4);
	*secfrac = htons(4048L * t.tv_usec / 1000000);
	len += mkHeader(out+len, CCN_DTAG_TIMESTAMP, CCN_TT_DTAG);
	len += mkHeader(out+len, sizeof(tstamp), CCN_TT_BLOB);
	memcpy(out+len, tstamp, sizeof(tstamp));
	len += sizeof(tstamp);
	out[len++] = 0; // end-of-timestamp

	len += mkHeader(out+len, CCN_DTAG_SIGNEDINFO, CCN_TT_DTAG);
	len += mkHeader(out+len, CCN_DTAG_PUBPUBKDIGEST, CCN_TT_DTAG);
	len += mkHeader(out+len, plen, CCN_TT_BLOB);
	memcpy(out+len, publisher, plen);
	len += plen;
	out[len++] = 0; // end-of-publisher
	out[len++] = 0; // end-of-signedinfo
    }

    len += mkHeader(out+len, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len += mkHeader(out+len, blen, CCN_TT_BLOB);
    memcpy(out + len, body, blen);
    len += blen;
    out[len++] = 0; // end-of-content

    out[len++] = 0; // end-of-contentobj

    return len;
}


struct ccnl_content_s *
add_computation_to_cache(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
        char *res, int reslen){
 
    int i = 0;
    char *out = ccnl_malloc(CCNL_MAX_PACKET_SIZE);
    memset(out, CCNL_MAX_PACKET_SIZE, 0);
    
    char **prefixcomps = ccnl_malloc(sizeof(char *) * prefix->compcnt+1);
    prefixcomps[prefix->compcnt] = 0;
    for(i = 0; i < prefix->compcnt; ++i)
    {
        prefixcomps[i] = strdup(prefix->comp[i]);
    }
    
    int len = mkContent(prefixcomps, NULL, 0, res, reslen, out);
    
    
    int rc= -1, scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    
    struct ccnl_content_s *c = 0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0;
    int num; int typ;
    dehead(&out, &len, &num, &typ);
    buf = ccnl_extract_prefix_nonce_ppkd(&out, &len, &scope, &aok, &minsfx,
			 &maxsfx, &p, &nonce, &ppkd, &content, &contlen);    
    
    c = ccnl_content_new(ccnl, &buf, &p, &ppkd, content, contlen);
    return c;
}

int ccnl_nfn_local_content_search(struct ccnl_relay_s *ccnl, struct ccnl_interest_s *i, struct ccnl_content_s *c){
    DEBUGMSG(2, "ccnl_nfn_local_content_search()\n");
    struct ccnl_content_s *c_iter; 
    
    for(c_iter = ccnl->contents; c_iter; c_iter = c_iter->next){
        unsigned char *md;
        md = i->prefix->compcnt - c_iter->name->compcnt == 1 ? compute_ccnx_digest(c_iter->pkt) : NULL;
        if(ccnl_prefix_cmp(c_iter->name, md, i->prefix, CMP_MATCH)){
            c = c_iter;
            return 1;
        }
    }
    return 0;
}

struct ccnl_interest_s *
ccnl_nfn_create_interest_object(struct ccnl_relay_s *ccnl, char *out, int len, char* name){
    DEBUGMSG(2, "ccnl_nfn_create_interest_object()\n");
    int rc= -1, scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = 0;
    int num; int typ;
    dehead(&out, &len, &num, &typ);
    buf = ccnl_extract_prefix_nonce_ppkd(&out, &len, &scope, &aok, &minsfx,
			 &maxsfx, &p, &nonce, &ppkd, &content, &contlen);
    
    struct ccnl_face_s * from = ccnl_malloc(sizeof(struct ccnl_face_s *));
    from->faceid = NFN_FACE;
    from->last_used = CCNL_NOW();
    from->outq = malloc(sizeof(struct ccnl_buf_s));
    from->outq->data[0] = strdup(name);
    from->outq->datalen = strlen(name);
    return ccnl_interest_new(ccnl, from, &buf, &p, minsfx, maxsfx, &ppkd);
}
#endif USE_UTIL
#endif //KRIVINE_COMMON_C