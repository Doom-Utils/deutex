/*
  The LZW CODEC for GIF is Copyright (c) 1995 David Kaplan

  Alas, This LZW DECODER doesn't work yet.

  In fact this file is not used at all. All the GIF stuff is
  really in lzw.c. -- AYM 1999-09-07
*/


#include "deutex.h" /*for types. equivalent of deu.h*/


#if defined DeuTex /*deutex shit*/
#if NEWGIFE || NEWGIFD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gifcodec.h"

/*
  local defines
*/
#define HASHSIZE  5101
#define TABLESIZE 4096
#define HASHBITS	4

#define OK		0
#define EMPTY		-1
#define BAD_ALLOC 	-2
#define OVERFLOW	-3
#define OUTOFSYNC	-4





static void Enc_clear(void);
static void putCode(Int16 code);
static Int16 flush(void);
static Int16 findStr(Int16 pfx, Int16 sfx);
static void putCode(Int16 code);
static Int16 putbyte(Int16 byt);


static UInt8 buffer[255]; /* the write buffer */


/*
  tables
*/
static Int16   *codes; /* the code entry column of the table */
static Int16   *prefix; /* the prefix entry column of the table */
static Int16   *suffix; /* the suffix entry column of the table  */
/*
  codes
*/
static Int16   clrc; /* the value of the clearcode (the minsize + 1)  */
static Int16   endc; /* the value of the endcode (the minsize + 2)  */
/*
  control
*/
static Int16   nroots;   /* how many codes are already in use = bitlength of the pixels*/
		         /* 1 << pixelsize */
static Int16   minsize;  /* the minimum size for the first code (the highest value for the bitlength of the pixels*/
static Int16   cursize;  /* the current bitlength being used in the codes */
static Int16   maxsize;  /* the maximum bitlength that can be used (8) */

static Int16   curcode;  /* the next entry value that is available */
static Int16   maxcode;  /*  the maximum entry value of the table */
static Int16   bytecode; /* last code read or written */
static Int16   bytemask; /* mask for bytecode */
static Int16   nbytes;   /* COD the number of bytes that have been written */
                         /* DEC bytes in buffer       */
static Int16   nbits;    /* COD the number of bits being used in the highest empty slot */
			 /* DEC current element width */

#endif /*NEWGIFE NEWGIFD*/


/*
   ENCODER
*/
#if NEWGIFE
FILE* outfile; /* the file to be written to  */
static Int16   chrc; /* COD  the current byte being read */
static Int16   strc; /* COD  the number of the last entry to have a match */
static Int16   started;  /* a boolean variable used to see if this is the first string */

/* Initializes all the variables (yup, all them up there!) */

Int16 InitEncoder(FILE* out, Int16 bpp)
{
  codes  = (Int16 *) malloc(sizeof(Int16)*HASHSIZE);
  prefix = (Int16 *) malloc(sizeof(Int16)*HASHSIZE);
  suffix = (Int16 *) malloc(sizeof(Int16)*HASHSIZE);
  if((codes == NULL) || (prefix == NULL) || (suffix == NULL))
    return BAD_ALLOC;

  outfile = out;
  chrc = strc = started = 0;
  minsize = bpp + 1;
	maxsize = 12;
  nroots = 1 << bpp;
  clrc = nroots;
  endc = clrc + 1;
  bytecode = 0;
  bytemask = 1;
  nbytes = 0;
  started = 0;
  Enc_clear();
  putCode(clrc);
  return OK;
}
Int16 ExitEncoder(void)
{ flush();
  free(codes);
  free(prefix);
  free(suffix);
  return 0;
}
/* Encodes one string of bytes and writes to the current file pointer */
/* I will comment this part heavily to illustrate the process...      */

Int16 Encode(UInt8  *buf, Int32 size)
{
   Int16 pos = 0, index;

   /* if this is the first table to be used, setup some variables we 
      need such as the table's first entry: the first byte in the 
   array */

   if(!started)
	 { strc = buf[pos++];  // changed pos+1 to pos++
     started = 1;
   }
   /* go through the array of bytes */
   while(pos <= size)
   {
     chrc = buf[pos++];  /* get the next (or first) character */
     /* see if the current string (be it one character or more) 
     is in the table and return the corresponding entry number.
     If not, return the number of the next available entry */	
     index = findStr(strc, chrc);
     /* if the string IS in the index, we will get a number that
     corresponds to a string in the table: it wont be empty */
     if(codes[index] != EMPTY)  /* string found! */
     {
	/* set the current string to the one-byte
	code for the string that was found */
	strc = codes[index];
     }
     else /* not found! */
     {
	/* set the next entry in the table to
        the next available entry number */
	codes[index] = curcode;
	prefix[index] = strc; /* set the prefix entry to the strc */
	suffix[index] = chrc; /* set the suffix entry to the chrc */
	putCode(strc);        /* put the code into the out file (compression!) */
	strc = chrc; /* set the prefix to the current char */
	curcode++;  /* remember that the current entry is now filled */
        /* if the number of bytes in the current code is as big as
	 possible, we must reset the table! */				
	if(curcode > maxcode)
	{
	  cursize++; /* set the currentsize of the code to the next number */		
	  /* if the current # of bytes being used exceeds the max */
	  if(cursize > maxsize)
	  {
	    putCode(clrc); /* put a clearcode into the file to tell the decoder */
	    Enc_clear(); /* reset the table and start over */
	  }
	  else /* if not */
	  {
	    nbits = cursize; /* set the number of bits to the current size */
	    maxcode <<= 1; /* set maxcode to the current char bitshifted once */
			if(cursize == maxsize) /* if we are at the last bit-increase */
			maxcode--; /* make maxcode one less  so curcode can be larger than it */
		}
	}
		 }
	 }
	 return OK;
}

