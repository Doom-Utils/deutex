/* +-------------------------------------------------------------------+ */
/* | Copyright 1993, David Koblas (koblas@netcom.com)                  | */
/* |                                                                   | */
/* | Permission to use, copy, modify, and to distribute this software  | */
/* | and its documentation for any purpose is hereby granted without   | */
/* | fee, provided that the above copyright notice appear in all       | */
/* | copies and that both that copyright notice and this permission    | */
/* | notice appear in supporting documentation.  There is no           | */
/* | representations about the suitability of this software for        | */
/* | any purpose.  this software is provided "as is" without express   | */
/* | or implied warranty.                                              | */
/* |                                                                   | */
/* +-------------------------------------------------------------------+ */
/*
** This file is derived from ppmtogif.c and giftoppm.c
** only the compression/decompression routines were kept.
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
** Lempel-Zim compression based on "compress".
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
**
** This coder is included in DeuTex only because that of David Kaplan
** didn't work
** 
*/


#include "deutex.h"
#include "tools.h"

/*compile only for DeuTex*/
#if defined DeuTex


#if NEWGIFE && NEWGIFD
/*nothing, for new gif encoder*/
#else
/*
 * General DEFINEs
 */

#define BITS    12
#define HSIZE  5003            /* 80% occupancy */

typedef char   char_type;
typedef Int16  code_int;  /*  2**BITS values  of type int, and also -1 */
typedef Int32   count_int;



/*static Int16	table[2][(1<<BITS)];*/
static Int16 *table[2];
/*static Int16	stack[(1<<(BITS))*2];*/
static Int16 *stack;
#endif


#if NEWGIFD
#else
void decompressInit(void)
{  
   table[0]=(Int16 *)Malloc(sizeof(Int16)*(1<<BITS));
   table[1]=(Int16 *)Malloc(sizeof(Int16)*(1<<BITS));
   stack=(Int16 *)Malloc(sizeof(Int16)*(1<<BITS)*2);
}
void decompressFree(void)
{  
   Free(table[0]);
   Free(table[1]);
   Free(stack);
}
#endif


#if NEWGIFE
#else
/*static count_int htab [HSIZE];*/
static count_int *htab=NULL;
/*static unsigned Int16 codetab [HSIZE];*/
static UInt16 *codetab=NULL;
void compressInit(void)
{  
   htab    =(count_int *)Malloc(sizeof(count_int)*HSIZE);
   codetab =(UInt16 *)Malloc(sizeof(Int16)*HSIZE);
}
void compressFree(void)
{
   Free(htab);
   Free(codetab);
}

static void output ( code_int code );
static void cl_block ( void );
static void cl_hash ( count_int hsize );
static void char_init ( void );
static void char_out ( Int16 c );
static void flush_char ( void );
#endif



/*
**
** GIF compression
**
*/









/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/



 
/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>


#if NEWGIFE
#else
static Int16 n_bits;                      /* number of bits/code */
static Int16 maxbits;                	/* user settable max # bits/code */
static code_int maxcode;                /* maximum code, given n_bits */
static code_int maxmaxcode; 		/* should NEVER generate this code */
# define MAXCODE(n_bits)        	(((code_int) 1 << (n_bits)) - 1)

#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int hsize;                 	/* for dynamic table sizing */

static UInt32 cur_accum;
static Int16 cur_bits;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i) ((char_type*)(htab))[i]
#define de_stack        ((char_type*)&tab_suffixof((code_int)1<<BITS))

static code_int free_ent;              /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static Int16 clear_flg;
static Int32 in_count;            /* length of input */
static Int32 out_count;           /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static Int16 g_init_bits;
static FILE* g_outfile;

static Int16 ClearCode;
static Int16 EOFCode;

