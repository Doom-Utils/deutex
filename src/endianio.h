/*
 *	endianio.h
 *	Endianness-independant file I/O.
 *
 *	Those functions allow to read little-endian and
 *	big-endian integers from a file regardless of the
 *	endianness of the CPU.
 *
 *	Adapted from Yadex by AYM on 1999-03-06.
 */


/* Use the names DeuTex provides */
#define i16 Int16
#define i32 Int32
#define u16 UInt16
#define u32 UInt32

int fread_i16_le (FILE *fd, i16 *buf);
int fread_i16_be (FILE *fd, i16 *buf);
int fread_i32_le (FILE *fd, i32 *buf);
int fread_i32_be (FILE *fd, i32 *buf);
int fread_u16_le (FILE *fd, u16 *buf);
int fwrite_i16_le (FILE *fd, i16 buf);
int fwrite_i16_be (FILE *fd, i16 buf);
int fwrite_i32_le (FILE *fd, i32 buf);
int fwrite_i32_be (FILE *fd, i32 buf);
int fwrite_u16_le (FILE *fd, u16 buf);

