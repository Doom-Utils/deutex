/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2017 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.


  SPDX-License-Identifier: GPL-2.0+
*/

#include "deutex.h"
#include <errno.h>
#include "tools.h"
#include "endianm.h"
#include "mkwad.h"
#include "sound.h"
#include "text.h"


/*********************** WAVE *********************/

static struct RIFFHEAD
{ char  riff[4];
  int32_t length;
  char 	wave[4];
} headr;
static struct CHUNK
{ char name[4];
  int32_t size;
} headc;
static struct WAVEFMT/*format*/
{ char fmt[4];      /* "fmt " */
  int32_t fmtsize;    /*0x10*/
  int16_t tag;        /*format tag. 1=PCM*/
  int16_t channel;    /*1*/
  int32_t smplrate;
  int32_t bytescnd;   /*average bytes per second*/
  int16_t align;      /*block alignment, in bytes*/
  int16_t nbits;      /*specific to PCM format*/
}headf;
static struct WAVEDATA /*data*/
{  char data[4];    /* "data" */
   int32_t datasize;
}headw;

static void SNDsaveWave(char *file,char  *buffer,int32_t size,int32_t speed)
{
  FILE *fp;
  int32_t wsize,sz=0;
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

char  *SNDloadWaveFile(char *file, int32_t *psize, int32_t *pspeed)
{ FILE *fp;
  int32_t wsize,sz=0,smplrate,datasize;
  int32_t chunk;
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


/**************** generic sound *******************/
void SNDsaveSound (char *file, char *buffer, int32_t size, SNDTYPE format,
                   const char *name)
{
  char  *data;
  int32_t  datasize;
  int32_t  phys_size;
  int16_t  type;
  int16_t  headsize;
  uint16_t rate;

  headsize = sizeof (int16_t) + sizeof (int16_t) + sizeof (int32_t);
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
  else if (datasize < phys_size)
  {
      datasize = phys_size;
  }

  switch (format) {
  case SNDWAV:
      SNDsaveWave(file, data, datasize, rate);
      break;
  default:
      Bug("SD14", "sndsv %d", (int) format);
  }
}


int32_t SNDcopyInWAD (struct WADINFO *info, char *file, SNDTYPE format)
{
  int32_t  size = 0;
  int32_t  datasize;
  int32_t  rate;
  char  *data = NULL;
  long   wadrate;

  switch (format) {
  case SNDWAV:
      data = SNDloadWaveFile(file, &datasize, &rate);
      break;
  default:
      Bug("SC10", "sndcw %d", (int) format);
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

	  datasize = (int32_t) (ratio * datasize) + 1;
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

	  datasize = (int32_t) (ratio * datasize) + 1;
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
    int32_t size)
{ FILE *fp;
  const char *data;
  int16_t datasize,type,headsize;
  int16_t i;
  headsize = sizeof(int16_t)+sizeof(int16_t);
  if(size<headsize)
    ProgError("KW10", "FIXME: wrong size", fname (file));
  type     = peek_i16_le (buffer);
  datasize = peek_i16_le (buffer + 2);
  data= buffer+(sizeof(int16_t)+sizeof(int16_t));
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

int32_t SNDcopyPCSoundInWAD(struct WADINFO *info,char *file)
{ struct TXTFILE *Txt;
  int32_t size,datasizepos;
  int16_t datasize,s;
  char c;
  Txt=TXTopenR(file, 1);
  if (Txt == NULL)
    ProgError("KR10", "%s: %s", fname (file), strerror (errno));
  size=WADRwriteShort(info,0);
  datasizepos=WADRposition(info);
  size+=WADRwriteShort(info,-1);
  datasize=0;
  while(TXTskipComment(Txt)!=false)
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