void compress( Int16 init_bits, FILE *outfile, code_int (*ReadValue)(void))
{
    register Int32 fcode;
    register code_int i /* = 0 */;
    register Int16 c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register Int16 hshift;

    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    g_init_bits = init_bits;
    g_outfile = outfile;
    /*
     * Set up the necessary values
     */
    out_count = 0;
    clear_flg = 0;
    in_count = 1;
    maxbits = BITS;
    maxmaxcode = 1 << BITS;
    maxcode = (code_int)MAXCODE(n_bits = g_init_bits);
    hsize = HSIZE;
    cur_accum = 0;
    cur_bits  = 0;

    ClearCode = (Int16)(1 << (init_bits - 1));
    EOFCode = (Int16)(ClearCode + 1);
    free_ent = (code_int)(ClearCode + 2);

    char_init();

    ent = ReadValue();

    hshift = 0;
    for ( fcode = (Int32) hsize;  fcode < 65536L; fcode *= 2L )
      hshift++;
    hshift = (Int16)(8 - hshift);      /* set hash code range bound */

    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);            /* clear hash table */

    output( (code_int)ClearCode );


    while ((c = ReadValue()) != EOF )
    {  ++in_count;
       fcode = (Int32) (((Int32) c << maxbits) + ent);
       i = (code_int)(((code_int)c << hshift) ^ ent);    /* xor hashing */

       if ( HashTabOf (i) == fcode )
       { ent = CodeTabOf (i);
         continue;
       }
       else if ( (Int32)HashTabOf (i) < 0 )      /* empty slot */
            goto nomatch;
       disp = (code_int)(hsize_reg - i);          /* secondary hash (after G. Knott) */
       if ( i == 0 )  disp = 1;
probe:
       if ( (i -= disp) < 0 )     i += hsize_reg;

       if ( HashTabOf (i) == fcode )
       {  ent = CodeTabOf (i);
          continue;
       }
       if( (Int32)HashTabOf (i) > 0 ) goto probe;
nomatch:
       output ( (code_int) ent );
       out_count++;
       ent = c;

       if ( free_ent < maxmaxcode )
       {  CodeTabOf (i) = free_ent++; /* code -> hashtable */
          HashTabOf (i) = fcode;
       }
       else
         cl_block();
    }
    /*
     * Put out the final code.
     */
    output( (code_int)ent );
    ++out_count;
    output( (code_int) EOFCode );
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (Int32)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits Int32.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static UInt32 masks[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                  0x001F, 0x003F, 0x007F, 0x00FF,
                                  0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                  0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

static void output( code_int  code)
{
    cur_accum &= masks[ cur_bits ];
    if( cur_bits > 0 )
      cur_accum |= ((Int32)code << cur_bits);
    else
      cur_accum = code;
    cur_bits += n_bits;
    while( cur_bits >= 8 )
    { char_out( (UInt16)(cur_accum & 0xff) );
      cur_accum >>= 8;
      cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
   if ( free_ent > maxcode || clear_flg )
   { if( clear_flg )
     { maxcode = (code_int)(MAXCODE (n_bits = g_init_bits));
       clear_flg = 0;
     }
     else
     { n_bits++;
       if ( n_bits == maxbits )
         maxcode = maxmaxcode;
       else
	 maxcode = (code_int)(MAXCODE(n_bits));
     }
   }
   if( code == EOFCode )
   { /* At EOF, write the rest of the buffer.*/
     while( cur_bits > 0 )
     { char_out( (UInt16)(cur_accum & 0xff) );
       cur_accum >>= 8;
       cur_bits -= 8;
     }
     flush_char();
     fflush( g_outfile );
   }
}

/*
 * Clear out the hash table
 */
static void cl_block ()             /* table clear for block compress */
{ cl_hash ( (count_int) hsize );
  free_ent = (code_int)(ClearCode + 2);
  clear_flg = 1;
  output( (code_int)ClearCode );
}

static void cl_hash(register count_int hsize)          /* reset code table */
{ count_int *htab_p = htab+(int)hsize;
  register Int32 i;
  register Int32 m1 = -1;
  i = hsize - 16;
  do
  {  /* might use Sys V memset(3) here */
     *(htab_p-16) = m1;
     *(htab_p-15) = m1;
     *(htab_p-14) = m1;
     *(htab_p-13) = m1;
     *(htab_p-12) = m1;
     *(htab_p-11) = m1;
     *(htab_p-10) = m1;
     *(htab_p-9) = m1;
     *(htab_p-8) = m1;
     *(htab_p-7) = m1;
     *(htab_p-6) = m1;
     *(htab_p-5) = m1;
     *(htab_p-4) = m1;
     *(htab_p-3) = m1;
     *(htab_p-2) = m1;
     *(htab_p-1) = m1;
     htab_p -= 16;
  } while ((i -= 16) >= 0);

  for ( i += 16; i > 0; --i )
                *--htab_p = m1;
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Number of characters so far in this 'packet'
 */
static Int16 a_count;

/*
 * Set up the 'byte output' routine
 */
static void char_init()
{  a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[ 256 ];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void char_out( Int16 c)
{  accum[ a_count++ ] = (char)c;
   if( a_count >= 254 )
                flush_char();
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void flush_char()
{  if( a_count > 0 )
   { fputc( a_count, g_outfile );
     fwrite( accum, 1, a_count, g_outfile );
     a_count = 0;
   }
}
#endif /*NEWGIFE*/
/* The End */





/*
**
**  Gif decompression
**
*/

#if NEWGIFD
#else
#define TRUE 1
#define FALSE 0
#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) == 1)

static Int16 ZeroDataBlock = FALSE;

Int16 GetDataBlock(FILE *fd, unsigned char 	*buf)
{ unsigned char	count;
  if (! ReadOK(fd,&count,1))
  { /* pm_message("error in getting DataBlock size" ); */
    return -1;
  }
  ZeroDataBlock = (count == 0)? TRUE :FALSE;
  if ((count != 0) && (! ReadOK(fd, buf, count)))
  { /* pm_message("error in reading DataBlock" ); */
    return -1;
  }
  return (Int16)(count&0xFF);
}

Int16 GetCode(FILE *fd, Int16 code_size, Int16 flag)
{  static unsigned char	buf[280];
   static Int16		curbit, lastbit, done, last_byte;
   Int16			i, j, ret;
   unsigned char		count;

   if (flag)
   { curbit = 0;
     lastbit = 0;
     done = FALSE;
     return 0;
   }

   if ( (curbit+code_size) >= lastbit)
   { if (done)
     {  /*if (curbit >= lastbit)("ran off the end of my bits" );*/
        return -1;
     }
     buf[0] = buf[last_byte-2];
     buf[1] = buf[last_byte-1];
     if ((count = (unsigned char)GetDataBlock(fd, buf+(2) )) == 0)
			done = TRUE;
     last_byte = (Int16)(2 + count);
     curbit = (Int16)((curbit - lastbit) + 16);
     lastbit = (Int16)((2+count)*8) ;
   }

   ret = 0;
   for (i = curbit, j = 0; j < code_size; ++i, ++j)
     ret |= ((buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;
   curbit += code_size;
   return ret;
}

Int16 LWZReadByte(FILE *fd, Int16 flag, Int16 input_code_size)
{  static Int16	fresh = FALSE;
   Int16		code, incode;
   register Int16	i;
   static Int16	code_size, set_code_size;
   static Int16	max_code, max_code_size;
   static Int16	firstcode, oldcode;
   static Int16	clear_code, end_code;
   static Int16      *sp;
   if (flag)
   { set_code_size = input_code_size;
     code_size = (Int16)(set_code_size+1);
     clear_code = (Int16)(1 << set_code_size) ;
     end_code = (Int16)(clear_code + 1);
     max_code_size = (Int16)(2*clear_code);
     max_code = (Int16)(clear_code+2);
     GetCode(fd, 0, TRUE);
     fresh = TRUE;
     for (i = 0; i < clear_code; ++i)
     { table[0][i] = 0;
       table[1][i] = i;
     }
     for (; i < (1<<BITS); ++i)
       table[0][i] = table[1][0] = 0;
     sp = stack;
     return 0;
   }
   else if (fresh)
   { fresh = FALSE;
     do
     {  firstcode = oldcode = GetCode(fd, code_size, FALSE);
     } while (firstcode == clear_code);
     return firstcode;
   }
   if (sp > stack) return *--sp;

   while ((code = GetCode(fd, code_size, FALSE)) >= 0)
   { if (code == clear_code)
     { for (i = 0; i < clear_code; ++i)
       { table[0][i] = 0;
         table[1][i] = i;
       }
       for (; i < (1<<BITS); ++i)
         table[0][i] = table[1][i] = 0;
       code_size = (Int16)(set_code_size+1);
       max_code_size = (Int16)(2*clear_code);
       max_code = (Int16)(clear_code+2);
       sp = stack;
       firstcode = oldcode = GetCode(fd, code_size, FALSE);
       return firstcode;
     }
     else if (code == end_code)
     {  Int16		count;
	unsigned char	buf[260];

	if (ZeroDataBlock) return -2;
	while ((count = GetDataBlock(fd, buf)) > 0);
	if (count != 0)
	{  /* pm_message("missing EOD in data stream (common occurence)");*/
        }
	return -2;
     }

     incode = code;
     if (code >= max_code)
     { *sp++ = firstcode;
       code = oldcode;
     }
     while (code >= clear_code)
     { *sp++ = table[1][code];
       if (code == table[0][code])
       { /*("circular table entry BIG ERROR");*/
         return -1;
       }
       code = table[0][code];
     }
     *sp++ = firstcode = table[1][code];
     if ((code = max_code) <(1<<BITS))
     { table[0][code] = oldcode;
       table[1][code] = firstcode;
       ++max_code;
       if ((max_code >= max_code_size) &&(max_code_size < (1<<BITS)))
       { max_code_size *= 2;
         ++code_size;
       }
     }
     oldcode = incode;
     if (sp > stack) return *--sp;
   }
   return code;
}
#endif /*NEWGIFD*/
#endif /*DeuTex*/

