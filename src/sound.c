/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

DeuTex is Copyright © 1994-1995 Olivier Montanuy,
          Copyright © 1999-2000 André Majorel.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this library; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
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
  { ProgError("Can't open %s for writing (%s)", fname (file), strerror (errno));
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
      ProgError("%s: write error (%s)", fname (file), strerror (errno));
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
    ProgError("Can't open %s for reading (%s)", fname (file), strerror (errno));
  /*read RIFF HEADER*/
  if(fread(&headr,sizeof(struct RIFFHEAD),1,fp)!=1)
     ProgError("%s: can't read header", fname (file));
  /*check RIFF header*/
  if(strncmp(headr.riff,"RIFF",4)!=0)
     ProgError("%s: bad RIFF magic in header (%s)",
	 fname (file), short_dump (headr.riff, 4));
  if(strncmp(headr.wave,"WAVE",4)!=0)
     ProgError("%s: bad WAVE magic in header (%s)",
	 fname (file), short_dump (headr.wave, 4));
  chunk=sizeof(struct RIFFHEAD);
  for(sz=0;;sz++)
  { if(sz>256)
       ProgError("%s: no fmt", fname (file));
    fseek(fp,chunk,SEEK_SET);
    if(fread(&headc,sizeof(struct CHUNK),1,fp)!=1)
      ProgError("%s: no fmt", fname (file));
    if(strncmp(headc.name,"fmt ",4)==0)break;
    /* There used to be a bug here; sizeof (struct CHUNK) had 
      its bytes swapped too. Reading .wav files on big endian
      machines must have been broken. -- AYM 1999-07-04 */
    chunk += sizeof (struct CHUNK) + peek_i32_le (&headc.size);
  }
  fseek(fp,chunk,SEEK_SET);
  fread(&headf,sizeof(struct WAVEFMT),1,fp);
  if(peek_i16_le (&headf.tag)!=1)
    ProgError("%s: not raw data", fname (file));
  if(peek_i16_le (&headf.channel)!=1)
    ProgError("%s: not one channel", fname (file));
  smplrate=peek_i32_le(&headf.smplrate);
  
  for(sz=0;;sz++)
  { if(sz>256)
      ProgError("%s: no data", fname (file));
    fseek(fp,chunk,SEEK_SET);
    if(fread(&headc,sizeof(struct CHUNK),1,fp)!=1)
      ProgError("%s: no data", fname (file));
    if(strncmp(headc.name,"data",4)==0)break;
    /* Same endianness bug as above. */
    chunk += sizeof (struct CHUNK) + peek_i32_le (&headc.size);
  }
  fseek(fp,chunk,SEEK_SET);
  if(fread(&headw,sizeof(struct WAVEDATA),1,fp)!=1)
    ProgError("%s: no data", fname (file));
  datasize = peek_i32_le (&headw.datasize);
  /*check WAVE header*/
  if(datasize>0x100000L)
    ProgError("%s: sample too long (%ld)", fname (file), (long) datasize);
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("%s: error reading data", fname (file));
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
    ProgError("AU: can't open %s for writing (%s)", fname (file), strerror (errno));
  /*header*/
  strncpy(heada.snd,".snd",4);
  write_i32_be (&heada.dataloc,  sizeof (struct AUHEAD));
  write_i32_be (&heada.datasize, size);
  write_i32_be (&heada.format,   2); /*8 bit linear*/
  write_i32_be (&heada.smplrate, speed);
  write_i32_be (&heada.channel,  1);
  heada.info[0]='\0';
  if(fwrite(&heada,sizeof(struct AUHEAD),1,fp)!=1)ProgError("AU: write error");
  for(i=0;i<size;i++)
  { buffer[i]-=0x80;
  }
  for(wsize=0;wsize<size;wsize+=sz)
  { sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE:(size-wsize);
    if(fwrite((buffer+(wsize)),(size_t)sz,1,fp)!=1)ProgError("AU: write error");
  }
    fclose(fp);
}

