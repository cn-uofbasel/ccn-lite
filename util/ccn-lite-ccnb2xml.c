/*
 * @f util/ccn-lite-ccnb2xml.c
 * @b pretty print CCNB content to XML
 *
 * Copyright (C) 2013, Christopher Scherb, University of Basel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2012-07-01  created
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include "ccnl-common.c"

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>

char *ctrl_public_key = 0;

int sha1(void* input, unsigned long length, unsigned char* md)
{
    SHA_CTX context;
    if(!SHA1_Init(&context))
        return 0;

    if(!SHA1_Update(&context, (unsigned char*)input, length))
        return 0;

    if(!SHA1_Final(md, &context))
        return 0;

    return 1;
}

int sign(char* private_key_path, char *msg, int msg_len, char *sig, int *sig_len)
{

    //Load private key
    FILE *fp = fopen(private_key_path, "r");
    if(!fp) {
        printf("Could not find private key\n");
        return 0;
    }
    RSA *rsa = (RSA *) PEM_read_RSAPrivateKey(fp,NULL,NULL,NULL);
    fclose(fp);
    if(!rsa) return 0;
    
    unsigned char md[SHA_DIGEST_LENGTH];
    sha1(msg, msg_len, md);
    
    //Compute signatur
    int err = RSA_sign(NID_sha1, md, SHA_DIGEST_LENGTH, sig, sig_len, rsa);
    if(!err){
        printf("Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return err;
}

int verify(char* public_key_path, char *msg, int msg_len, char *sig, int sig_len)
{
    //Load public key
    FILE *fp = fopen(public_key_path, "r");
    if(!fp) {
        printf("Could not find public key\n");
        return 0;
    }
    RSA *rsa = (RSA *) PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if(!rsa) return 0;
    fclose(fp);
  
    //Compute Hash
    unsigned char md[SHA_DIGEST_LENGTH];
    sha1(msg, msg_len, md);
    
    //Verify signature
    int verified = RSA_verify(NID_sha1, md, SHA_DIGEST_LENGTH, sig, sig_len, rsa);
    if(!verified){
        //printf("Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return verified;
}

const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

void
print_offset(int offset){
    int it;
    for(it = 0; it < offset; ++it)
        printf(" ");
}

int 
print_tag_content(unsigned char **buf, int *len, FILE *stream){
    while((**buf) !=  0)
    {
        printf("%c", **buf);
        ++(*buf); --(*len);
    }
    ++(*buf); --(*len);
    return 0;
}

int print_tag_content_with_tag(unsigned char **buf, int *len, char *tag, FILE *stream)
{
    int num, typ; 
    printf("<%s>", tag);
    dehead(buf, len, &num, &typ);
    print_tag_content(buf, len, stream);
    printf("</%s>\n", tag);
    return 0;
}

int 
handle_ccn_content(unsigned char **buf, int *len, int offset, FILE *stream);


int
handle_ccn_debugreply_content(unsigned char **buf, int *len, int offset, char* tag, FILE *stream)
{
    int num, typ;
    print_offset(offset); printf("<%s>\n", tag);
    while(1)
    {
        if(dehead(buf, len, &num, &typ)) return -1;
        switch(num)
        {
            case CCNL_DTAG_IFNDX:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "IFNDX", stream);
               break;
            case CCNL_DTAG_ADDRESS:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "ADDRESS", stream);
               break;
            case CCNL_DTAG_ETH:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "ETH", stream);
               break;
            case CCNL_DTAG_SOCK:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "SOCK", stream);
               break;
            case CCNL_DTAG_REFLECT:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "REFLECT", stream);
               break;
            case CCN_DTAG_FACEID:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "FACEID", stream);
               break;   
            case CCNL_DTAG_NEXT:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "NEXT", stream);
               break;
            case CCNL_DTAG_PREV:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "PREV", stream);
               break;
            case CCNL_DTAG_FWD:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "FWD", stream);
               break;
            case CCNL_DTAG_FACEFLAGS:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "FACEFLAGS", stream);
               break;
            case CCNL_DTAG_IP:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "IP", stream);
               break;
            case CCNL_DTAG_UNIX:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "UNIX", stream);
               break;
            case CCNL_DTAG_PEER:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "PEER", stream);
               break;
            case CCNL_DTAG_FACE:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "FACE", stream);
               break;
            case CCNL_DTAG_INTERESTPTR:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "INTERESTPTR", stream);
               break;
            case CCNL_DTAG_LAST:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "LAST", stream);
               break;
            case CCNL_DTAG_MIN:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "MIN", stream);
               break;
            case CCNL_DTAG_MAX:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "MAX", stream);
               break;
            case CCNL_DTAG_RETRIES:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "RETRIES", stream);
               break;
            case CCNL_DTAG_PUBLISHER:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "PUBLISHER", stream);
               break;
            case CCNL_DTAG_CONTENTPTR:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "CONTENTPTR", stream);
               break;
            case CCNL_DTAG_LASTUSE:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "LASTUSE", stream);
               break;
            case CCNL_DTAG_SERVEDCTN:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "SERVEDCTN", stream);
               break;
            case CCN_DTAG_ACTION:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "ACTION", stream);
               break;
            case CCNL_DTAG_MACSRC:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "MACSRC", stream);
               break;
            case CCNL_DTAG_IP4SRC:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "IP4SRC", stream);
               break;
            case CCN_DTAG_IPPROTO:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "IPPROTO", stream);
               break;
            case CCN_DTAG_HOST:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "HOST", stream);
               break;
            case CCN_DTAG_PORT:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "PORT", stream);
               break;
            case CCNL_DTAG_FRAG:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "FRAG", stream);
               break;
            case CCNL_DTAG_MTU:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "MTU", stream);
               break;
            case CCNL_DTAG_DEVFLAGS:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "DEVFLAGS", stream);
               break;
            case CCNL_DTAG_DEVNAME:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "DEVNAME", stream);
               break;
            case CCN_DTAG_NAME:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "NAME", stream);
               break;
            case CCNL_DTAG_PREFIX:
               print_offset(offset+4); 
               print_tag_content_with_tag(buf, len, "PREFIX", stream);
               break;
            default: 
              goto Bail;
        }
    }
    Bail:
    print_offset(offset); printf("</%s>\n", tag);
    return 0;
}   

int 
handle_ccn_debugreply(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
   
    print_offset(offset); printf("<DEBUGREPLY>\n");
    
    while(1)
    {
        if(dehead(buf, len, &num, &typ)) return -1;
        switch(num)
        {
            case CCNL_DTAG_INTERFACE:
                handle_ccn_debugreply_content(buf, len, offset+4, "INTERFACE", stream);
                break;
            case CCN_DTAG_FACEINSTANCE:
                handle_ccn_debugreply_content(buf, len, offset+4, "FACEINSTANCE", stream);
                break;
            case CCN_DTAG_FWDINGENTRY:
                handle_ccn_debugreply_content(buf, len, offset+4, "FWDINGENTRY", stream);
                break;
            case CCN_DTAG_INTEREST:
                handle_ccn_debugreply_content(buf, len, offset+4, "INTEREST", stream);
                break;
            case CCN_DTAG_CONTENT:
                handle_ccn_debugreply_content(buf, len, offset+4, "CONTENT", stream);
                break;
            default:
                goto Bail;
        }
    }
    Bail:
    print_offset(offset); printf("</DEBUGREPLY>\n"); 
    return 0;
}

int 
handle_ccn_debugaction(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<DEBUGACTION>");
    print_tag_content(buf,len,stream);
    printf("</DEBUGACTION>\n");
    return 0;
}

int 
handle_ccn_action(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<ACTION>");
    print_tag_content(buf,len,stream);
    printf("</ACTION>\n");
    return 0;
}

int
handle_ccn_debugrequest(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    
    print_offset(offset); printf("<DEBUGREQUEST>\n");
    while(1)
    {
        if(dehead(buf, len, &num, &typ)) goto Bail;
        switch(num)
        {
            case CCN_DTAG_ACTION:
                handle_ccn_action(buf, len, offset+4, stream);
                break;
            case CCNL_DTAG_DEBUGACTION:
                handle_ccn_debugaction(buf, len, offset+4, stream);
                break;
            default:
                goto Bail;
        }
    }
    Bail:
    print_offset(offset); printf("</DEBUGREQUEST>\n");
    return 0;
}

int 
handle_ccn_signature(unsigned char **buf, int *buflen, int offset, FILE *stream)
{
   int num, typ, verified, siglen;
   char *sigtype = 0, *sig = 0; 
   while (dehead(buf, buflen, &num, &typ) == 0) {
        
        if (num==0 && typ==0)
	    break; // end
        
        extractStr2(sigtype, CCN_DTAG_NAME);
        siglen = *buflen;
        extractStr2(sig, CCN_DTAG_SIGNATUREBITS);
        
        if (consume(typ, num, buf, buflen, 0, 0) < 0) goto Bail;
    }
    siglen = siglen-((*buflen)+4);
    if(ctrl_public_key)
    {
        char *buf2 = *buf;
        int buflen2 = *buflen;

        if (dehead(&buf2, &buflen2, &num, &typ) != 0) goto Bail;
        if (typ != CCN_TT_DTAG || num != CCN_DTAG_CONTENT) goto Bail;

        if (dehead(&buf2, &buflen2, &num, &typ) != 0) goto Bail;
        if (typ != CCN_TT_BLOB) goto Bail;

        verified = verify(ctrl_public_key, buf2, buflen2 - 5, sig, siglen);

        print_offset(offset); 
        printf("<SIGNATURE>");
        if(!verified) {
            printf("Signature NOT verified");
        }else
        {
            printf("Signature verified");
        }
        printf("</SIGNATURE>\n");
    }
    else{
        print_offset(offset); 
        printf("<SIGNATURE>\n");
        print_offset(offset+4); printf("<NAME>%s</NAME>\n", sigtype);
        print_offset(offset+4); printf("<SIGNATUREBITS>%s</SIGNATUREBITS>\n", sig);
        printf("</SIGNATURE>\n");
    }
    Bail:
    
    return 0;
}

int
handle_ccn_content_obj(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    print_offset(offset); printf("<CONTENTOBJ>\n");
    while(1)
    {
        if(dehead(buf, len, &num, &typ)) goto Bail;
        switch(num)
        {
            case CCN_DTAG_CONTENT:
                handle_ccn_content(buf, len, offset+4, stream);
                break;
            case CCN_DTAG_SIGNATURE: 
                handle_ccn_signature(buf, len, offset+4, stream);
                break;
            default:
                goto Bail;
        }
    }
    Bail:
    print_offset(offset); printf("</CONTENTOBJ>\n");
    return 0;
}

int 
handle_ccn_component_content(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    print_offset(offset);
    printf("<COMPONENT>");
    if(dehead(buf, len, &num, &typ)) return -1;
    print_tag_content(buf,len,stream);
    printf("</COMPONENT>\n");
    return 0;
}

int 
handle_ccn_component_tag(unsigned char **buf, int *len, int offset, FILE *stream)
{
    int num, typ;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<COMPONENT>\n");
    while(typ != 2){
        dehead(buf, len, &num, &typ);
    }
    switch(num)
    {
        case CCN_DTAG_CONTENTOBJ:
            handle_ccn_content_obj(buf, len, offset+4, stream);
            break;
    }
    print_offset(offset); printf("</COMPONENT>\n");
    return 0;
}

int 
handle_ccn_name(unsigned char **buf, int *len, int offset, FILE *stream){
    int num, typ, i;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<NAME>\n");
    for(i = 0; i < 3; ++i)
    {
        if(num != CCN_DTAG_COMPONENT) return -1;
        handle_ccn_component_content(buf, len, offset+4, stream);
        if(dehead(buf, len, &num, &typ)) return -1;
        
    }
    handle_ccn_component_tag(buf, len, offset+4, stream);
    print_offset(offset); printf("</NAME>\n");
    return 0;
}

int 
handle_ccn_content(unsigned char **buf, int *len, int offset, FILE *stream){
    int num, typ;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<CONTENT>\n");
    while(typ != 2){
        dehead(buf, len, &num, &typ);
    }
    while(1)
    {
        switch(num)
        {
            case CCN_DTAG_NAME:
                handle_ccn_name(buf, len, offset+4, stream);
                break;
            case CCN_DTAG_INTEREST:
                break;
            case CCNL_DTAG_DEBUGREQUEST:
                handle_ccn_debugrequest(buf, len, offset+4, stream);
                break;
            case CCNL_DTAG_DEBUGREPLY:
                handle_ccn_debugreply(buf, len, offset+4, stream);
                break;
            case CCN_DTAG_FACEINSTANCE:
                handle_ccn_debugreply_content(buf, len, offset+4, "FACEINSTANCE", stream);
                break;
            case CCNL_DTAG_DEVINSTANCE:
                handle_ccn_debugreply_content(buf, len, offset+4, "DEVINSTANCE", stream);
                break;
            case CCNL_DTAG_PREFIX:
                handle_ccn_debugreply_content(buf, len, offset+4, "PREFIX", stream);
                break;
            case CCN_DTAG_ACTION:
                print_offset(offset + 4); print_tag_content_with_tag(buf, len, "ACTION", stream);
                break;
            default:
                //printf("%i,%i\n", num, typ);
                goto Bail;
                break;
        }
        if(dehead(buf, len, &num, &typ)) break;
    }
    Bail:
    print_offset(offset); printf("</CONTENT>\n");
    return 0;
}


int  
handle_ccn_packet(unsigned char *buf, int len, int offset, FILE *stream){
    
    int num, typ;
    if(dehead(&buf, &len, &num, &typ)) return -1;
    switch(num)
    {
        case CCN_DTAG_CONTENT:
            return handle_ccn_content(&buf, &len, offset, stream);
            break;
        case CCN_DTAG_INTEREST:
            break;
    }
    return 0;
}

int
main(int argc, char *argv[])
{

    if(argc > 2 && !strcmp(argv[1],"-k"))  {
        ctrl_public_key = argv[2];
    }else if(argc > 1 && !strcmp(argv[1],"-h")){
        goto usage;
    }

    unsigned char out[64000];
    int len;

    len = read(0, out, sizeof(out));
    if (len < 0) {
	perror("read");
	exit(-1);
    }

    handle_ccn_packet(out, len, 0, stdout);
    printf("\n");
    
    return 0;
    
    usage:
    fprintf(stderr, "usage: \n" 
    " parse ccn-lite-ctrl/ccnl-ext-mgmt messages and shows it as xml\n"
    " %s -k public_key_path to verify signatures\n"
    " %s -h print this message\n",
    argv[0], argv[0]);
    return 0;
}