/* close the table: */

Int16 flush(void)
{
   putCode(endc); /* put an endcode in the file */
   if(bytemask != 1) /* if the last string was less than 256 bytes */
   {
     putbyte(bytecode); /* put the last byte into the file */
     bytecode = 0; /* and reset the variables*/
     bytemask = 1;
   }
   if(nbytes > 0) /* if the last string has anything in it */
   {
     fputc(nbytes, outfile); /* write the block out */
     fwrite(buffer, nbytes, 1, outfile);
     nbytes = 0;
   }
   return ferror(outfile) ? BAD_ALLOC : OK;
}

/* put the code in.  This is tricky because we use an int and we might have
	 a code that is only using 7 or less bits, we must compensate */

void putCode(Int16 code)
{
   Int16 mask = 1;
   Int16 n = nbits;
   while(n-- >0)
   {
     if(code & mask) bytecode |= bytemask;
     if((bytemask <<=1) > 0x80)
     {
       putbyte(bytecode);
       bytecode = 0;
       bytemask = 1;
     }
     mask <<= 1;
   }
}

/* clear the table for a new one to be built */

void Enc_clear(void)
{
  Int16 i;
  cursize = minsize;
  nbits = cursize;
  curcode = endc + 1;
  maxcode = 1 << cursize;
  for(i=0;i<HASHSIZE;i++) codes[i] = EMPTY;
}

/* see if a code and byte combo are already in the table */

Int16 findStr(Int16 pfx, Int16 sfx)
{
  Int16 i = (sfx << HASHBITS) ^ pfx; /* don't ask... */
  Int16 di = (i==0) ? 1: HASHSIZE - i; /* oy! */
  while(1) /* uhh... don't stop? */
  {
    if(codes[i] == EMPTY)  break;/* are we at the end of the list? */    
    /* if not, see if there is a match in the current entry */
    if((prefix[i] == pfx) && (suffix[i] == sfx)) break;
    i -= di; /* if not, check the next one */
    if(i<0) i+= HASHSIZE;
  }
  return i;
}

/*
   put a byte into the outfile.
   This coding by block is valid. it was tested OK.
*/

Int16 putbyte(Int16 byt)
{
  buffer[nbytes] = byt; /* adding the byte to the buffer */
  nbytes++;
  if(nbytes == 255) /* if we have reached the max of the buffer */
  {
    /* put the size of the next block of code to the file*/
    fputc(nbytes, outfile);
    /* put the code into the file next */
    fwrite(buffer, nbytes, 1, outfile);
    nbytes = 0;
  }
  /* we have now successfully written a coded block of data to the GIF file! */
  /* if the buffer isn't full, just wait until another byte is added */
  return ferror(outfile) ? BAD_ALLOC : OK;
}
#endif /*NEWGIFE*/





/*
  decoder
*/
#if NEWGIFD
#define MAX_BUF_SIZE 100

static Int16  bitcode;   /* DEC last bit code read */
static Int16  bitmask;   /* DEC mask for bitcode  */
static Int16  curbyte;   /* DEC index of current byte */
static Int16  capacity;  /* DEC 1 << 12  */
static Int16  rwbytes;   /* DEC bytes per scan line */
static FILE  *ifile;     /* input file */
static Int16  stkp;      /* stack pointer */
static UInt8 *stack;
static UInt8 *bytes;
static Int16  newc;  /* DEC  new code */
static Int16  oldc;  /* DEC  old code */
/*! David
  suffix is now an INT16, not a unsigned char as before
  cond was unused
*/

void reset_decoder(void);
Int16 getcode( void );
Int16 getbyte( void );


