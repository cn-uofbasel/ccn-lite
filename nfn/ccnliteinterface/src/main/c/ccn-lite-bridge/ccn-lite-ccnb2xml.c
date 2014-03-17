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
#include <openssl/obj_mac.h>

#include "../../../../../../ccnx.h"
#include "../../../../../../ccnl.h"

#include "ccnl-common.c"

#define USE_SIGNATURES
// #include "ccnl-crypto.c"


char *ctrl_public_key = 0;

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

#ifdef USE_SIGNATURES
int
handle_ccn_signature(unsigned char **buf, int *buflen, int offset, FILE *stream)
{
   int num, typ, i;
   if(dehead(buf, buflen, &num, &typ)) return -1;
   if (typ != CCN_TT_BLOB) return 0;

   print_offset(offset);
   printf("<SIGNATURE>");
   for(i = 0; i < num; ++i){
       printf("%c",(*buf)[i]);
   }

   printf("</SIGNATURE>\n");

   *buflen -= (num+1);
   *buf += (num+1);


    return 0;
}
#endif /*USE_SIGNATURES*/

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
handle_ccn_content_obj(unsigned char **buf, int *len, int offset, FILE *stream){
    int num, typ;
    if(dehead(buf, len, &num, &typ)) return -1;
    print_offset(offset); printf("<CONTENTOBJ>\n");
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
            case CCN_DTAG_SIGNATURE:
                handle_ccn_signature(buf, len, offset+4, stream);
                break;
            case CCN_DTAG_CONTENT:
                handle_ccn_content(buf, len, offset+4, stream);
                break;
            default:
                goto Bail;
                break;
        }
        if(dehead(buf, len, &num, &typ)) break;
    }
    Bail:
    print_offset(offset); printf("</CONTENTOBJ>\n");
    return 0;
}


int
handle_ccn_packet(unsigned char *buf, int len, int offset, FILE *stream){

    int num, typ;
    if(dehead(&buf, &len, &num, &typ)) return -1;
    switch(num)
    {
        case CCN_DTAG_CONTENTOBJ:
            return handle_ccn_content_obj(&buf, &len, offset, stream);
            break;
        case CCN_DTAG_INTEREST:
            break;
    }
    return 0;
}

// int
// main(int argc, char *argv[])
// {

//     if(argc > 1 && !strcmp(argv[1],"-h")){
//         goto usage;
//     }

//     unsigned char out[64000];
//     int len;

//     len = read(0, out, sizeof(out));
//     if (len < 0) {
// 	perror("read");
// 	exit(-1);
//     }
//     handle_ccn_packet(out, len, 0, stdout);
//     printf("\n");

//     return 0;

//     usage:
//     fprintf(stderr, "usage: \n"
//     " Parse ccn-lite-ctrl/ccnl-ext-mgmt messages and shows it as xml.\n"
//     " To check the status of the signatures, use the ccn-lite-ctrl with publickey.\n"
//     " %s -h print this message.\n",
//     argv[0], argv[0]);
//     return 0;
// }
