#ifdef ESJ_DEMO

#define PIPE_STRING "PYPIPE: "
#define DEBUG_PIPE(fmt, ...) DEBUGMSG(99, PIPE_STRING#fmt, __VA_ARGS__);

/* named pipe commands */
enum {
    NP_DUMP_FORWARDING_TABLE = 123,
    NP_INTEREST_HIJACK       = 234,
    NP_HELLO                 = 345,
    NP_INTEREST_NEW          = 456,
    NP_CONTENT_RESPONSE      = 567,
    NP_NOTIFY_FIB_CHANGE     = 678,
    NP_NOTIFY_NEW_CONTENT    = 789,
};

//struct __attribute__((__packed__)) {
struct named_pipe_cmd_hdr {
    char magic_header[16];
    unsigned int command;
    unsigned int size;
} __attribute__((__packed__));


int ccnl_helper_read_file_for_len (int fd, unsigned char* buf, size_t len)
{
    int count = 0;

    while (count < len) {
        count += read(fd, &buf[count], len - count);
    }

    return count;
}

struct ccnl_face_s *ccnl_find_face_by_id (struct ccnl_relay_s *ccnl,
                                          int faceid)
{
    int i;
    struct ccnl_face_s *f;

    for (f = ccnl->faces, i = 0; f; f = f->next, i++) {
        if (f->faceid == faceid) {
            return f;
        }
    }
    return NULL;
}


int
ccnl_named_pipe_write (struct ccnl_relay_s *ccnl,
                       int command,
                       unsigned char* buffer,
                       unsigned int len)
{
    struct named_pipe_cmd_hdr npch;
    unsigned int total_bytes = 0;

    memset(&npch, 0, sizeof(npch));

    strcpy(npch.magic_header, "SPECIAL!!!");
    npch.command = command;
    npch.size = len;

    total_bytes += write(ccnl->named_pipe_write_fd, &npch,
                                sizeof(struct named_pipe_cmd_hdr));
    total_bytes += write(ccnl->named_pipe_write_fd, buffer, len);

    DEBUG_PIPE("wrote %d, total %d\n ", len, total_bytes);
    return 0;
}

int ccnl_named_pipe_write_notify_content_object (struct ccnl_relay_s *ccnl,
                                          unsigned char *buffer,
                                          unsigned int len)
{
    return ccnl_named_pipe_write(ccnl, NP_NOTIFY_NEW_CONTENT,
            buffer, len);
}

int ccnl_named_pipe_write_content_object (struct ccnl_relay_s *ccnl,
                                                  unsigned char *buffer,
                                                  unsigned int len)
{
    return ccnl_named_pipe_write(ccnl, NP_CONTENT_RESPONSE, buffer, len);
}

int ccnl_named_pipe_write_interest (struct ccnl_relay_s *ccnl,
                                    unsigned long int fromptr,
                                    unsigned char *buffer,
                                    unsigned int len)
{
    int rc;
    int buflen =  len + sizeof(unsigned long int);

    unsigned char *bigbuf = (unsigned char *) ccnl_malloc(buflen);

    memcpy(bigbuf, &fromptr, sizeof(fromptr));
    memcpy(bigbuf + sizeof(fromptr), buffer, len);

    DEBUG_PIPE("ptr is %ld, buffer len %u\n", fromptr, len);

    rc = ccnl_named_pipe_write(ccnl, NP_INTEREST_HIJACK,
                                bigbuf, buflen);
    ccnl_free(bigbuf);
    return rc;
}

int ccnl_dump_forwarding_table (unsigned char *buffer,
                                struct ccnl_relay_s *ccnl)
{
    int len = 0, i;
    struct ccnl_forward_s *fwd;
    unsigned char fname[10];

    for (fwd = ccnl->fib, i = 0; fwd; fwd = fwd->next, i++) {
        sprintf((char *)fname, "f%d", fwd->face->faceid);
        len += sprintf((char *)&buffer[len],
                "%4s: %s\n",
                fname, ccnl_prefix_to_path(fwd->prefix));
    }
    return 0;
}

