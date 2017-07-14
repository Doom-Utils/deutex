/*
This file is Copyright © 1994-1995 Olivier Montanuy,
             Copyright © 1999-2005 André Majorel.

It may incorporate code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "deutex.h"
#include <errno.h>
#include "tools.h"
#include "endianm.h"
#include "mkwad.h"
#include "sound.h"
#include "text.h"

/*compile only for DeuTex*/
#if defined DeuTex


/*********************** WAVE *********************/

static struct RIFFHEAD
{ char  riff[4];
  Int32 length;
  char 	wave[4];
} headr;
static struct CHUNK
{ char name[4];
  Int32 size;
} headc;
static struct WAVEFMT/*format*/
{ char fmt[4];      /* "fmt " */
  Int32 fmtsize;    /*0x10*/
  Int16 tag;        /*format tag. 1=PCM*/
  Int16 channel;    /*1*/
  Int32 smplrate;
  Int32 bytescnd;   /*average bytes per second*/
  Int16 align;      /*block alignment, in bytes*/
  Int16 nbits;      /*specific to PCM format*/
}headf;
static struct WAVEDATA /*data*/
{  char data[4];    /* "data" */
   Int32 datasize;
}headw;

static void SNDsaveWave(char *file,char  *buffer,Int32 size,Int32 speed)
{
  FILE *fp;
  Int32 wsize,sz=0;
  fp=fopen(file,FOPEN_WB);
  if(fp==NULL)
  { ProgError("RW10", "%s: %s", fname (file), strerror (errno));
  }
  /*header*/
  strncpy(headr.riff,"RIFF",4);
  write_i32_le (&headr.length, 4+sizeof(struct WAVEFMT)+sizeof(struct WAVEDATA)+size);
  strncpy(headr.wave,"WAVE",4);
  fwrite(&headr,sizeof(struct RIFFHEAD),1,fp);
  strncpy(headf.fmt, "fmt ",4);
  write_i32_le (&headf.fmtsize,  sizeof(struct WAVEFMT)-8);
  write_i16_le (&headf.tag,      1);
  write_i16_le (&headf.channel,  1);
  write_i32_le (&headf.smplrate, speed);
  write_i32_le (&headf.bytescnd, speed);
  write_i16_le (&headf.align,    1);
  write_i16_le (&headf.nbits,    8);
  fwrite(&headf,sizeof(struct WAVEFMT),1,fp);
  strncpy(headw.data,"data",4);
  write_i32_le (&headw.datasize, size);
  fwrite(&headw,sizeof(struct WAVEDATA),1,fp);
  for(wsize=0;wsize<size;wsize+=sz)
  { sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE:(size-wsize);
    if(fwrite((buffer+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("RW11", "%s: write error", fname (file));
  }
  fclose(fp);
}

char  *SNDloadWaveFile(char *file, Int32 *psize, Int32 *pspeed)
{ FILE *fp;
  Int32 wsize,sz=0,smplrate,datasize;
  Int32 chunk;
  char  *data;
  fp=fopen(file,FOPEN_RB);
  if(fp==NULL)
    ProgError("RR10", "%s: %s", fname (file), strerror (errno));
  /*read RIFF HEADER*/
  if(fread(&headr,sizeof(struct RIFFHEAD),1,fp)!=1)
     ProgError("RR11", "%s: read error in header", fname (file));
  /*check RIFF header*/
  if(strncmp(headr.riff,"RIFF",4)!=0)
     ProgError("RR12", "%s: bad RIFF magic in header (%s)",
	 fname (file), short_dump (headr.riff, 4));
  if(strncmp(headr.wave,"WAVE",4)!=0)
     ProgError("RR13", "%s: bad WAVE magic in header (%s)",
	 fname (file), short_dump (headr.wave, 4));
  chunk=sizeof(struct RIFFHEAD);
  for(sz=0;;sz++)
  { if(sz>256)
       ProgError("RR14", "%s: no fmt", fname (file));
    fseek(fp,chunk,SEEK_SET);
    if(fread(&headc,sizeof(struct CHUNK),1,fp)!=1)
      ProgError("RR15", "%s: no fmt", fname (file));
    if(strncmp(headc.name,"fmt ",4)==0)break;
    /* There used to be a bug here; sizeof (struct CHUNK) had 
      its bytes swapped too. Reading .wav files on big endian
      machines must have been broken. -- AYM 1999-07-04 */
    chunk += sizeof (struct CHUNK) + peek_i32_le (&headc.size);
  }
  fseek(fp,chunk,SEEK_SET);
  fread(&headf,sizeof(struct WAVEFMT),1,fp);
  if(peek_i16_le (&headf.tag)!=1)
    ProgError("RR16", "%s: not raw data", fname (file));
  if(peek_i16_le (&headf.channel)!=1)
    ProgError("RR17", "%s: not one channel", fname (file));
  smplrate=peek_i32_le(&headf.smplrate);
  
  for(sz=0;;sz++)
  { if(sz>256)
      ProgError("RR18", "%s: no data", fname (file));
    fseek(fp,chunk,SEEK_SET);
    if(fread(&headc,sizeof(struct CHUNK),1,fp)!=1)
      ProgError("RR19", "%s: no data", fname (file));
    if(strncmp(headc.name,"data",4)==0)break;
    /* Same endianness bug as above. */
    chunk += sizeof (struct CHUNK) + peek_i32_le (&headc.size);
  }
  fseek(fp,chunk,SEEK_SET);
  if(fread(&headw,sizeof(struct WAVEDATA),1,fp)!=1)
    ProgError("RR20", "%s: no data", fname (file));
  datasize = peek_i32_le (&headw.datasize);
  /*check WAVE header*/
  if(datasize>0x1000000L)  /* AYM 2000-04-22 */
    ProgError("RR21", "%s: sample too long (%ld)",
	fname (file), (long) datasize);
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("RR22", "%s: read error in data", fname (file));
  }
  fclose(fp);
  *psize=datasize;
  *pspeed=smplrate&0xFFFFL;
  return data;
}


/***************** AU **********************/
struct AUHEAD
{ char snd[4];    /* ".snd" */
  Int32 dataloc;  /* Always big endian */
  Int32 datasize; /* Always big endian */
  Int32 format;	  /* Always big endian */
  Int32 smplrate; /* Always big endian */
  Int32 channel;  /* Always big endian */
  char  info[4];
};
static struct AUHEAD heada;
/*char data[datasize] as signed char*/

static void SNDsaveAu(char *file,char  *buffer,Int32 size,Int32 speed)
{ FILE *fp;
  Int32 i,wsize,sz=0;
  fp=fopen(file,FOPEN_WB);
  if(fp==NULL)
    ProgError("AW10", "%s: %s", fname (file), strerror (errno));
  /*header*/
  strncpy(heada.snd,".snd",4);
  write_i32_be (&heada.dataloc,  sizeof (struct AUHEAD));
  write_i32_be (&heada.datasize, size);
  write_i32_be (&heada.format,   2); /*8 bit linear*/
  write_i32_be (&heada.smplrate, speed);
  write_i32_be (&heada.channel,  1);
  heada.info[0]='\0';
  if(fwrite(&heada,sizeof(struct AUHEAD),1,fp)!=1)
    ProgError("AW11", "%s: write error", fname (file));
  for(i=0;i<size;i++)
  { buffer[i]-=0x80;
  }
  for(wsize=0;wsize<size;wsize+=sz)
  { sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE:(size-wsize);
    if(fwrite((buffer+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("AW12", "%s: write error", fname (file));
  }
    fclose(fp);
}

char *SNDloadAuFile(char *file, Int32 *psize, Int32 *pspeed)
{ FILE *fp;
  Int32 wsize,sz=0,i,smplrate,datasize;
  char *data;
  fp=fopen(file,FOPEN_RB);
  if(fp==NULL)
    ProgError("AR10", "%s: %s", fname (file), strerror (errno));
  /*read AU HEADER*/
  if(fread(&heada,sizeof(struct AUHEAD),1,fp)!=1)
    ProgError("AR11", "%s: read error in header", fname (file));

  /*check AU header*/
  if(strncmp(heada.snd,".snd",4)!=0)
    ProgError ("AR12", "%s: bad magic in header (%s)",
	fname (file), short_dump (heada.snd, 4));
  if(peek_i32_be (&heada.format) != 2)
    ProgError("AR13", "%s: not linear 8 bit", fname (file));
  if(peek_i32_be (&heada.channel)!= 1)
    ProgError("AR14", "%s: not one channel", fname (file));

  if (fseek (fp, peek_i32_be (&heada.dataloc), SEEK_SET))
    ProgError("AR15", "%s: bad header", fname (file));
  smplrate = peek_i32_be (&heada.smplrate);
  datasize = peek_i32_be (&heada.datasize);
  /*check header*/
  if(datasize>0x100000L)
	ProgError("AR16", "%s: sample too long (%ld)", fname (file), (long) datasize);
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("AR17", "%s: read error in data", fname (file));
  }
  fclose(fp);
  /*convert from signed to unsigned char*/
  for(i=0;i<datasize;i++) data[i]+=0x80;
  /*return*/
  *psize=datasize;
  *pspeed=smplrate&0xFFFFL;
  return data;
}


/*********************** VOC *********************/
#define VOCIDLEN (0x013)
static char VocId[]="Creative Voice File";
static struct VOCHEAD
{ char ident[VOCIDLEN];  /*0x13*/
  char eof;       /*0x1A*/
  short block1;   /*offset to block1=0x1A*/
  short version;  /*0x010A*/
  short version2; /*0x2229*/
}headv;
static struct VOCBLOCK1
{ char  type;   /*1=sound data.0=end block*/
  char  sizeL;  /*2+length of data*/
  char  sizeM;
  char  sizeU;
  char  rate;   /*rate = 256-(1000000/sample_rate)*/
  char  cmprs;  /*0=no compression*/
}blockv;

static void SNDsaveVoc(char *file,char  *buffer,Int32 size,Int32 speed)
{ FILE *fp;
  Int32 wsize,sz=0;
  fp=fopen(file,FOPEN_WB);
  if(fp==NULL)
    ProgError("VW10", "%s: %s", fname (file), strerror (errno));
  /*VOC header*/
  strncpy(headv.ident,VocId,VOCIDLEN);
  headv.eof=0x1A;
  headv.block1=0x1A;
  headv.version=0x10A;
  headv.version2=0x2229;
  fwrite(&headv,sizeof(struct VOCHEAD),1,fp);
  blockv.type=0x1;
  sz=(size+2)&0xFFFFFFL; /*char rate, char 0, then char[size]*/
  blockv.sizeL= (char)(sz&0xFFL);
  blockv.sizeM= (char)((sz>>8)&0xFFL);
  blockv.sizeU= (char)((sz>>16)&0xFFL);
  if(speed<=4000)speed=4000;
  blockv.rate=(char)(256-(1000000L/((long)speed)));
  blockv.cmprs=0;
  fwrite(&blockv,sizeof(struct VOCBLOCK1),1,fp);
  /*VOC data*/
  for(wsize=0;wsize<size;wsize+=sz)
  { sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE:(size-wsize);
    if(fwrite((buffer+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("VW11", "%s: write error", fname (file));
  }
  blockv.type=0;/*last block*/
  fwrite(&blockv,1,1,fp);
  fclose(fp);
}

char  *SNDloadVocFile(char *file, Int32 *psize, Int32 *pspeed)
{ FILE *fp;
  Int32 wsize,sz=0,smplrate,datasize;
  char  *data;
  fp=fopen(file,FOPEN_RB);
  if(fp==NULL)
    ProgError("VR10", "%s: %s", fname (file), strerror (errno));
  /*read VOC HEADER*/
  if(fread(&headv,sizeof(struct VOCHEAD),1,fp)!=1)
    ProgError("VR11", "%s: read error in header", fname (file));
  if(strncmp(VocId,headv.ident,VOCIDLEN)!=0)
    ProgError("VR12", "%s: bad magic", fname (file));
  if(fseek(fp,headv.block1,SEEK_SET))
    ProgError("VR13", "%s: bad header", fname (file));
  if(fread(&blockv,sizeof(struct VOCHEAD),1,fp)!=1)
    ProgError("VR14", "%s: read error in first block", fname (file));
  if(blockv.type!=1)
    ProgError("VR15", "%s: first block is not sound", fname (file));
  datasize= ((blockv.sizeU)<<16)&0xFF0000L;
  datasize+=((blockv.sizeM)<<8)&0xFF00L;
  datasize+= (blockv.sizeL)&0xFFL;
  datasize -=2;
  /*check VOC header*/
  if(datasize>0x10000L)
    ProgError("VR16", "%s: sample too long", fname (file));
  if(blockv.cmprs!=0)
    ProgError("VR17", "%s: compression not supported", fname (file));
  smplrate= (1000000L)/(256-(((int)blockv.rate)&0xFF));
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("VR18", "%s: read error in data", fname (file));
  }
  fclose(fp);
  /*should check for more blocks*/
  *psize=datasize;
  *pspeed=smplrate&0xFFFFL;
  return data;
}




/**************** generic sound *******************/
void SNDsaveSound (char *file, char *buffer, Int32 size, SNDTYPE format,
    Bool fullsnd, const char *name)
{
  char  *data;
  Int32  datasize;
  Int32  phys_size;
  Int16  type;
  Int16  headsize;
  UInt16 rate;

  headsize = sizeof (Int16) + sizeof (Int16) + sizeof (Int32);
  if (size < headsize)
  {
    Warning ("SD10", "Sound %s: lump has no header, skipping",
	lump_name (name));
    return;
  }
  type     = peek_i16_le (buffer);
  rate     = peek_u16_le (buffer + 2);
  datasize = peek_i32_le (buffer + 4);
  data	   = buffer + headsize;
  if (type!=3)
    Warning ("SD11", "Sound %s: weird type %d, extracting anyway",
	lump_name (name), type);

  phys_size = size - headsize;
  if (datasize > phys_size)
  {
    Warning ("SD12",
	"Sound %s: declared length %lu > lump size %lu, truncating",
	lump_name (name), (unsigned long) datasize, (unsigned long) phys_size);
    datasize = phys_size;
  }
  /* Sometimes the size of sound lump is greater
     than the declared sound size. */

  else if (datasize < phys_size)
  {
    if (fullsnd == TRUE)       /* Save entire lump */
      datasize = phys_size;
    else
    {
      Warning ("SD13",
	"Sound %s: lump size %lu > declared length %lu, truncating",
	lump_name (name), (unsigned long) datasize, (unsigned long) phys_size);
    }
  }

  switch (format)
  {
    case SNDWAV: SNDsaveWave (file,data,datasize,rate); break;
    case SNDAU:  SNDsaveAu   (file,data,datasize,rate); break;
    case SNDVOC: SNDsaveVoc  (file,data,datasize,rate); break;
    default:     Bug ("SD14", "sndsv %d", (int) format);
  }
}


Int32 SNDcopyInWAD (struct WADINFO *info, char *file, SNDTYPE format)
{
  Int32  size = 0;
  Int32  datasize;
  Int32  rate;
  char  *data = NULL;
  long   wadrate;

  switch (format)
  {
    case SNDWAV: data = SNDloadWaveFile (file, &datasize, &rate); break;
    case SNDAU:  data = SNDloadAuFile   (file, &datasize, &rate); break;
    case SNDVOC: data = SNDloadVocFile  (file, &datasize, &rate); break;
    default:     Bug ("SC10", "sndcw %d", (int) format);
  }
  wadrate = rate;
  switch (rate_policy)
  {
    case RP_REJECT:
      if (rate != 11025)
	ProgError ("SC11", "%s: sample rate not 11025 Hz", fname (file));
      break;

    case RP_FORCE:
      if (rate > 11025)
      {
	Warning("SC12", "%s: resampling down from %ld Hz to 11025 Hz",
	  fname (file),
	  rate);
	wadrate = 11025;
	{
	  double ratio = 11025.0 / rate;
	  long s;

	  datasize = (Int32) (ratio * datasize) + 1;
	  for (s = 0; s < datasize; s++)
	    data[s] = data[(size_t) (s / ratio + 0.5)];
	}
	data = (char *) Realloc (data, datasize);
      }
      else if (rate < 11025)
      {
	Warning("SC13", "%s: resampling up from %ld Hz to 11025 Hz",
	  fname (file),
	  rate);
	wadrate = 11025;
	{
	  double ratio = 11025.0 / rate;
	  long s;

	  datasize = (Int32) (ratio * datasize) + 1;
	  data = (char *) Realloc (data, datasize);
	  for (s = datasize - 1; s >= 0; s--)
	    data[s] = data[(size_t) (s / ratio + 0.5)];
	}
      }
      break;

    case RP_WARN:
      if (rate != 11025)
	Warning ("SC14",
	    "%s: sample rate != 11025 Hz, won't work on Doom < 1.4",
	    fname (file));
      break;

    case RP_ACCEPT:
      break;

    default:
      Bug ("SC15", "SNDcopyInWAD: rate_policy %d", (int) rate_policy);
  }

  if (datasize > 0)
  {
    size  = WADRwriteShort (info, 3);
    size += WADRwriteShort (info, wadrate);
    size += WADRwriteLong  (info, datasize);
    size += WADRwriteBytes (info, data, datasize);
  }
  Free (data);
  return size;
}


/*********** PC speaker sound effect ***********/


void SNDsavePCSound(const char *lumpname, const char *file, const char *buffer,
    Int32 size)
{ FILE *fp;
  const char *data;
  Int16 datasize,type,headsize;
  Int16 i;
  headsize = sizeof(Int16)+sizeof(Int16);
  if(size<headsize)
    ProgError("KW10", "FIXME: wrong size", fname (file));
  type     = peek_i16_le (buffer);
  datasize = peek_i16_le (buffer + 2);
  data= buffer+(sizeof(Int16)+sizeof(Int16));
  if(type!=0)
    Bug("KW11", "FIXME: not a PC sound", fname (file));
  if(size<datasize+headsize)
    ProgError("KW12", "FIXME: wrong size", fname (file));
  fp=fopen(file,FOPEN_WT); /*text file*/
  if(fp==NULL)
    ProgError("KW13", "%s: %s", fname (file), strerror (errno));
  for(i=0;i<datasize;i++)
  { fprintf(fp,"%d\n",((int)data[i])&0xFF);
#ifdef WinDeuTex
    windoze();   /*Actually Process Windows Messages*/
#endif
  }
  if (fclose(fp) != 0)
    ProgError("KW14", "%s: %s", fname (file), strerror (errno));
}

Int32 SNDcopyPCSoundInWAD(struct WADINFO *info,char *file)
{ struct TXTFILE *Txt;
  Int32 size,datasizepos;
  Int16 datasize,s;
  char c;
  Txt=TXTopenR(file, 1);
  if (Txt == NULL)
    ProgError("KR10", "%s: %s", fname (file), strerror (errno));
  size=WADRwriteShort(info,0);
  datasizepos=WADRposition(info);
  size+=WADRwriteShort(info,-1);
  datasize=0;
  while(TXTskipComment(Txt)!=FALSE)
  { s=TXTreadShort(Txt);
    if((s<0)||(s>255))
      ProgError("KR11", "%s: number out of bounds [0-255]", fname (file));
    datasize+=sizeof(char);
    c=(char)(s&0xFF);
    size+=WADRwriteBytes(info,&c,sizeof(c));
  }
  WADRsetShort(info,datasizepos,datasize);
  TXTcloseR(Txt);
  return size;
}

#endif /*DeuTex*/