char *SNDloadAuFile(char *file, Int32 *psize, Int32 *pspeed)
{ FILE *fp;
  Int32 wsize,sz=0,i,smplrate,datasize;
  char *data;
  fp=fopen(file,FOPEN_RB);
  if(fp==NULL)
    ProgError("Can't open %s for reading (%s)", fname (file), strerror (errno));
  /*read AU HEADER*/
  if(fread(&heada,sizeof(struct AUHEAD),1,fp)!=1)
    ProgError("%s: can't read header", fname (file));

  /*check AU header*/
  if(strncmp(heada.snd,".snd",4)!=0)
    ProgError ("%s: bad magic in header (%s)",
	fname (file), short_dump (heada.snd, 4));
  if(peek_i32_be (&heada.format) != 2)
    ProgError("%s: not linear 8 bit", fname (file));
  if(peek_i32_be (&heada.channel)!= 1)
    ProgError("%s: not one channel", fname (file));

  if (fseek (fp, peek_i32_be (&heada.dataloc), SEEK_SET))
    ProgError("%s: bad header", fname (file));
  smplrate = peek_i32_be (&heada.smplrate);
  datasize = peek_i32_be (&heada.datasize);
  /*check header*/
  if(smplrate!=11025)
    Warning("%s: sample rate is %u instead of 11025", fname (file), smplrate);
  if(datasize>0x100000L)
	ProgError("%s: sample too long (%ld)", fname (file), (long) datasize);
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)
      ProgError("%s: error reading data", fname (file));
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
    ProgError("VOC: can't open %s for writing", fname (file), strerror (errno));
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
    if(fwrite((buffer+(wsize)),(size_t)sz,1,fp)!=1)ProgError("VOC: write error");
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
    ProgError("VOC: can't open %s for reading (%s)", fname (file), strerror (errno));
  /*read VOC HEADER*/
  if(fread(&headv,sizeof(struct VOCHEAD),1,fp)!=1) ProgError("VOC: can't read header");
  if(strncmp(VocId,headv.ident,VOCIDLEN)!=0) ProgError("VOC: bad header");
  if(fseek(fp,headv.block1,SEEK_SET)) ProgError("VOC: bad header");
  if(fread(&blockv,sizeof(struct VOCHEAD),1,fp)!=1) ProgError("VOC: can't read block");
  if(blockv.type!=1) ProgError("VOC: first block is not sound");
  datasize= ((blockv.sizeU)<<16)&0xFF0000L;
  datasize+=((blockv.sizeM)<<8)&0xFF00L;
  datasize+= (blockv.sizeL)&0xFFL;
  datasize -=2;
  /*check VOC header*/
  if(datasize>0x10000L) ProgError("VOC: sample too long!");
  if(blockv.cmprs!=0) ProgError("VOC: compression not supported.");
  smplrate= (1000000L)/(256-(((int)blockv.rate)&0xFF));
  /*read data*/
  data=(char  *)Malloc(datasize);
  for(wsize=0;wsize<datasize;wsize+=sz)
  { sz = (datasize-wsize>MEMORYCACHE)? MEMORYCACHE:(datasize-wsize);
    if(fread((data+(wsize)),(size_t)sz,1,fp)!=1)    	ProgError("VOC: can't read sound");
  }
  fclose(fp);
  /*should check for more blocks*/
  *psize=datasize;
  *pspeed=smplrate&0xFFFFL;
  return data;
}




/**************** generic sound *******************/
void SNDsaveSound(char *file,char  *buffer,Int32 size,SNDTYPE Sound,Bool
    fullSND, const char *name)
{ char  *data;
  Int32 datasize;
  Int32 phys_size;
  Int16 type,headsize;
  UInt16 speed;
  headsize = sizeof(Int16)+sizeof(Int16)+sizeof(Int32);
  if(size<headsize)
  {
    Warning ("Sound %s: lump has no header. Skipping.");
    return;
  }
  type     = peek_i16_le (buffer);
  speed    = peek_u16_le (buffer + 2);
  datasize = peek_i32_le (buffer + 4);
  data	   = buffer+(headsize);
  if (type!=3)
    Warning ("Sound %s: weird type %d. Extracting anyway.",
	lump_name (name), type);

  phys_size = size - headsize;
  if (datasize > phys_size)
  {
    Warning (
	"Sound %s: declared sample size %lu greater than lump size %lu ;",
	lump_name (name), (unsigned long) datasize, (unsigned long) phys_size);
    Warning ("Sound %s: truncating to lump size.", lump_name (name));
    datasize = phys_size;
  }
  /* Sometimes the size of sound lump is greater
     than the declared sound size. */

  else if (datasize < phys_size)
  {
    if (fullSND == TRUE)       /* Save entire lump */
      datasize = phys_size;
    else
    {
      Warning (
	"Sound %s: lump size %lu greater than declared sample size %lu ;",
	lump_name (name), (unsigned long) datasize, (unsigned long) phys_size);
      Warning ("Sound %s: truncating to declared sample size.",
	  lump_name (name));
    }
  }

  switch(Sound)
  { case SNDWAV: SNDsaveWave(file,data,datasize,speed);break;
    case SNDAU:  SNDsaveAu(file,data,datasize,speed);break;
    case SNDVOC: SNDsaveVoc(file,data,datasize,speed);break;
    default:  Bug("sndsv");
  }
}


