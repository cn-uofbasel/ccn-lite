/*
 * @f util/ccn-lite-put.c
 * @b request content: create an content object or add an existing to a relay's cache
 *
 * Copyright (C) 2013, Christian Tschudin, University of Basel
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
 * 2013-04-06  created
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>

// ----------------------------------------------------------------------
int
mkHeader(unsigned char *buf, unsigned int num, unsigned int tt)
{
    unsigned char tmp[100];
    int len = 0, i;

    *tmp = 0x80 | ((num & 0x0f) << 3) | tt;
    len = 1;
    num = num >> 4;

    while (num > 0) {
	tmp[len++] = num & 0x7f;
	num = num >> 7;
    }
    for (i = len-1; i >= 0; i--)
	*buf++ = tmp[i];
    return len;
}

int
addBlob(unsigned char *out, char *cp, int cnt)
{
    int len;

    len = mkHeader(out, cnt, CCN_TT_BLOB);
    memcpy(out+len, cp, cnt);
    len += cnt;

    return len;
}


int
mkBlob(unsigned char *out, unsigned int num, unsigned int tt,
       char *cp, int cnt)
{
    int len;

    len = mkHeader(out, num, tt);
    len += addBlob(out+len, cp, cnt);
    out[len++] = 0;

    return len;
}


int
mkStrBlob(unsigned char *out, unsigned int num, unsigned int tt,
	  char *str)
{
    return mkBlob(out, num, tt, str, strlen(str));
}

int
split_string(char *in, char c, char *out)
{
    
    int i = 0, j = 0;
    if(!in[0]) return 0;
    if(in[0] == c) ++i;
    while(in[i] != c)
    {
        if(in[i] == 0) break;
        out[j] = in[i];
        ++i; ++j;
        
    }
    out[j] = 0;
    return i;  
}

time_t
get_mtime(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        perror(path);
        exit(1);
    }
    return statbuf.st_mtime;
}

int
add_ccnl_name(unsigned char *out, char *ccn_path)
{
    char comp[256];
    int len = 0, len2 = 0;
    int h;
    memset(comp, 0 , 256);
    len += mkHeader(out + len, CCN_DTAG_NAME, CCN_TT_DTAG);
    while( (h = split_string(ccn_path + len2, '/', comp)) )
    {   
        len2 += h;
        len += mkStrBlob(out + len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, comp);
        memset(comp, 0 , 256);
    }
    out[len++] = 0;
    return len;
}

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
        printf("Error: %d\n", ERR_get_error());
    }
    RSA_free(rsa);
    return verified;
}

int add_signature(unsigned char *out, char *private_key_path, char *file, int fsize)
{
    int len, i;
    
    unsigned char sig[256];
    int sig_len;

    len = mkHeader(out, CCN_DTAG_SIGNATURE, CCN_TT_DTAG);
    len += mkStrBlob(out + len, CCN_DTAG_NAME, CCN_TT_DTAG, "SHA1");
    len += mkStrBlob(out + len, CCN_DTAG_WITNESS, CCN_TT_DTAG, "");
    
    if(!sign(private_key_path, file, fsize, sig, &sig_len)) return 0;
    printf("SIGLEN: %d\n",sig_len);
    sig[sig_len]=0;
    
    //add signaturebits bits...
    len += mkHeader(out + len, CCN_DTAG_SIGNATUREBITS, CCN_TT_DTAG);
    len += addBlob(out + len, sig, sig_len);
    out[len++] = 0; // end signaturebits
    
    out[len++] = 0; // end signature
    
    /*char *publickey = "/home/blacksheeep/.ssh/publickey.pem";
    int verified = verify(publickey, file, fsize, sig, sig_len);
    printf("Verified: %d\n", verified);*/
    
    return len;
}

int
ux_open(char *frompath)
{
  int sock, bufsize;
    struct sockaddr_un name;

    /* Create socket for sending */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
	perror("opening datagram socket");
	exit(1);
    }
    unlink(frompath);
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, frompath);
    if (bind(sock, (struct sockaddr *) &name,
	     sizeof(struct sockaddr_un))) {
	perror("binding name to datagram socket");
	exit(1);
    }
