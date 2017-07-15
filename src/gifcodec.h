/*
  The LZW CODEC for GIF is Copyright (c) 1995 David Kaplan 

  The Graphics Interchange Format(c) is the Copyright property of
  CompuServe Incorporated.
*/
/*
   initialise encoder. declare the output file
*/
int16_t InitEncoder(FILE* out, int16_t bpp); /*bpp=8*/
/*
   encode a buffer of 256 characters,
   and write it to output file
*/

int16_t Encode(uint8_t  *buf, int32_t size);
/*
   last operation to do, when the coding is finished
*/
int16_t ExitEncoder(void);

/*
   Init decoder
   input file, codebits=8, pixelcount=8?
*/
int16_t InitDecoder( FILE* infile, int16_t codebits, int16_t rowsize);
/*
  decode a buffer
*/
int16_t Decode( uint8_t  *buf, int16_t npxls );
/*
  last operation
*/
void ExitDecoder(void);