Int32 SNDcopyInWAD(struct WADINFO *info,char *file,SNDTYPE Sound)
{ Int32 size=0,d,s,soundsize,datasize,speed;
  char  *data=NULL;
  long rate;
  switch(Sound)
  { case SNDWAV: data=SNDloadWaveFile(file,&datasize,&speed);break;
    case SNDAU:  data=SNDloadAuFile(file,&datasize,&speed);break;
    case SNDVOC: data=SNDloadVocFile(file,&datasize,&speed);break;
    default:  Bug("sndcw");
  }
  rate = (11025L<<8)/((Int32)speed);
  soundsize= (datasize*rate)>>8;
  rate = ((Int32)speed<<8)/11025L;
  if(speed>11025)
  { Warning("shrinking %ld to 11025 sample/s",speed);
    for(s=0;s<soundsize;s++)
    { d= (s*rate)>>8;  /* (s*((Int32)speed))/11025;*/
      data[s]= data[d];
    }
    data=(char  *)Realloc(data,soundsize);
  }
  else if(speed < 11025)
  { Warning("expanding %ld to 11025 sample/s",speed);
    data=(char  *)Realloc(data,soundsize);
    for(s=soundsize-1;s>=0;s--)
    { d= (s*rate)>>8;
      data[s]= data[d];
    }
  }
  else  /*11025*/
  { soundsize=datasize;
  }
  if(soundsize>0)
  { size= WADRwriteShort(info,3);
    size+=WADRwriteShort(info,11025);
    size+=WADRwriteLong(info,soundsize);
    size+=WADRwriteBytes(info,data,soundsize);
  }
  Free(data);
  return size;
}
/*********** end soundcard sound effects  WAVE ***********/




/*********** PC speaker sound effect ***********/


void SNDsavePCSound(char *file,char  *buffer,Int32 size)
{ FILE *fp;
  char  *data;
  Int16 datasize,type,headsize;
  Int16 i;
  headsize = sizeof(Int16)+sizeof(Int16);
  if(size<headsize)	ProgError("WAV: wrong size");
  type     = peek_i16_le (buffer);
  datasize = peek_i16_le (buffer + 2);
  data= buffer+(sizeof(Int16)+sizeof(Int16));
  if(type!=0)		Bug("SNDsavePCcound: not a PC sound");
  if(size<datasize+headsize)ProgError("WAV: wrong size");
  fp=fopen(file,FOPEN_WT); /*text file*/
  if(fp==NULL)
    ProgError("Can't open %s for writing (%s)",
	fname (file), strerror (errno));
  for(i=0;i<datasize;i++)
  { fprintf(fp,"%d\n",((int)data[i])&0xFF);
#ifdef WinDeuTex
    windoze();   /*Actually Process Windows Messages*/
#endif
  }
  fclose(fp);
}

Int32 SNDcopyPCSoundInWAD(struct WADINFO *info,char *file)
{ struct TXTFILE *Txt;
  Int32 size,datasizepos;
  Int16 datasize,s;
  char c;
  Txt=TXTopenR(file);
  size=WADRwriteShort(info,0);
  datasizepos=WADRposition(info);
  size+=WADRwriteShort(info,-1);
  datasize=0;
  while(TXTskipComment(Txt)!=FALSE)
  { s=TXTreadShort(Txt);
    if((s<0)||(s>255))ProgError("SND: number out of bounds [0-255]");
    datasize+=sizeof(char);
    c=(char)(s&0xFF);
    size+=WADRwriteBytes(info,&c,sizeof(c));
  }
  WADRsetShort(info,datasizepos,datasize);
  TXTcloseR(Txt);
  return size;
}

#endif /*DeuTex*/