//    printf("socket -->%s\n", NAME);

    bufsize = 4 * CCNL_MAX_PACKET_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    return sock;
}

int ux_sendto(int sock, char *topath, unsigned char *data, int len)
{
    struct sockaddr_un name;
    int rc;

    /* Construct name of socket to send to. */
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, topath);

    /* Send message. */
    rc = sendto(sock, data, len, 0, (struct sockaddr*) &name,
		sizeof(struct sockaddr_un));
    if (rc < 0) {
      fprintf(stderr, "named pipe \'%s\'\n", topath);
      perror("sending datagram message");
    }
    return rc;
}

int
createCCNXFile(char *file_uri, char *ccn_path, char *private_key_path)
{
    long fsize, len, siglen;
    char *out, *file, *new_file_uri;
    FILE *f = fopen(file_uri, "r");
    if(!f) return 0;
    
    //size vom file abmessen
    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    file = (unsigned char * ) malloc(sizeof(unsigned char)*fsize);
    fread(file, fsize, 1, f);
    fclose(f);
    out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);

    
    len = mkHeader(out, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG); //content object
  
    siglen = add_signature(out + len, private_key_path, file, fsize);
    if(!siglen)
    {
        printf("Could sign message\n");
    }
    len += siglen;
    
    //FIXME: Add sign info + publisher public key, what is this?
    
    len += add_ccnl_name(out + len, ccn_path);
    
    len += mkStrBlob(out + len, CCN_DTAG_CONTENT, CCN_TT_DTAG, (char *) file);

    out[len++] = 0; //content object end
    
    //write file
    new_file_uri = (char *) malloc(sizeof(char)*1024);
    sprintf(new_file_uri, "%s.ccnb", file_uri);
    FILE *f2 = fopen(new_file_uri, "w");
    if(!f2) return 0;
    fwrite(out, 1L, len, f2);
    fclose(f2);
    free(out);
    free(file);
    
    return 1;
}

int
addToRelayCache(char *file_uri, char * socket_path, char *private_key_path)
{
    int sock, i;
    char mysockname[200];
    long len = 0, len2 = 0, len3 = 0;
    long fsize, siglen;
    char *ccnb_file, *out, *contentobj, *stmt;
    FILE *f = fopen(file_uri, "r");
    if(!f) return 0;
    
    //determine size of the file
    fseek(f, 0L, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    ccnb_file = (unsigned char *) malloc(sizeof(unsigned char)*fsize);
    fread(ccnb_file, fsize, 1, f);
    fclose(f);
   
    //Create ccn-lite-ctrl interest object with signature to add content...
    out = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 1000);
    contentobj = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 500);
    stmt = (unsigned char *) malloc(sizeof(unsigned char)*fsize + 400);
    
    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "addcacheobject");
    
    //add signature to interest...
    siglen = add_signature(stmt + len3, private_key_path, ccnb_file, fsize);
    if(!siglen)
    {
        printf("Could sign message\n");
        free(ccnb_file);
        free(out);
        free(contentobj);
        free(stmt);
        return 0;
    }
    len3 += siglen;
    
    //add content to interest...
    len3 += mkHeader(stmt+len3, CCN_DTAG_CONTENT, CCN_TT_DTAG);
    len3 += addBlob(stmt+len3, ccnb_file, fsize);
    stmt[len3++] = 0; // end content
    
    len2 += mkHeader(contentobj+len2, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    
    
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);
    
    out[len++] = 0; //name end
    out[len++] = 0; //interest end
    
    sprintf(mysockname, "/tmp/.ccn-light-ctrl-%d.sock", getpid());
    sock = ux_open(mysockname);
    printf("%s\n", socket_path);
    
    ux_sendto(sock, socket_path, out, len);
    
    free(ccnb_file);
    free(out);
    free(contentobj);
    free(stmt);
    return len;
}

