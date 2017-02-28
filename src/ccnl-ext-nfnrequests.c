/*
 * @f ccnl-ext-nfncommon.c
 * @b CCN-lite, execution/state management of running computations
 *
 * Copyright (C) 2016, Balazs Faludi, University of Basel
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
 * 2016-02-10 created
 */


#ifdef USE_NFN_REQUESTS

#include <wordexp.h>

enum nfn_request_type {
    NFN_REQUEST_TYPE_UNKNOWN = 0,
    NFN_REQUEST_TYPE_START,
    NFN_REQUEST_TYPE_PAUSE,
    NFN_REQUEST_TYPE_RESUME,
    NFN_REQUEST_TYPE_CANCEL,
    NFN_REQUEST_TYPE_STATUS,
    NFN_REQUEST_TYPE_KEEPALIVE,
    NFN_REQUEST_TYPE_COUNT_INTERMEDIATES,
    NFN_REQUEST_TYPE_GET_INTERMEDIATE,
    NFN_REQUEST_TYPE_MAX = NFN_REQUEST_TYPE_GET_INTERMEDIATE,
};

char *nfn_request_names[NFN_REQUEST_TYPE_MAX] = {
    "START",
    "PAUSE",
    "RESUME",
    "CANCEL",
    "STATUS",
    "KEEPALIVE",
    "CIM",
    "GIM"};

struct nfn_request_s {
    unsigned char *comp;
    int complen;
    enum nfn_request_type type;
    
    unsigned char *arg;
    int arglen;
    // int argc;
    // unsigned char **argv;
    // int *argslen;
    // unsigned char **args;
};

struct nfn_request_s*
nfn_request_new(unsigned char *comp, int complen)
{
    struct nfn_request_s *request = 
        (struct nfn_request_s *) ccnl_calloc(1, sizeof(struct nfn_request_s));

    DEBUGMSG_CORE(TRACE, "nfn_request_new(%.*s)\n", complen, comp);

    if (!request)
        return NULL;

    request->complen = complen;
    request->comp = ccnl_malloc(complen);
    memcpy(request->comp, comp, complen);

    request->type = NFN_REQUEST_TYPE_UNKNOWN;
    char *request_name = NULL;
    int request_len = 0;

    int i;
    for (i = 0; i < NFN_REQUEST_TYPE_MAX; i++) {
        request_name = nfn_request_names[i];
        request_len = strlen(request_name);
        if (request_len <= complen && strncmp(request_name, (char *)comp, request_len) == 0) {
            request->type = (enum nfn_request_type)i+1; // 0 is "unknown"
            break;
        }
    }

    if (request->type == NFN_REQUEST_TYPE_UNKNOWN) {
        DEBUGMSG_CORE(DEBUG, "Unknown request: %.*s\n", complen, comp);
        return request;
    }
 
    // request->argscount = 0;
    // for (i = request_len; i < complen; i++) {
    //     char c = (char)comp[i];
    //     if (c == '\0') {
    //     } else {   
    //     }
    // }

    if (complen >= request_len + 2 && comp[request_len] == '\0') {
        request->arglen = complen - request_len - 1;
        request->arg = request->comp + request_len + 1;
    }
    
    // char state = 'T';
    // unsigned char *cursor = comp + request_len;
    // unsigned char *buf = malloc(1024 * sizeof(unsigned char));
    // int len = 0;
    // while (cursor < comp + complen) {
    //     switch (state) {
    //         // case ',':   // skipping delimiters, whitespace
    //         //     if (*cursor == ',' || *cursor == ' ' || *cursor == '\t') {
                    
    //         //     } else if (*cursor == '\"') {
    //         //         start = cursor + 1;
    //         //         state = 'Q';
    //         //     } else {
    //         //         start = cursor;
    //         //         state = 'T';
    //         //     }
    //         //     break;
    //         case 'T':   // scanning text
    //             if (*cursor == ',' && len > 0) {
    //                 //end
    //             } else if (*cursor == '\"') {
    //                 state = 'Q';
    //             } else {
    //                 start = cursor;
    //                 state = 'T';
    //             }
    //             break;
    //         case 'Q':   // scanning quoted text
    //     }
    // }

    // request->argscount = 0;

    return request;
}

void 
nfn_request_free(struct nfn_request_s *request) {
    if (request) {
        if (request->comp) {
            ccnl_free(request->comp);
        }
        ccnl_free(request);
    }
}

char *
nfn_request_description_new(struct nfn_request_s* request)
{
    int len = 0;
    char *buf = (char*) ccnl_malloc(256);
    
    if (request == NULL) {
        len += sprintf(buf + len, "request(NULL)");
    } else {
        len += sprintf(buf + len, "request(");
        if (request->complen > 0) {
            len += sprintf(buf + len, "comp: %.*s, ", request->complen, request->comp);
        }
        if (request->type == NFN_REQUEST_TYPE_UNKNOWN) {
            len += sprintf(buf + len, "type: %s, ", "UNKNOWN");
        } else {
            len += sprintf(buf + len, "type: %s, ", nfn_request_names[request->type-1]);
        }
        len += sprintf(buf + len, "arg: %.*s", request->arglen, request->arg);
        len += sprintf(buf + len, ")");
    }

    buf[len] = '\0';
    return buf;
}




#endif

// eof
