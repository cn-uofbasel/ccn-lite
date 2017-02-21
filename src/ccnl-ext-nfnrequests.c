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

enum nfn_request_type {
    NFN_REQUEST_TYPE_UNKNOWN = 0,
    NFN_REQUEST_TYPE_START,
    NFN_REQUEST_TYPE_SUSPEND,
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
    "SUSPEND",
    "RESUME",
    "CANCEL",
    "STATUS",
    "KA",
    "CIM",
    "GIM"};

struct nfn_request_s {
    unsigned char *comp;
    int complen;
    enum nfn_request_type type;
    
    int argscount;
    int *argslen;
    unsigned char **args;
};

char *
nfn_request_description_new(struct nfn_request_s* request)
{
    char *desc = (char *) ccnl_malloc(request->complen + 1);
    strncpy(desc, (char *)request->comp, request->complen);
    desc[request->complen] = '\0';
    return desc;
}

struct nfn_request_s*
nfn_request_new(unsigned char *comp, int complen)
{
    struct nfn_request_s *request = 
        (struct nfn_request_s *) ccnl_calloc(1, sizeof(struct nfn_request_s));

    DEBUGMSG_CORE(TRACE, "nfn_request_new(%.*s)\n", complen, comp);

    if (!request)
        return NULL;

    request->type = NFN_REQUEST_TYPE_UNKNOWN;
    char *request_name = NULL;
    int request_len = 0;

    
    int i;
    for (i = 0; i < 3; i++) {
        request_name = nfn_request_names[i];
        request_len = strlen(request_name);
        if (request_len <= complen && strncmp(request_name, (char *)comp, request_len)) {
            request->type = (enum nfn_request_type)i+1; // 0 is "unknown"
            break;
        }
    }

    if (request->type == NFN_REQUEST_TYPE_UNKNOWN) {
        DEBUGMSG_CORE(DEBUG, "Unknown request: %.*s\n", complen, comp);
        return request;
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

    request->argscount = 0;

    return request;
}






#endif

// eof
