/*
 *	endianio.h - file I/O with explicit endianness
 *
 *	Those functions allow to read little-endian and
 *	big-endian integers from a file regardless of the
 *	endianness of the CPU.
 *
 *	Adapted from Yadex by AYM on 1999-03-06.
 */


int fread_i16_le (FILE *fd, int16_t *buf);
int fread_i16_be (FILE *fd, int16_t *buf);
int fread_i32_le (FILE *fd, int32_t *buf);
int fread_i32_be (FILE *fd, int32_t *buf);
int fread_u16_le (FILE *fd, uint16_t *buf);
int fwrite_i16_le (FILE *fd, int16_t buf);
int fwrite_i16_be (FILE *fd, int16_t buf);
int fwrite_i32_le (FILE *fd, int32_t buf);
int fwrite_i32_be (FILE *fd, int32_t buf);
int fwrite_u16_le (FILE *fd, uint16_t buf);
