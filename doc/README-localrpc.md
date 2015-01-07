# README-localrpc.txt

2014-06-23 cft

A) Introduction

"Local RPC" is a framework for calling services at a neighbor's face.

To that end, a RPC request ("application" in terms of Lambda-Calculus)
and a RPC reply (in HTML style) message format is defined.

The data packet encoding conforms to NDN-TLV (Nov 2013) and introduces
new types for the two RPC message types as well as for data
marshalling (inside these two message types).

In NDN, the reserved code points (first byte of a packet) at the
top level are:

   interest   0x05 (defined by NDN)
   data       0x06 (defined by NDN)

We add to this a single code point

   RPC        0x80 (appl_cp)

for introducing both the request and reply messages. A grammar is
defined for the TLV format of the request and reply messages: The
rules in EBNF style follow in Section B.

As an example, APPL is the grammar rule for a message in TLV format:

   APPL   ::= appl_cp len EXPR ARG*
              ------- --- ---------
                 T     L      V

where appl_cp is the "T" (0x80 code point), "len" is the "L" counting
the bytes of all content following the "L" until the end of line,
which in this case the sum of bytes used for the EXPR and ARG elements
which together are the "V" of this TLV.


B) Local-RPC grammar in EBNF:

   RPCREQUEST ::= APPL
   RPCRESULT  ::= appl_cp len INT STR ARG*

   APPL   ::= appl_cp len EXPR ARG*
   VAR    ::= NAME | user_defined_cp 0
   LAMBDA ::= lambda_cp len VAR EXPR*

   EXPR   ::= APPL | VAR | LAMBDA

   ARG    ::= EXPR | SEQ | BASIC
   SEQ    ::= seq_cp len ARG*
   BASIC  ::= NAME | INT | BIN | STR

   NAME   ::= nam_cp len octet*
   INT    ::= int_cp len octet*
   BIN    ::= bin_cp len octet*
   STR    ::= str_cp len octet*


C) Table of defined code points:

   interest   0x05 (defined by NDN)
   data       0x06 (defined by NDN)
   appl_cp    0x80 (reserved across all protocol suites)

   lambda_cp  0x81
   seq_cp     0x82
   nam_cp     0x83
   int_cp     0x84
   bin_cp     0x85
   str_cp     0x86

   The values 0x00..0x7f are names with length 0 and an empty
   value field. These names have no assigned meaning and are
   available for users to bind them to arbitrary objects
   (functions, constants). See the note on future extensions.


D) Use cases/examples:

a) Requesting the remote local time:

   Symbolically:
     Fct call style:  '/rpc/builtin/lookup( /rpc/config/localTime )'
     LISP style:      '(/rpc/builtin/lookup /rpc/config/localTime)'

   Encoding of the RPC request:

   0000  80 2c -- <Application, len=44>
   0002    83 13 -- <Name, len=19>
   0004      /rpc/builtin/lookup
   0017    83 15 -- <Name, len=21>
   0019      /rpc/config/localTime

   Encoding of the RPC reply:

   0000  80 21 -- <RPC-result, len=33>
   0002    84 01 -- <Integer, len=1>
   0004      200
   0005    86 02 -- <String, len=2>
   0007      "ok"
   0009    86 18 -- <String, len=24>
   000b      "Tue Jun 24 04:03:29 2014"


E) Internal data structure / RPC Data Representation

A single data structure is used to represent all of:
- TLV packet
- RPC data representation (called RPR below)
as a tree data structure.

There are procedures for
- composing a new tree
- converting a tree into a flat TLV char buffer (serialize)
- converting a TLV char buffer to a tree (unserialize)

struct rdr_ds_s { // RPC Data Representation (RPR) data structure
    int type;
    int flatlen;
    unsigned char *flat;
    struct rdr_ds_s *nextinseq;
    union {
	struct rdr_ds_s *fct;
	struct rdr_ds_s *lambdavar;
	unsigned int nonnegintval;
	int namelen;
	int binlen;
	int strlen;
    } u;
    struct rdr_ds_s *aux;
};

Importing is done "lazily": A node's bytes are only parsed if details
are requested. When importing (rdr_unserialize()), a root node is
created that just keeps track of the flat bytes - at this stage it has
the type "SERIALIZED". Only by calling rdr_getType(), the bytes are
parsed and the true type is assigned to the node. At the same time,
this node's child nodes are created (in SERIALIZED state) and are
ready for inspection.

Export (rdr_serialize()) is done in two waves: first, a node's total
length requirement is computed (rdr_getFlatLen()); then, the tree is
traversed a second time and bytes are consecutively written to the
buffer (rdr_serialize_fill()).

Use of data structure fields:

ftc  - another node containing the function (of an 'application')

lambdavar - another node with the formal parameter (of a 'lambda')

nonnegintval - integer value (of an INT node)

aux  - used according to the node's type
       . for pointing at octets (in case of a name, string, binary node),
         where the octet count is in namelen, strlen, or binlen
       . first parameter node (of an 'application')
       . first body element node (of a 'lambda')
       . first list element node (of a 'sequence')

nextinseq - chains the elements (in case of application, lambda, sequence)

flat - points at the import/export buffer (valid after import, or after
       export)

flatlen - size of this node when serialized (valid after import followed
          by rdr_getType(), after rdr_getFlatLen(), or after serialization)


F) Command line conventions for ccn-lite-rpc

Expressions are written in prefix form (LISP style). That is,
parentheses introduce an 'application' where the first entry stands
for the function and the subsequent entries are its
parameters. Example:

'(/rpc/builtin/forward /rpc/const/encoding/ndn2013 &1)'

At the outermost level, parentheses can be omitted because
ccn-lite-rpc expects a remote procedure call to some function
(/rpc/builtin/forward in the example above).

Integers start with a digit.

Strings have to start and end with double quotes.

Binary data is introduced by &NUMBER, where the number is referring
  to additional filename parameters from where the content should be read.

The backslash '\\' introduces a lambda form and must be followed by a
name.


G) Things to come / future extension

A Lambda expression resolver will be added for full functional
programming support. With the associated environments (that keep the
name bindings), semi-permanent remote and user-definable contexts can
be offered where the special variable names (code points 0x00..0x7f)
will live.

# eof
