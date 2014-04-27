#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "ccnl-core.h"

#define CCNL_MAX_PACKET_SIZE 8192

static char 
encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void 
build_decoding_table() {

    decoding_table = malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


char 
*base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char 
*base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

void 
base64_cleanup() {
    free(decoding_table);
}

int 
create_packet_log(/*long fromip, int fromport,*/ char* toip, int toport,
	struct ccnl_prefix_s *prefix, char *data, int datalen, char *res)
{
    char name[CCNL_MAX_PACKET_SIZE];
    int len = 0;
    for(int i = 0; i < prefix->compcnt; ++i){
        len += sprintf(name+len, "/%s", prefix->comp[i]); 
    }
    
    len = 0;
    len += sprintf(res + len, "{\n");
    len += sprintf(res + len, "\"packetLog\":{\n");

//	len += sprintf(res + len, "\"from\":{\n");
//	len += sprintf(res + len, "\"host\": %d,\n", fromip);
//	len += sprintf(res + len, "\"port\": %d\n", fromport);
//	len += sprintf(res + len, "\"type\": \"NFNNode\", \n");
//	len += sprintf(res + len, "\"prefix\": \"docrepo1\"\n");
//	len += sprintf(res + len, "},\n"); //from

    len += sprintf(res + len, "\"to\":{\n");
    len += sprintf(res + len, "\"host\": \"%s\",\n", toip);
    len += sprintf(res + len, "\"port\": %d\n", toport);
    len += sprintf(res + len, "},\n"); //to

    len += sprintf(res + len, "\"isSent\": %s,\n", "true");

    len += sprintf(res + len, "\"packet\":{\n");
    len += sprintf(res + len, "\"type\": \"%s\",\n", data != NULL ? "content" : "interest" );
    len += sprintf(res + len, "\"name\": \"%s\"\n", name);

    if(data){
            size_t newlen;
            char *newdata = base64_encode(data, datalen, &newlen);

            len += sprintf(res + len, ",\"data\": \"%s\"\n", data);
    }
    len += sprintf(res + len, "}\n"); //packet

    len += sprintf(res + len, "}\n"); //packetlog
    len += sprintf(res + len, "}\n");

    return len;
}

int udp_sendto(int sock, char *address, int port, char *data, int len){
    int rc;
    struct sockaddr_in si_other;
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);
    if (inet_aton(address, &si_other.sin_addr)==0) {
          fprintf(stderr, "inet_aton() failed\n");
          exit(1);
    }
    rc = sendto(sock, data, len, 0, &si_other, sizeof(si_other));
    return rc;
}

int 
sendtomonitor(char *content, int contentlen){
    char *address = "127.0.0.1";
    int port = 10666;
    int s = socket(PF_INET, SOCK_DGRAM, 0);
    return udp_sendto(s, address, port, content, contentlen); 
}