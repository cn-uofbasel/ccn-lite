/*
 * @f ccnl.h
 * @b header file for CCN lite (CCNL)
 *
 * Copyright (C) 2011, Christian Tschudin, University of Basel
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
 * 2011-03-30 created
 */

#define CCNL_ETH_TYPE			0x88b5

#define CCNL_MAX_INTERFACES		10
#define CCNL_MAX_PACKET_SIZE		8096

#define CCNL_CONTENT_TIMEOUT		30 // sec
#define CCNL_INTEREST_TIMEOUT		4  // sec
#define CCNL_MAX_INTEREST_RETRANSMIT	2

// #define CCNL_FACE_TIMEOUT	60 // sec
#define CCNL_FACE_TIMEOUT	15 // sec

#define CCNL_MAX_NAME_COMP	64
#define CCNL_MAX_IF_QLEN	64

#define CCNL_DEFAULT_MAX_CACHE_ENTRIES	0   // means: no content caching
#define CCNL_MAX_NONCES			256 // for detected dups


// ----------------------------------------------------------------------
// our own CCN-lite extensions for the ccnb encoding:

// management protocol: (ccnl-ext-mgmt.c)
#define CCNL_DTAG_MACSRC	99001 // newface: which L2 interface
#define CCNL_DTAG_IP4SRC	99002 // newface: which L3 interface
#define CCNL_DTAG_ENCAPS	99003 // fragmentation protocol, see core.h
#define CCNL_DTAG_FACEFLAGS	99004 //
#define CCNL_DTAG_DEVINSTANCE	99005 // adding/removing a device/interface
#define CCNL_DTAG_DEVNAME	99006 // name of interface (eth0, wlan0)
#define CCNL_DTAG_DEVFLAGS	99007 //

#define CCNL_DTAG_DEBUGREQUEST  99008 //
#define CCNL_DTAG_DEBUGACTION   99009 // dump, halt, dump+halt      

#define CCNL_DTAG_UNIXSRC	99010 // newface: which UNIX path

// fragmentation protocol: (ccnl-ext-encaps.c)
#define CCNL_DTAG_FRAGMENT	144144 // http://redmine.ccnx.org/issues/100803
#define CCNL_DTAG_FRAG_FLAGS	(CCNL_DTAG_FRAGMENT+1)
#define CCNL_DTAG_FRAG_OSEQN	(CCNL_DTAG_FRAGMENT+2)  // our seq number
#define CCNL_DTAG_FRAG_OLOSS	(CCNL_DTAG_FRAGMENT+3)  // our loss count
#define CCNL_DTAG_FRAG_YSEQN	(CCNL_DTAG_FRAGMENT+4)  // your (last) seq no
/*
#define CCNL_DTAG_FRAG_YSEQN16	(CCNL_DTAG_FRAGMENT+4)
#define CCNL_DTAG_FRAG_YSEQN32	(CCNL_DTAG_FRAGMENT+5)
*/
#define CCNL_DTAG_FRAG_FLAG_FIRST	0x01
#define CCNL_DTAG_FRAG_FLAG_LAST	0x02
#define CCNL_DTAG_FRAG_FLAG_STATUSREQ	0x04

// eof
