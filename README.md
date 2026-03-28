# htmlcodec-c
A library for compressing and decompressing texts with HTML symbols

# Design
The library uses a greedy dictionary approach to scan input text for HTML codes such as tags and attributes, and compress them to a single byte.
A pre-built dictionary of HTML tags and attributes are used to compress the data. 

# Algorithm
## Compression
1. Let *S* be the input string
1. Let *T* be the size of the input string
1. Let *U* be the symbol table
1. Let *R1* be the range of tags symbol, from 0 to 255
1. Let *R2* be the range of attributes symbol, from 256 to 512
1. Let *R3* be the range of MIME type symbol, from 513 to 768
1. Let *m* be the start of a substring
1. Let *n* be the end of a substring
1. Let *o* be the size of the symbol in the table
1. Let *p* be the flag to the type of symbol discovered
1. Set *m* to 0
1. Set *p* to 0  
1. Parse *S* while *m* is less than *T* - 1:
    1. Find *n*, the position of the first right sharp bracket character (0x3E) between position *m* and position *T*-1 in *S*
    1. Extract substring *V* from position *m* to position *n* in *S*
    1. Fetch next symbol *L* from *R1*
    1. Compare *V* to *L*, ignoring case:
        1. Let *W* be the type of tags found
        1. Let *C1* be the first character in *V*
        2. Let *C2* be the second character in *V*
        3. Let *C3* be the second last character in *V*
        4. Let *C4* be the last character in *V*
        1. If *C1* is left sharp bracket (0x3C), *C4* is (0x3E) and the remaining characters between *C1* and *C4* matches *L*, this is an starting tag with no attribute, set *W* to 1
        1. If *C1* is (0x3C), *C2* is slash (0x2F), *C4* is (0x3E) and the remaining characters between *C2* and *C4* matches *L*, this is a closing tag with no attribute, set *W* to 2
        1. If *C1* is (0x3C), *C3* is (0x2F), *C4* is (0x3E) and the remaining characters between *C1* and *C3* matches *L*, this is a self-closed tag with no attribute, set *W* to 3
        1. If *C1* is (0x3C), *C4* is (0x3E) and the remaining characters between *C1* and *C4* _does not_ matches *L*, this is a starting tag with attributes
    

## Decompression
### Data format
Data is read in sequence of 10-bits segments where the first two bits define how the next bits will be decoded.
| Two bits header value | Meaning |
|-----------------------|---------|
| 0x0 | Decode next 8 bit as ASCII |
| 0x1 | Decode next 9 bits as HTML tags |
| 0x2 | Decode next 9 bits as HTML attribute |
| 0x3 | Decode next 8 bits as common MIME type |

