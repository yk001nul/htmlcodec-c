# htmlcodec-c
A library for compressing and decompressing texts with HTML symbols

# Design
The library uses a greedy dictionary approach to scan input text for HTML codes such as tags and attributes, and compress them to a single byte.
A pre-built dictionary of HTML tags and attributes are used to compress the data. 

# Algorithm
## Token extractions
```mermaid
flowchart LR
  Start([START])
  CheckChar{i < len ?}
  TextState[TEXT]
  TagOpenState[TAG_OPEN]
  CommentState[COMMENT]
  FindClose[READ TAG UNTIL '>' ]
  Malformed[INVALID TAG -> APPEND TEXT]
  GetTagInfo[PARSE TAG NAME, SELF-CLOSE, CLOSING]
  ValidTag{valid name ?}
  EmitStart[emit TAG_START]
  EmitEnd[emit TAG_END]
  ParseAttributes[parseHTMLAttributes]
  LoopBack((loop))
  Enrich[enrichHTMLTokenSubdata + RETURN]
  ContinueAfterComment[after comment -> loop]
  DetermineType{isClosing ?}

  Start --> CheckChar
  CheckChar -->|no| Enrich
  CheckChar -->|yes, not '<'| TextState
  TextState --> LoopBack
  CheckChar -->|yes, is '<'| TagOpenState
  TagOpenState -->|comment prefix| CommentState
  TagOpenState --> FindClose
  CommentState -->|closing '-->' found| ContinueAfterComment
  ContinueAfterComment --> LoopBack
  FindClose -->|no '>'| Malformed
  FindClose -->|yes '>'| GetTagInfo
  GetTagInfo --> ValidTag
  ValidTag -->|no| Malformed
  ValidTag -->|yes| DetermineType
  DetermineType -->|yes| EmitEnd
  DetermineType -->|no| EmitStart
  EmitStart --> ParseAttributes
  EmitEnd --> ParseAttributes
  ParseAttributes --> LoopBack
  Malformed --> LoopBack
```
## Decompression
### Data format
Data is read in sequence of 10-bits segments where the first two bits define how the next bits will be decoded.
| Two bits header value | Meaning |
|-----------------------|---------|
| 0x0 | Decode next 8 bit as ASCII |
| 0x1 | Decode next 9 bits as HTML tags |
| 0x2 | Decode next 9 bits as HTML attribute |
| 0x3 | Decode next 8 bits as common MIME type |

