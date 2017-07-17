/*
 *	wadio.h
 *	Wad low level I/O routines
 *	AYM 1999-03-06
 */


void set_output_wad_endianness (int big_endian);
void set_input_wad_endianness  (int big_endian);
extern int (* wad_write_i16) (FILE *, int16_t);
extern int (* wad_write_i32) (FILE *, int32_t);
extern int (* wad_read_i16)  (FILE *, int16_t *);
extern int (* wad_read_i32)  (FILE *, int32_t *);
int wad_read_name (FILE *fd, char name[32]);
int wad_write_name (FILE *fd, const char *name);