void dump_buf (unsigned char *buf, unsigned char *data, unsigned int size)
{
    int len = 0;
    for (int z=0; z< size; z++) {
        len += sprintf((char *)&buf[len], "%x ", (unsigned char) data[z]);
    }
}

unsigned long int
ccnl_get_unique_id ()
{
    static unsigned long int uid = 1;

    return uid++;
}

struct ccnl_interest_s *
ccnl_get_matching_pit_uid (struct ccnl_relay_s *relay, unsigned long int uid)
{
    struct ccnl_interest_s *i = 0;
    for (i = relay->pit; i; i = i->next) {
        if (i->uid == uid) {
        return i;
        }
    }
    return NULL;
}

int
ccnl_named_pipe_reader (struct ccnl_relay_s *ccnl)
{
    int rc;
    unsigned char *buffer;
    struct named_pipe_cmd_hdr npch;

    rc = ccnl_helper_read_file_for_len(ccnl->named_pipe_read_fd,
                                  (unsigned char *) &npch,
                                  sizeof(struct named_pipe_cmd_hdr));

    DEBUG_PIPE("read %d, expected size %zu\n", rc,
                sizeof(struct named_pipe_cmd_hdr));
    DEBUG_PIPE("magic %s, command %d, size %d\n", npch.magic_header,
                                              npch.command,
                                              npch.size);

    if (npch.size) {
        buffer = (unsigned char * )ccnl_malloc(npch.size * sizeof(char));
        if (!buffer) {
            fprintf(stderr, "FATAL ERROR");
            return -1;
        }
        rc = ccnl_helper_read_file_for_len(ccnl->named_pipe_read_fd,
                                           buffer,
                                           npch.size);
    }

    switch (npch.command) {
        case NP_DUMP_FORWARDING_TABLE:
            {
                unsigned char *fwtable_buffer = (unsigned char *) ccnl_malloc(16*1024);
                if (!fwtable_buffer) {
                    fprintf(stderr, "FATAL ERROR");
                    return -1;
                }
                ccnl_dump_forwarding_table(fwtable_buffer, ccnl);
                DEBUG_PIPE("Table \n %s\n", fwtable_buffer);
                ccnl_named_pipe_write(ccnl, NP_DUMP_FORWARDING_TABLE,
                        fwtable_buffer, strlen((char *)fwtable_buffer));
                ccnl_free(fwtable_buffer);
            }
            break;
        case NP_INTEREST_HIJACK:
            {
                unsigned long int *interest_id = (unsigned long int *) &buffer[0];
                struct ccnl_interest_s *interest;

                interest = ccnl_get_matching_pit_uid (ccnl, *interest_id);
                if (!interest) {
                    fprintf(stderr,
                            "FATAL error: interest %ld no longer found\n",
                            interest->uid);
                    break;
                }
                DEBUG_PIPE("\nInterest id is %ld\n", interest->uid);
                /* reset interest uid to "normal
                */
                interest->uid = 0;
                ccnl_interest_propagate(ccnl, interest);
            }
            break;
        case NP_INTEREST_NEW:
            {
                struct ccnl_face_s *face;
                unsigned char *adj_buf = buffer+2;
                int len = npch.size - 2;
                face = ccnl_get_face_or_create(ccnl, -1, NULL, 0);

                ccnl_ccnb_forwarder(ccnl, face,
                        &adj_buf, &len);
            }
            break;
        case NP_HELLO:
            ccnl_named_pipe_write(ccnl, NP_HELLO,
                    NULL, 0);
            break;
        case NP_CONTENT_RESPONSE:
            {
                struct ccnl_face_s *face;
                int len = npch.size;
                face = ccnl_get_face_or_create(ccnl, -1, NULL, 0);
                ccnl_RX_ccnb(ccnl, face, &buffer, &len);
            }
            break;
        default:
            DEBUG_PIPE("Unknown command id %d\n", npch.command);
    }

    if (npch.size) {
        ccnl_free(buffer);
    }

    return 0;
}

void ccnl_pipe_notify_fib_change (struct ccnl_relay_s *ccnl)
{
    ccnl_named_pipe_write(ccnl, NP_NOTIFY_FIB_CHANGE, NULL, 0);
}

#endif
