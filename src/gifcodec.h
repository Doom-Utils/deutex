/*
  The LZW CODEC for GIF is Copyright (c) 1995 David Kaplan 

  The Graphics Interchange Format(c) is the Copyright property of
  CompuServe Incorporated.
*/
/*
   initialise encoder. declare the output file
*/
Int16 InitEncoder(FILE* out, Int16 bpp); /*bpp=8*/
/*
   encode a buffer of 256 characters,
   and write it to output file
*/

Int16 Encode(UInt8  *buf, Int32 size);
/*
   last operation to do, when the coding is finished
*/
Int16 ExitEncoder(void);

/*
   Init decoder
   input file, codebits=8, pixelcount=8?
*/
Int16 InitDecoder( FILE* infile, Int16 codebits, Int16 rowsize);
/*
  decode a buffer
*/
Int16 Decode( UInt8  *buf, Int16 npxls );
/*
  last operation
*/
void ExitDecoder(void);
