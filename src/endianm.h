/*
 *	endianm.h
 *	Endianness-independant memory access.
 *
 *	Those functions allow to retrieve little-endian and
 *	big-endian integers from a memory area regardless of
 *	the endianness of the CPU.
 *
 *	AYM 1999-07-04
 */


/* Use the names DeuTex provides */
#ifndef i16
#  define i16 Int16
#endif

#ifndef i32
#  define i32 Int32
#endif

#ifndef u16
#  define u16 UInt16
#endif

#ifndef u32
#  define u32 UInt32
#endif

void read_i16_le (const void *ptr, i16 *buf);
void read_i32_le (const void *ptr, i32 *buf);
i16 peek_i16_le (const void *ptr);
u16 peek_u16_le (const void *ptr);
i32 peek_i32_be (const void *ptr);
i32 peek_i32_le (const void *ptr);
void write_i16_le (void *ptr, i16 val);
void write_i32_be (void *ptr, i32 val);
void write_i32_le (void *ptr, i32 val);

