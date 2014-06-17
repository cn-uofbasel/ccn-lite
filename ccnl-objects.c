#ifndef CCNL_OBJECTS_C
#define CCNL_OBJECTS_C


#include "pkt-ccnb-enc.c"
#include "pkt-ndntlv-enc.c"

static int
mkContent(char **namecomp, char *data, int datalen, unsigned char *out);

#ifdef CCNL_NFN
struct ccnl_interest_s *
mkInterestObject(struct ccnl_relay_s *ccnl, struct configuration_s *config,
                 struct ccnl_prefix_s *prefix)
{

    DEBUGMSG(2, "mkInterestObject()\n");
    int scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen, mbf=0, len, typ, num, i;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *out = malloc(CCNL_MAX_PACKET_SIZE);
    unsigned char *content;


    struct ccnl_face_s * from = ccnl_malloc(sizeof(struct ccnl_face_s *));
    from->faceid = config->configid;
    from->last_used = CCNL_NOW();
    from->outq = malloc(sizeof(struct ccnl_buf_s));
    from->outq->data[0] = strdup((char *)prefix->comp[0]);
    from->outq->datalen = strlen((char *)prefix->comp[0]);

    char *namecomps[CCNL_MAX_NAME_COMP];
    namecomps[prefix->compcnt] = 0;
    for(i = 0; i < prefix->compcnt; ++i){
        namecomps[i] = strdup((char *)prefix->comp[i]);
    }

    if(config->suite == CCNL_SUITE_CCNB){

        len = mkInterest(namecomps, NULL, out);
        if(dehead(&out, &len, &num, &typ)){
            return 0;
        }
        buf = ccnl_ccnb_extract(&out, &len, &scope, &aok, &minsfx, &maxsfx,
                                &p, &nonce, &ppkd, &content, &contlen);
        return ccnl_interest_new(ccnl, from, CCNL_SUITE_CCNB, &buf, &p, minsfx, maxsfx);
    }
    else if(config->suite == CCNL_SUITE_CCNTLV){
        //NOT YET IMPLEMETED
        return -1;
    }
    else if(config->suite == CCNL_SUITE_NDNTLV){
       int tmplen = CCNL_MAX_PACKET_SIZE;
       len = ccnl_ndntlv_mkInterest(namecomps, -1, &tmplen, out);
       memmove(out, out + tmplen, CCNL_MAX_PACKET_SIZE - tmplen);
       len = CCNL_MAX_PACKET_SIZE - tmplen;
       unsigned char *cp = out;
       if(ccnl_ndntlv_dehead(&out, &len, &typ, &num)){
           return 0;
       }
       buf = ccnl_ndntlv_extract(out - cp, &out, &len, &scope, &mbf, &minsfx, &maxsfx,
                                 &p, &nonce, &ppkd, &content, &contlen);
       return ccnl_interest_new(ccnl, from, CCNL_SUITE_NDNTLV, &buf, &p, minsfx, maxsfx);
    }
    return 0;
}
#endif

struct ccnl_content_s *
create_content_object(struct ccnl_relay_s *ccnl, struct ccnl_prefix_s *prefix,
        unsigned char *contentstr, int contentlen, int suite){
    DEBUGMSG(49, "create_content_object()\n");
    int i = 0;
    int scope=3, aok=3, minsfx=0, maxsfx=CCNL_MAX_NAME_COMP, contlen, len, mbf = 0;
    struct ccnl_buf_s *buf = 0, *nonce=0, *ppkd=0;
    struct ccnl_prefix_s *p = 0;
    unsigned char *content = malloc(CCNL_MAX_PACKET_SIZE);
    int num; int typ;
    unsigned char *out = malloc(CCNL_MAX_PACKET_SIZE);
    memset(out, 0, CCNL_MAX_PACKET_SIZE);

    char **prefixcomps = ccnl_malloc(sizeof(char *) * prefix->compcnt+1);

    prefixcomps[prefix->compcnt] = 0;

    for(i = 0; i < prefix->compcnt; ++i)
    {
        prefixcomps[i] = strdup((char *)prefix->comp[i]);
    }

    if(suite == CCNL_SUITE_CCNB){
        len = mkContent(prefixcomps, contentstr, contentlen, out);
        if(dehead(&out, &len, &num, &typ)){
            return NULL;
        }
        buf = ccnl_ccnb_extract(&out, &len, &scope, &aok, &minsfx, &maxsfx,
                              &p, &nonce, &ppkd, &content, &contlen);
        return ccnl_content_new(ccnl, CCNL_SUITE_CCNB, &buf, &p, &ppkd, content, contlen);
    }
    else if(suite == CCNL_SUITE_CCNTLV){
        // net yet implemeted;
        return 0;
    }
    else if(suite == CCNL_SUITE_NDNTLV){
        int len2 = CCNL_MAX_PACKET_SIZE;
        len = ccnl_ndntlv_mkContent(prefixcomps, contentstr, contentlen, &len2, out);
        memmove(out, out+len2, CCNL_MAX_PACKET_SIZE - len2);
        len = CCNL_MAX_PACKET_SIZE - len2;

        unsigned char *cp = out;
        if(ccnl_ndntlv_dehead(&out, &len, &typ, &num)){
            return NULL;
        }
        buf = ccnl_ndntlv_extract(out - cp, &out, &len, &scope, &mbf, &minsfx, &maxsfx,
                                  &p, &nonce, &ppkd, &content, &contlen);
        return ccnl_content_new(ccnl, CCNL_SUITE_NDNTLV, &buf, &p, &ppkd, content, contlen);
    }
    return 0;
}
#endif
