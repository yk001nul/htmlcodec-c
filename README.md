# htmlcodec-c
A library for compressing and decompressing texts with HTML symbols

# Design
The library uses a greedy dictionary approach to scan input text for HTML codes such as tags and attributes, and compress them to a single byte.
A pre-built dictionary of HTML tags and attributes are used to compress the data. 

# Compression
todo

# Decompression
## Data format
Data is read in sequence of 10-bits segments where the first two bits define how the next 8 bits will be decoded.
| Two bits header value | Meaning |
|-----------------------|---------|
| 0x0 | Decode next 8 bit as ASCII |
| 0x1 | Decode next 8 bits as HTML tags |
| 0x2 | Decode next 8 bits as HTML attribute |
| 0x3 | Decode next 8 bits as common MIME type |

