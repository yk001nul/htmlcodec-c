# htmlcodec-c
A library for compressing and decompressing texts with HTML symbols

# Design
The library uses a greedy dictionary approach to scan input text for HTML codes such as tags and attributes, and compress them to a single byte.
A pre-built dictionary of HTML tags and attributes are used to compress the data. 

# Algorithm
## Token extractions
1. Extract tokens from an input HTML string and classify each token on whether they are:
    1. Text
    1. Open HTML tags
    1. Closed HTML tags
    1. Self-closed HTML tags
    1. Attributes
    1. Comments

## Decompression
### Data format
Data is read in sequence of 10-bits segments where the first two bits define how the next bits will be decoded.
| Two bits header value | Meaning |
|-----------------------|---------|
| 0x0 | Decode next 8 bit as ASCII |
| 0x1 | Decode next 9 bits as HTML tags |
| 0x2 | Decode next 9 bits as HTML attribute |
| 0x3 | Decode next 8 bits as common MIME type |

