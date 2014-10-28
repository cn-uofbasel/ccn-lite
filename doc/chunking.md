# CCN-Lite Content Object Chunking Protocol

If data for a name does exceed the maximum size of a content object (usually restricted by the network MTU size) the data has to be split up into several chunks of content objects. The util/ccn-lite-produce client application implements protocol-compatible chunking protocols for CCNTLV and NDNTLV respectively.  The util/ccn-lite-fetch application, similar to util/ccn-lite-peek, gets the data for a name. The difference to peek is that fetch only returns the final data instead of the full content object.

In the following the chunking protocols for both CCNTLV and NDNTLV is discussed, as well as the fetch application collects the chunks from the network and reconstructs them to the final data.

## CCNTLV
In CCNx each packet has optional TLV MetaData fields. On of these fields is called ENDChunk representing an integer. This integer is the number of the last chunk of a chunked object. In the CCNx Name representation exists one field called `Chunk` containing the chunk index of the content object the name belongs to. This means each content object is clearly identified by the chunk number as well as the number of the last chunk.


## NDNTLV

Each content object has a optional field in MetaInfo called FinalBlockId. A FinalBlockId contains a single NameComponent. This component represents the id of the last chunk of a chunked object. According to the NDNTLV specification, not each chunk of a chunked object requires a FinalBlockId, but we have decided to add it to each chunk to make it very clear if a content object is a chunk or not. This does also not require CCN-Lite to have a sorted content store (the first content object returned for a name could be the first or the last or any other chunk, because each chunks has a FinalBlockId). 
The name of a chunked object is appended to the name as a name component by a chunk identifier. Both the FinalBlockId as well as the chunk identifier could be any kind of data. We have decided for the following. A chunk starts with the character `c` followed by a signed integer starting from 0. The chunks must be ordered and continuous without any missing number. This means the FinalBlockId represents the total number of chunks. 
This would have to be changed for a streaming like chunking protocol where the number of chunks is not known beforehand. If size is of any concern, the character `c` is also not required at all, but increases legibility of names of chunked content objects.


## Produce
`ccn-lite-produce` implements both the CCNTLV and NDNTLV protocols. It splits data into equally sized (either maximum of 4096B or user defined if smaller) chunks, with the last one not necessarily filled. This makes the final size predictable for the receiver.

## Fetch

The current implementation of fetch is very simplistic and not optimized yet. It first sends an interest for the initial given URI and/or NFN expression. For NDNTLV a non-chunk name can be send and one chunk (if sorted correctly the first chunk) will be send back. Since in CCNTLV forwarding relies on exact match, the first request must contain the identifier 0 in the name. This is also the reason why `fetch` currently only is able to fetch non-chunked objects for NDN.
If the received content object is not a chunk, the data is extracted and returned. If it is a chunk, a data structure of the length of the expected number of chunks is initialized. In a loop, each missing chunk is fetched from the network until all chunks are found. If there is a timeout for a name, it is retried two times before the fetch times out and exits the application. Fetch only sequentially fetches the chunks without any optimization with a windowing-like mechanism. In its current form, fetch is only able to fully reconstruct data and cannot operate in a streaming mode. A streaming mode would be very simple to implement (output each chunk directly if all previous chunks were already fetched), the only change required would be a dynamic data structure for all fetched chunks.
