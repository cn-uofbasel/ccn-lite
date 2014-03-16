<<<<<<< HEAD
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
=======
#include <stdlib.h>
#include <stdio.h>
>>>>>>> 77d83cd... Add utility to create compute requests

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../ccnx.h"
#include "../ccnl.h"

#include "ccnl-common.c"
#include "../krivine-common.c"

<<<<<<< HEAD
#include "ccnl-socket.c"


int main(int argc, char **argv){
    
    unsigned char out[CCNL_MAX_PACKET_SIZE];
    int i = 0, len, opt, sock = 0;
    char *namecomp[CCNL_MAX_NAME_COMP], *cp, *dest, *comp;
    char *udp = "127.0.0.1/9695", *ux = NULL;
    float wait = 3.0;
    int (*sendproc)(int,char*,unsigned char*,int);
    int print = 0;
    int thunk_request = 0;
    
    while ((opt = getopt(argc, argv, "hptu:w:x:")) != -1) {
        switch (opt) {
        case 'u':
	    udp = optarg;
	    break;
        case 'w':
	    wait = atof(optarg);
	    break;
        case 'x':
	    ux = optarg;
	    break;
        case 'p':
            print = 1;
            break;
        case 't':
            thunk_request = 1;
            break;
        case 'h':
        default:
Usage:
	    fprintf(stderr, "usage: %s "
	    "[-u host/port] [-x ux_path_name] [-w timeout] COMPUTATION URI\n"
	    "  -u a.b.c.d/port  UDP destination (default is 127.0.0.1/9695)\n"
	    "  -x ux_path_name  UNIX IPC: use this instead of UDP\n"
	    "  -w timeout       in sec (float)\n"
            "  -p               print interest on console and exit\n",
	    argv[0]);
	    exit(1);
        }
    }
    if (!argv[optind]) 
	goto Usage;  
    comp = argv[optind++];
    if (!argv[optind]) 
	goto Usage;  
    
    int j= splitComponents(argv[optind], namecomp);   
    len = mkInterestCompute(namecomp, comp, strlen(comp), thunk_request, out);

    if(print){
        fwrite(out, sizeof(char), len, stdout);
        return 0;
    }
    
    if (ux) { // use UNIX socket
	dest = ux;
	sock = ux_open();
	sendproc = ux_sendto;
    } else { // UDP
	dest = udp;
	sock = udp_open();
	sendproc = udp_sendto;
    }
    request_content(sock, sendproc, dest, out, len, wait);
=======

int main(int argc, char **argv){
    
    char out[CCNL_MAX_PACKET_SIZE];
    
    char *namecomp[CCNL_MAX_NAME_COMP];
    
    int i, j= splitComponents(argv[1], namecomp);
    
    //for(i = 0; i < j; ++i)
        //printf("%s\n", namecomp[i]);
    
    int len = mkInterestCompute(namecomp, argv[2], strlen(argv[2]), 0, out);
   // printf("len: %d\n", len);
    
    fwrite(out, sizeof(char), len, stdout);

>>>>>>> 77d83cd... Add utility to create compute requests
    return 0;
}
