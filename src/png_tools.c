/*
  This file is Copyright © 2017 contributors to the DeuTex project.

  SPDX-License-Identifier: GPL-2.0+
*/

/*
  This is a quick-and-dirty hack to read/write the grAb chunk from a
  png file A grAb chunk is a custom PNG chunk that zdoom and SLADE
  uses to store the offset of the image.
*/

#include "deutex.h"
#include "tools.h"
#include "png_tools.h"
#include "endianm.h"

uint32_t gen_grAb_crc(unsigned char *buf)
{
    uint32_t crc = 0xffffffff;
    uint32_t crc_table[256];
    uint32_t c;
    int32_t n, k;
    for (n = 0; n < 256; n++) {
        c = (uint32_t) n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = ((uint32_t) 0xedb88320) ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    for (n = 0; n < 12; n++)
        crc = crc_table[(crc ^ buf[n]) & 0xff] ^ (crc >> 8);
    return crc ^ ((uint32_t) 0xffffffff);
}

unsigned char *read_whole_image(char *file, long *sz)
{
    FILE *fd;
    unsigned char *buffer;
    size_t result;
    fd = fopen(file, FOPEN_RB);
    if (fd == NULL) {
        ProgError("XX16", "PNG image read error");
        return NULL;
    }
    fseek(fd, 0L, SEEK_END);
    *sz = ftell(fd);
    rewind(fd);
    buffer = malloc(sizeof(unsigned char) * (*sz));
    result = fread(buffer, 1, *sz, fd);
    if (result != *sz) {
        ProgError("XX12", "PNG image read error");
        *sz = 0;
        return NULL;
    }
    fclose(fd);
    return buffer;
}

bool read_grAb_chunk(unsigned char *buffer, long sz, int32_t * xofs,
                     int32_t * yofs, long *grabpos)
{
    long i;
    bool is_grab = false;
    if (buffer == NULL)
        return false;
    for (i = 4; i < sz; i++) {
        if (buffer[i] == 'I' && buffer[i + 1] == 'D' &&
            buffer[i + 2] == 'A' && buffer[i + 3] == 'T') {
            break;
        }
        if (buffer[i] == 'g' && buffer[i + 1] == 'r' &&
            buffer[i + 2] == 'A' && buffer[i + 3] == 'b') {
            i += 4;
            if (i < sz) {
                read_i32_be(buffer + i, xofs);
                i += 4;
                if (i < sz) {
                    read_i32_be(buffer + i, yofs);
                    *grabpos = i - 8;
                    is_grab = true;
                } else {
                    ProgError("XX23", "read_grAb_chunk error");
                }
            } else {
                ProgError("XX22", "read_grAb_chunk error");
            }
        }
    }
    return is_grab;
}

void read_grAb(char *file, int16_t * Xinsr, int16_t * Yinsr)
{
    long sz = 0;
    long IDATpos = 0;
    unsigned char *buffer = read_whole_image(file, &sz);
    if (buffer == NULL)
        return;
    long grabpos = 0;
    int32_t xofs = 0;
    int32_t yofs = 0;
    if (read_grAb_chunk(buffer, sz, &xofs, &yofs, &grabpos)) {
        *Xinsr = (int16_t) xofs;
        *Yinsr = (int16_t) yofs;
    } else {
        *Xinsr = INVALIDINT;
        *Yinsr = INVALIDINT;
    }
    free(buffer);
}

//To be called after the PNG is already written.
void write_grAb(char *file, int16_t Xinsr, int16_t Yinsr)
{
    long sz = 0;
    FILE *fd;
    long grabpos = 33;
    long index = 0;
    long IDATpos = 0;
    int32_t grabsz = 0;
    int32_t xofs = 0;
    int32_t yofs = 0;
    int32_t crc = 0;
    unsigned char grab_chunk[20];
    bool grab_exists;
    unsigned char *buffer = read_whole_image(file, &sz);
    //create grAb Chunk
    //size of chunk
    write_i32_be(grab_chunk, 8);
    //name of chunk
    strncpy((char *) grab_chunk + 4, "grAb", 4);
    //xoffset
    write_i32_be(grab_chunk + 8, (int32_t) Xinsr);
    //yoffset
    write_i32_be(grab_chunk + 12, (int32_t) Yinsr);
    crc = gen_grAb_crc(grab_chunk + 4);
    //crc
    write_i32_be(grab_chunk + 16, crc);
    if (buffer == NULL)
        return;
    grab_exists = read_grAb_chunk(buffer, sz, &xofs, &yofs, &grabpos);
    fd = fopen(file, FOPEN_WB);
    if (fd == NULL) {
        ProgError("XX75", "write_grAb error");
    }
    fwrite(buffer, sizeof(unsigned char), grabpos, fd);
    fwrite(grab_chunk, sizeof(char), 20, fd);
    if (!grab_exists)
        index = grabpos;
    else
        index = grabpos + 20;
    fwrite(buffer + index, sizeof(unsigned char), sz - index, fd);
    fclose(fd);
    free(buffer);
}
