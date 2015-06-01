# CCN-Lite Content Object Chunking Protocol

If data for a name exceeds the maximum size of a single content object (usually restricted by the network MTU size) the data has to be split into several content objects, also referred to as chunks. The `ccn-lite-produce` client application implements a chunking protocol for CCNTLV and NDNTLV respectively according to their specification.  The `ccn-lite-fetch` application, similar to `ccn-lite-peek`, gets the data for a name. The difference to peek is that fetch only returns the final data instead of the full content object but if the first content object is a chunk it will fetch or stream all chunks.

In principle, both implementations are based on the same chunking protocol. Each chunk of a chunk stream has a chunk number. Chunk numbers start from 0 and increment until the last chunk of the stream. Each chunk object can optionally provide the number of the last chunk. The stream of chunks is complete as soon as the chunk number is equal to the last chunk number. This means that the producer can dynamically increase the last chunk number, but only as long as there was never a chunk published with the same number as the last chunk number.

In the following the encoding of this protocol for both CCNTLV and NDNTLV as well as the produce and fetch application are discussed in more detail. To see how these tools work or to test the implementation, the `test/scripts/demo-relay-fetch.sh` script can be used.

## CCNTLV
In CCNx are two protocols which can be used for chunking. One makes use of metadata, the other is a simpler scheme relying only on chunk numbers. `ccn-lite-produce` implements the ladder.
In CCNx a name consists of segments with types, where one type is called `Chunk`. The value is an integer and represents the chunk number (for interests and content objects).  
The last chunk number is encoded in an optional TLV MetaData field. The field is called `ENDChunk` and its value is an integer. This integer is the number of the last chunk of a stream of chunks. 
For more details see [CCNx Content Object Chunking](http://www.ccnx.org/pubs/ccnx-mosko-chunking-01.txt)

## NDNTLV

In NDN there is no explicit encoding of chunk numbers in the protocol. NDN suggests the use of markers to encode chunk numbers, but the applications are free to use their own scheme.
To encode the chunk number in an interests or content object, the last component of the name should start with the chunk marker. The marker for chunk numbers is the byte `%00` followed by variable integer for the actual number.
To encode the last chunk number, there is an optional field for the content object in NDN called `FinalBlockId`. The value of this field is a name component. This is used for the chunking protocol. As soon as the last component of a name is equal to the component of the final block id, the chunk stream is complete. This means the final block id should also start with the byte `%00` followed by a variable integer of the last chunk number.
For more details see [NDN Packet Spec for Content Objects](http://named-data.net/doc/ndn-tlv/data.html).

## Produce
`ccn-lite-produce` implements both the CCNTLV and NDNTLV chunking protocols. CCNB or other encodings are not yet supported. It splits data into equally sized (either maximum of 4096B or user defined with `-c` if it should be smaller) chunks where the last chunk contains the value for the last chunk. 
By default it prints all chunks to stdout. With `-o DIRNAME` each chunk is written to a separate file (`-f FILENAME` can be used to change the name of the files).

## Fetch
`ccn-lite-fetch` retrieves the data for either a single content object (only NDN) or a stream of chunks. For NDN it first sends an interest for the user-provided name. For CCNx the first interest is always for chunk 0, because CCNx uses exact matches for content. This has the consequence that fetch is only able to fetch chunk streams for CCNx and not a single content object.

If the retrieved object for the first interest does not have a chunk number, the data is extracted and printed and the application exits. Otherwise it continues to fetch all chunks. If the first chunk does not have the chunk number 0, it copies the name, replaces the chunk number with the chunk number 0, drops the fetched content object and sends an interest for the first chunk 0. For each retrieved chunk, it replaces the chunk number in the name of the fetched content object with the next chunk number to get the next chunk, until there is a chunk where the chunk number is equal to the last chunk number (or there is a timeout for a specific chunk). 

It is important to note that by taking the retrieved name and adding/replacing a chunk number, with NDN fetch is able to retrieve data for names which have some other name components potentially not provided by the user (like the version number). Without this, a fetch for the name `/foo/bar` would not be able to retrieve chunks of the form `/foo/bar/versionbytes/%00%00` because an interest for `/foo/bar/%00%00` would be sent. For CCNx, fetch only works if the provided name is fully qualified.
