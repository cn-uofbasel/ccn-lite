// aux header file for the fragment utilities

#include <stdlib.h>
#include <string.h>

struct ccnl_buf_s {
    struct ccnl_buf_s *next;
    unsigned int datalen;
    unsigned char data[1];
};

struct ccnl_frag_s { // fragment protocol control block (per face)
    int protocol; // 0=plain CCNx (no fragments), 1=v20130705
    // int state;    // (face ctrl protocol)
    // int encoding; // 0=ccnb, 1=TLV, etc

    int mtu;
    struct ccnl_buf_s *bigpkt;
    unsigned int sendoffs;
    struct ccnl_buf_s *defrag; // incoming queue

    // protocol v20130705:
    unsigned char flagswidth;
    unsigned int sendseq;
    unsigned char sendseqwidth;
    unsigned int recvseq;
    unsigned char recvseqwidth;

    // ccn-lite extensions:
    unsigned int losscount;
    unsigned char losscountwidth;
};

#define ccnl_free(p) free(p)

struct ccnl_buf_s*
ccnl_buf_new(void *data, int len)
{
    struct ccnl_buf_s *b;

    b = (struct ccnl_buf_s *) malloc(sizeof(*b) + len);
    if (!b)
	return NULL;
    b->next = NULL;
    b->datalen = len;
    if (data)
	memcpy(b->data, data, len);
    return b;
}

// eof
