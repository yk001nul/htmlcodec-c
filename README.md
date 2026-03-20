# htmlcodec-c
A library for compressing and decompressing texts with HTML symbols

# Design
The library uses a greedy dictionary approach to scan input text for HTML codes such as tags and attributes, and compress them to a single byte.
A pre-built dictionary of HTML tags and attributes are used to compress the data. 

# Algorithm
## Compression
1. Let *S* be the input string
1. Let *T* be the size of the input string
1. Let *m* be the start of a substring
1. Let *n* be the end of a substring
1. Let *o* be the size of the symbol in the current table
1. Let *p* be the flag to the type of symbol discovered
1. Set *m* to 0
1. 
1. While a symbol is not found
  1. Set *p* to 0  
  1. Fetch the next symbol *L* from the tags table
  1. Set *n* to the length of *L*
  1. Extract substring *U* from position *m* to position *n* in *S*
  1. If *U* matches *L* ...

## Decompression
### Data format
Data is read in sequence of 10-bits segments where the first two bits define how the next bits will be decoded.
| Two bits header value | Meaning |
|-----------------------|---------|
| 0x0 | Decode next 8 bit as ASCII |
| 0x1 | Decode next 9 bits as HTML tags |
| 0x2 | Decode next 9 bits as HTML attribute |
| 0x3 | Decode next 8 bits as common MIME type |

