#include <stdlib.h>
#include <stdio.h>

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


int main(int argc, char **argv){
    
    char out[CCNL_MAX_PACKET_SIZE];
    
    char *namecomp[CCNL_MAX_NAME_COMP];
    
    int i, j= splitComponents(argv[1], namecomp);
    
    //for(i = 0; i < j; ++i)
        //printf("%s\n", namecomp[i]);
    
    int len = mkInterestCompute(namecomp, argv[2], strlen(argv[2]), 0, out);
   // printf("len: %d\n", len);
    
    fwrite(out, sizeof(char), len, stdout);

    return 0;
}
