# CCN-Lite Content Object Chunking Protocol

If a data for a name does exceed the maximum size of a content object (usually restricted by the network mtu size) the data has to be split up into several content objects. The util/ccn-lite-produce client application implements the standardized, offical and compatible chunking protocols for CCNTLV and NDNTLV respectively.  The util/ccn-lite-fetch application, similar to util/ccn-lite-peek, gets the data for a name. The difference to peek is that fetch only returns the final data instead of the full content object. This enables the implementation of the chunking protocol on the receiver side.

In the following the chunking protocols for both CCNTLV and NDNTLV is discussed as well as the fetch application collects the chunks from the network and reconstructs them to the final data.

## CCNTLV

## NDNTLV

Each content object has a optional field in MetaInfo calld FinalBlockId. A FinalBlockId contains a single NameComponent. This component represents the id of the last chunk for a chunked object. According to spec, not each chunk of a chunked object requires a FinalBlockId, but we have decided to add it to each chunk to make it very clear if a content object is a chunk or not. This does also not require CCN-Lite to have a sorted content store (the first content object returned for a name could be the first or the last or any other chunk, because each chunks has a FinalBlockId). The name of a chunked object is appended by a chunk identifier. Both the FinalBlockId as well as the chunk identifier could be any kind of data. We have decided for the following. A chunk starts with the character `c` followed by a signed integer starting from 0. The chunks must be ordered and continous without any missing number. This means the FinalBlockId contains the number of chunks. 
This would have to be changed for a streaming like chunking protocol where the number of chunks is not known beforehand. If size is of any concern, the character `c` is also not required at all, but increases legibility of names of chunked content objects.

`ccn-lite-produce` implements this protocol. It splits data into equally sized (either maximum of 4096B or user defined if smaller) chunks, with the last one not necessarily filled. This makes the final size predictable for the receiver.

## Fetch

The current implementation of fetch is very simplistic and not optimized yet. It first sends an interest for the initial given URI and/or NFN expression. If the received content object is not a chunk, the data is extracted and returned. If it is a chunk, a datastructure of the length of the expected number of chunks is initialized. In a loop, each missing chunk is fetched from the network until all chunks are found. If there is a timeout for a name, it is retried two times before the fetch times out and exits the application.
This means, the fetch only sequentially fetches the chunks without any optimization with a windowing-like mechanims. Another drawback is, that fetch is only able to fully reconstruct data and cannot operate in a streaming mode. A streaming mode would be very simple to implement (output each chunk directly if all previous chunks were already fetched), the only change required would be a dynamic data structure for all fetched chunks.