int
removeFormRelayCache(char *ccn_path, char * socket_path, char *private_key_path){
    
    
    int len = 0, len2, len3;
    int sock, i;
    char mysockname[200];
    unsigned char contentobj[2000], out[2000];
    unsigned char stmt[1000];

    len = mkHeader(out, CCN_DTAG_INTEREST, CCN_TT_DTAG);   // interest
    len += mkHeader(out+len, CCN_DTAG_NAME, CCN_TT_DTAG);  // name

    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "ccnx");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "");
    len += mkStrBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG, "removecacheobject");

    // prepare debug statement
    //len3 = mkHeader(stmt, CCNL_DTAG_DEBUGREQUEST, CCN_TT_DTAG);
    len3 += add_ccnl_name(stmt+len3, ccn_path);
    
    //stmt[len3++] = 0; // end-of-debugstmt

    // prepare CONTENTOBJ with CONTENT
    len2 = mkHeader(contentobj, CCN_DTAG_CONTENTOBJ, CCN_TT_DTAG);   // contentobj
    len2 += add_signature(contentobj+len2, private_key_path, stmt, len3);
    len2 += mkBlob(contentobj+len2, CCN_DTAG_CONTENT, CCN_TT_DTAG,  // content
		   (char*) stmt, len3);
    contentobj[len2++] = 0; // end-of-contentobj

    
    
    // add CONTENTOBJ as the final name component
    len += mkBlob(out+len, CCN_DTAG_COMPONENT, CCN_TT_DTAG,  // comp
		  (char*) contentobj, len2);

    out[len++] = 0; // end-of-name
    out[len++] = 0; // end-of-interest

//    ccnl_prefix_free(p);
    sprintf(mysockname, "/tmp/.ccn-light-ctrl-%d.sock", getpid());
    sock = ux_open(mysockname);
    printf("%s\n", socket_path);
    
    ux_sendto(sock, socket_path, out, len);
    
    return len;
 
}

// ----------------------------------------------------------------------

int
main(int argc, char *argv[])
{
    int create = 0;
    
    char *unix_socket_path;
    char *file_uri;
    char *ccn_path;
    char *private_key_path;
    
    if(!strcmp(argv[1], "create"))
    {
        if(argc < 5) goto Usage;
        create = 1;
        file_uri = argv[2];
        ccn_path = argv[3];
        private_key_path = argv[4];
        if(!createCCNXFile(file_uri, ccn_path, private_key_path)) goto Usage;
    }
    else if(!strcmp(argv[1], "add"))
    {
        if(argc < 5) goto Usage;
        file_uri = argv[2];  
        unix_socket_path = argv[3];
        private_key_path = argv[4];
        if(!addToRelayCache(file_uri, unix_socket_path, private_key_path)) goto Usage;
    }
    else if(!strcmp(argv[1], "remove"))
    {
        if(argc < 5) goto Usage;
        ccn_path = argv[2];  
        unix_socket_path = argv[3];
        private_key_path = argv[4];
        if(!removeFormRelayCache(ccn_path, unix_socket_path, private_key_path)) goto Usage;
    }
       
    /*//create socket    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { perror("cannot create socket"); return 0; }
    struct sockaddr_in outaddr; 
    outaddr.sin_family = AF_INET;
    outaddr.sin_port = 9000;
    if (bind(sock, (struct sockaddr *)&outaddr, sizeof(outaddr)) < 0) { perror("bind failed"); return 0; }
    
    //send packet
    struct sockaddr_in relayaddr; 
    memset((char *)&relayaddr, 0, sizeof(relayaddr));
    relayaddr.sin_family = AF_INET;
    relayaddr.sin_addr.s_addr = inet_addr(ip_address);
    relayaddr.sin_port = port;
    sendto(sock, out, len, 0, (struct sockaddr *)&relayaddr, sizeof(relayaddr));*/
    
    return 0;
    
Usage:
    fprintf(stderr, "usage: \n" 
    " %s create file_uri ccn_path private_key_path\n"
    " %s add file_uri unix_socket_path private_key_path\n"
    " %s remove ccn_path unix_socket_path private_key_path\n",
    argv[0], argv[0], argv[0]);
    exit(1);
    return 0; // avoid a compiler warning
}

// eof