/*            Decoder( ), iCodeStream( infile, cobits+1 )*/
Int16 InitDecoder( FILE* infile, Int16 codebits, Int16 rowsize)
{  minsize  = codebits + 1;
   maxsize  = 12;
   nroots   = 1 << codebits; /*cobits=8*/
   capacity = 1 << maxsize;
   rwbytes  = rowsize;
   ifile    = infile;
   nbytes   = curbyte = 0;
   nbits    = minsize;
   bytecode = 0;
   bytemask = 0x80;
   bitcode  = 0;
   bitmask  = 0x01;

   prefix   = (Int16*) malloc(capacity);
   suffix   = (Int16*) malloc(capacity);
   stack    = (UInt8*) malloc(capacity);
   bytes    = (UInt8*) malloc(rwbytes);
   if( (prefix==NULL) || (suffix==NULL) || (stack==NULL) || (bytes==NULL) )
      return BAD_ALLOC;
   reset_decoder();
   return OK;
}


void ExitDecoder()
{
  free(prefix);
  free(suffix);
  free(stack);
  free(bytes);
}

Int16 push( Int16 s )
{ if( stkp < capacity )
  {
     stack[stkp++] = s;
     return OK;
  }
  return OVERFLOW;
}

Int16 Decode(UInt8  *buf, Int16 npxls )
{ Int16 nb = 0;
  Int16 cond, code;
  /* empty stack into scan line buffer */
  while( (stkp > 0) && (nb < rwbytes) )
    bytes[nb++] = stack[--stkp];

  /* loop until a row has been decoded */
  while( nb < rwbytes )
  {  /*....... get next code */
     if( (code=getcode()) < 0 ) break;

     /*....... check for a clear code*/
     if( code == clrc )
     {
	reset_decoder();
	if( oldc == endc ) break;
	bytes[nb++] = oldc;
	if( nb == rwbytes ) break;
     }
     /*....... check for an end code*/
     else if( code == endc )
     {
       break;
     }
     /*....... code is a data code*/
     else
     {
	if( code == curcode )
	{
	  code = oldc;
	  cond = push( newc );
	}
	else if( code > curcode )
	{
	  cond = OUTOFSYNC;
	  break;
	}
	while( code > endc )
	{
	  cond = push( (suffix[code]&0xFF) );
	  code = prefix[code];
	}
	cond = push( code );
        /* add code to table */
	if( curcode < maxcode )
	{
	  newc = code;
	  suffix[curcode] = newc;
	  prefix[curcode] = oldc;
	  oldc = bitcode;
	  curcode++;
	}
        /* current width exhausted? */
	if( curcode >= maxcode )
	{
	  if( cursize < maxsize )
	  {
	    cursize++;
	    nbits = cursize;
	    maxcode <<= 1;
          }
	  else if( (code=getcode()) == clrc )
	  {
	    reset_decoder();
	    if( oldc == endc ) break;
	    bytes[nb++] = oldc;
	    if( nb == rwbytes ) break;
	  }
	  else
	  {
		cond = OVERFLOW;
		break;
	  }
	}
        /* transfer string from stack to buffer*/
	while( (stkp > 0) && (nb < rwbytes) )
		 bytes[nb++] = stack[--stkp];
     }
  }
  /*! David
    I use fmemcpy not memcpy, because data is FAR    (char  *)
  */
  _fmemcpy( buf, bytes, npxls );
  return cond;
}


void Dec_clear(void)
{
  Int16 i;
  cursize = minsize;
  nbits = cursize;
  clrc = nroots;
  endc = clrc + 1;
  curcode = endc + 1;
  maxcode = 1 << cursize;
  for(i=0; i<curcode; i++ )
  {
    suffix[i] = i;
    prefix[i] = 0;
  }
}




Int16 getcode( void )
{ Int16 n;
  bitcode = 0;
  bitmask = 1;
/*! David, one only write that kind of stuff at the IOCCC.
  MODIF:
   n = nbits;
   while( n-- > 0 )
*/
  for(n=0; n<nbits; n++)
  {
    if( (bytemask<<=1) > 0x80 )
    {
      bytemask = 1;
      if( (bytecode = getbyte()) < 0 ) return -1;
    }
    if( bytecode & bytemask ) bitcode |= bitmask;
    bitmask <<= 1;
  }  return bitcode;
}
/*
  get a byte block
*/
Int16 getbyte( void )
{
  if( curbyte >= nbytes ) /*need a new block*/
  {
    /*! David
       the block size is an unsigned char
       MODIFIED
       if( (nbytes = fgetc( ifile )&) < 1 ) return -1;
    */
    for(nbytes=0;nbytes==0;) /*get a non zero block*/
    { if( (nbytes = fgetc( ifile )) == EOF ) return -1;
      nbytes &= 0xFF;
    }
    if(fread( buffer, nbytes, 1, ifile ) != 1 ) return -1;
    curbyte = 0;
  }
  return buffer[curbyte++];
}


void reset_decoder(void)
{ stkp = 0;
  Dec_clear();
  while( (oldc=getcode()) == clrc );
  newc = oldc;
}
#endif /*NEWGIFD*/
#endif /*DeuTex*/

