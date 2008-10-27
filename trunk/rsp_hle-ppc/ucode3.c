#ifdef __BIG_ENDIAN__

/**
 * Mupen64 hle rsp - ucode3.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifdef __WIN32__
#include <windows.h>
#else
#ifndef SIXTYFORCE_PLUGIN
#include "wintypes.h"
#endif // SIXTYFORCE_PLUGIN
#endif
#include <stdio.h>

#ifndef SIXTYFORCE_PLUGIN
#include "Rsp_#1.1.h"
#else
#include "plugin_rsp.h"
#endif // SIXTYFORCE_PLUGIN
#include <string.h>
#include "hle.h"
//static char stemp[1024];

static struct
{
   short    in[0x1000];
   short	 adpcm[0x1000];
   
   // last SETBUFF vars
   unsigned long buf_in;
   unsigned long buf_out;
   unsigned long buf_len;
   int      aux1;
   int      aux2;
   int      aux3;
   
   // envmix params
   int      env_ltvol;
   int      env_rtvol;
   int      env_ltadd;
   int      env_rtadd;
   int      env_lteff;
   int      env_rteff;
   
   unsigned long loop_addr;
} d;

static void NI(void)
{
   static int warned = 0;
   if (!warned)
     {
#ifdef DEBUG
	sprintf(stemp, "unknown ucode3 command:%x", inst1 >> 24);
#ifdef __WIN32__
	MessageBox(NULL, stemp, "warning", MB_OK);
#else
	printf("%s\n", stemp);
#endif
#endif // DEBUG
	warned = 1;
     }
}

static void adpcm_block(short *out,int src, int bits)
{
   int a,s,r,i,j,n;
   int bookbase,magnitude;
   int last1,last2,mul1,mul2;
   int downshift=11;
   unsigned char buf[16];
   short *last = out - 16;
   
   if(bits==4) n=9; else n=5;
   i=0;
   s=src>>1;
   if(src&1)
     {
        // src was not short aligned
        buf[i++]=(unsigned char)d.in[s++];
     }
   for(;i<n;)
     {
        r=d.in[s++];
        buf[i++]=r>>8;
        buf[i++]=r;
     }
   
   a=buf[0];
   magnitude=(a>>4)+11;
   bookbase=(a&15)<<4;
   
   mul2=d.adpcm[bookbase+0];
   mul1=d.adpcm[bookbase+8];
   
   // avoid internal overflow
   magnitude-=2;
   mul1>>=2;
   mul2>>=2;
   downshift-=2;
   
   // decrease volume to 50%
   mul2*=2;
   mul1*=2;
   downshift+=1;
  
   // init tmp buffer with last block
   last2=last[14];
   last1=last[15];
   
   if (bits==4)
     {
	j=1;
	int d=0;
	for(i=0;i<16;i++)
	  {
	     if(!(i&1)) d=buf[j++]<<24;
	     else d<<=4;
	     
	     s =(d>>28)<<magnitude;
	     s+=last2 * mul2;
	     s+=last1 * mul1;
	     
	     r=s>>downshift;
	     if(r<-32767) r=-32767;
	     if(r> 32767) r= 32767;
	     out[i]=r;
	     
	     last2=last1;
	     last1=r;
	  }
     }
   else
     {
	j=1;
	int d=0;
        for(i=0;i<16;i++)
	  {  
	     if(!(i&3)) d=buf[j++]<<24;
	     else d<<=2;
	     
	     s =(d>>30)<<magnitude;
	     s+=last2 * mul2;
	     s+=last1 * mul1;
	     
	     r=s>>downshift;
	     if(r<-32767) r=-32767;
	     if(r> 32767) r= 32767;
	     out[i]=r;
	     
	     last2=last1;
	     last1=r;
	  }
     }
}

static void ADPCM(void)
{
   unsigned long flags = (inst1 >> 16) & 0xFF;
   unsigned long addr = inst2 & 0xFFFFFF;
   unsigned int i;
   int src = d.buf_in, len, len0;
   short *cur = d.in + d.buf_out/2 + 16;
   
   len = len0 = ((/*inst1 & 0xFFFF*/d.buf_len)+31) >> 5;
   
   // loading adpcm state
   if (flags & 1) // init
     memset((char*)d.in + d.buf_out, 0, 32);
   else if (flags & 2) // loop
     {
	for (i=0; i<16; i++)
	  d.in[d.buf_out/2 + i] = ((short*)rsp.RDRAM)[(d.loop_addr/2) + (i^S)];
     }
   else
     {
	for (i=0; i<16; i++)
	  d.in[d.buf_out/2 + i] = ((short*)rsp.RDRAM)[(addr/2) + (i^S)];
     }
   
   // main loop
   if (flags & 4)
     {
	while (len--)
	  {
	     adpcm_block(cur,src,2);
	     src += 5;
	     cur += 16;
	  }
     }
   else
     {
	while (len--)
	  {
	     adpcm_block(cur,src,4);
	     src += 9;
	     cur += 16;
	  }
     }
   
   // save adpcm state
   for (i=0; i<16; i++)
     ((short*)rsp.RDRAM)[(addr/2) + (i^S)] = d.in[d.buf_out/2 + 16*(len0) + i];
}

static void CLEARBUFF(void)
{
   unsigned long len = inst2 & 0xFFFF;
   unsigned long dmem = inst1 & 0xFFFF;
   
   if (len)
     {
	//unsigned long out = inst2 >> 16;
	
	memset((char*)d.in + dmem, 0, len);
     }
}

static void RESAMPLE(void)
{
   unsigned long buf_in = d.buf_in;
   unsigned long buf_len = d.buf_len;
   unsigned long buf_out = d.buf_out;
   unsigned long addr = inst2 & 0xFFFFFF;
   int pitch = inst1 & 0xFFFF;
   unsigned long flags = (inst1 >> 16) & 0xFF;
   int subpos,a,b,r;
   int statedata,adpcmdata;
   int loadedstate[8];
   int savedstate[8];
   
   buf_in-=16; // first 4 shorts will come from state
   
   pitch<<=1;
   buf_len>>=1;
   buf_out>>=1;
   buf_in>>=1;
   
    // state was also first 16 samples
    // 
   adpcmdata=(d.in[buf_in+2]<<16) | d.in[buf_in+3];
   if(flags&1)
     {
        subpos=0;
        d.in[buf_in+0]=0;
        d.in[buf_in+1]=0;
        d.in[buf_in+2]=0;
        d.in[buf_in+3]=0;
     }
   else
     {
        loadedstate[0]=*(int*)(rsp.RDRAM + addr + 0);
        loadedstate[1]=*(int*)(rsp.RDRAM + addr + 4);
        loadedstate[2]=*(int*)(rsp.RDRAM + addr + 8);
        loadedstate[3]=*(int*)(rsp.RDRAM + addr + 12);
        loadedstate[4]=*(int*)(rsp.RDRAM + addr + 16);
        d.in[buf_in+0]=loadedstate[0]>>16;
        d.in[buf_in+1]=loadedstate[0];
        d.in[buf_in+2]=loadedstate[1]>>16;
        d.in[buf_in+3]=loadedstate[1];
        d.in[buf_in+4]=loadedstate[2]>>16;
        d.in[buf_in+5]=loadedstate[2];
        d.in[buf_in+6]=loadedstate[3]>>16;
        d.in[buf_in+7]=loadedstate[3];
        subpos        =loadedstate[4];
     }
   statedata=(d.in[buf_in+2]<<16) | d.in[buf_in+3];
   
   buf_len+=16;
   
   subpos&=0xffff;
   while(buf_len--)
     {
        a=(subpos>>16);
        b=(subpos&0xffff);
        if(1)
	  { // linear interpolation
	     r=d.in[buf_in+a+1]-d.in[buf_in+a+0];
	     r=((r*b)>>16)+d.in[buf_in+a+0];
	  }
        else
	  {
	     r=d.in[buf_in+a];
	  }
        d.in[buf_out++]=r;
        subpos+=pitch;
     }
   
   subpos-=pitch*16;
   
   buf_in+=(subpos>>16);
   
   // save state
     {
        a =d.in[buf_in+0]<<16;
        a|=d.in[buf_in+1]&0xffff;
        savedstate[0]=a;
        a =d.in[buf_in+2]<<16;
        a|=d.in[buf_in+3]&0xffff;
        savedstate[1]=a;
        a =d.in[buf_in+4]<<16;
        a|=d.in[buf_in+5]&0xffff;
        savedstate[2]=a;
        a =d.in[buf_in+6]<<16;
        a|=d.in[buf_in+7]&0xffff;
        savedstate[3]=a;
        savedstate[4]=subpos;
	*(int*)(rsp.RDRAM + addr + 0)=savedstate[0];
        *(int*)(rsp.RDRAM + addr + 4)=savedstate[1];
        *(int*)(rsp.RDRAM + addr + 8)=savedstate[2];
        *(int*)(rsp.RDRAM + addr + 12)=savedstate[3];
        *(int*)(rsp.RDRAM + addr + 16)=savedstate[4];
     }
}

static void FILTER(void)
{
}

static void SETBUFF(void)
{
   if((inst1 >> 16) & 0x8) // FLAGS == A_AUX
     {
	d.aux1=inst1 & 0xFFFF;
	d.aux2=inst2 >> 16;
	d.aux3=inst2 & 0xFFFF;
     }
   else
     {
	d.buf_in=inst1 & 0xFFFF;
	d.buf_out=inst2 >> 16;
	d.buf_len=inst2 & 0xFFFF;
     }
}

static void MEMLOOP(void)
{
   int in,out,count,i,j;
   count=((inst1 >> 16)&0xFF)*128;
   in   =inst1 & 0xFFFF;
   out  =(inst2 >> 16) & 0xFFFF;
   
   out>>=1;
   in>>=1;
   count>>=1;
   for(i=0;i<count;i+=64)
     {
        for(j=0;j<64;j++)
	  {
	     d.in[out+j+i]=d.in[in+j];
	  }
     }
}

static void DMEMMOVE(void)
{
   unsigned long len = inst2 & 0xFFFF;
   
   if (len)
     {
	unsigned long in = inst1 & 0xFFFF;
	unsigned long out = inst2 >> 16;
	
	memcpy((char*)d.in + out, (char*)d.in + in, len);
     }
}

static void LOADADPCM(void)
{
   unsigned long addr = (inst2 & 0xFFFFFF)/2;
   unsigned long count = (inst1 & 0xFFF)>>1;
   unsigned int i;
   for (i=0; i<count; i++)
     d.adpcm[i] = ((short*)rsp.RDRAM)[(addr) + (i^S)];
}

static void MIXER(void)
{
   int cnt=((inst1>>16)&255)*16;
   int gain=(inst1&0xffff);
   int src=inst2>>16;
   int dst=inst2&0xffff;
   int i,r;
   gain=(short)gain;
   
   dst>>=1;
   src>>=1;
   cnt>>=1;
   for(i=0;i<cnt;i++)
     {
        r=(d.in[src+i]*gain)>>15;
        r+=d.in[dst+i];
        if(r<-32767) r=-32767;
        if(r> 32767) r= 32767;
        d.in[dst+i]=r;
     }
}

static void INTERLEAVE(void)
{
   unsigned long inL = (inst2 >> 16)/2;
   unsigned long inR = (inst2 & 0xFFFF)/2;
   unsigned long out = (inst1 & 0xFFFF)/2;
   unsigned long len = ((inst1>>16)&255)*16;
   unsigned int i;
   if (!len) len = d.buf_len;
   
   for (i=0; i<len/2; i++)
     {
	d.in[out+(i*2)  ] = d.in[inL+i];
	d.in[out+(i*2)+1] = d.in[inR+i];
     }
}

static void BOOST(void)
{
   int len = inst1 & 0xFFFF;
   int gain= (inst1 >> 12) & 0xFF0;
   int buf_out = inst2>>16;
   int i,r;
   
   buf_out >>= 1;
   len >>= 1;
   for(i=0;i<len;i++)
     {
	r=(d.in[buf_out+i]*gain)>>8;
	if(r<-32767) r=-32767;
	if(r> 32767) r= 32767;
	d.in[buf_out+i]=r;
     }
}

static void SETLOOP(void)
{
   d.loop_addr = inst2 & 0xFFFFFF;
}

static void MEMHALVE(void)
{
   int in,out,count,flags,i;
   flags=(inst1 >> 16) & 0xFF;
   count=(inst1 & 0xFFFF)*2;
   in   =(inst2 >> 16) & 0xFFFF;
   out  =(inst2 & 0xFFFF);
   
   out>>=1;
   in>>=1;
   count>>=1;
   for(i=0;i<count;i++)
     {
        d.in[out+i]=d.in[in+i*2];
     }
}

static void ENVSET1(void)
{
   d.env_lteff = (inst1 >> 8) & 0xFF00;
   d.env_rteff = (short)(inst1 & 0xffff) + d.env_lteff;
   d.env_ltadd = (short)((inst2 >> 16) & 0xffff);
   d.env_rtadd = (short)(inst2 & 0xffff);
}

static void ENVMIXER(void)
{
   int src =(inst1 >> 12) & 0xFF0;
   int dst1=(inst2 >> 20) & 0xFF0;
   int dst2=(inst2 >> 12) & 0xFF0;
   int eff1=(inst2 >> 4) & 0xFF0;
   int eff2=(inst2 & 0xFF) << 4;
   int cnt =((inst1 >> 8) & 0xFF)*2 ;
   
   int i,j,jn,lx,rx;
   int lteff,rteff;
   int ltmul,ltadd;
   int rtmul,rtadd;
   int lt,rt;
   cnt>>=1;
   src>>=1;
   dst1>>=1;
   dst2>>=1;
   eff1>>=1;
   eff2>>=1;
   
   lteff=d.env_lteff;
   rteff=d.env_rteff;
   ltmul=d.env_ltvol;
   rtmul=d.env_rtvol;
   ltadd=d.env_ltadd;
   rtadd=d.env_rtadd;
   
   if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul)
     {
        // no need to mix, volumes zero
        return;
     }
   
   
   
   if(!lteff && !rteff && !ltadd && !rtadd)
     {
        // no effect mix
        for(j=0;j<cnt;j++)
	  {
	     lt=rt=d.in[src+j];
	     lt=(lt*ltmul)>>16;
	     rt=(rt*rtmul)>>16;
	     lx=lt+d.in[dst1+j];
	     rx=rt+d.in[dst2+j];
	     
	     d.in[dst1+j]=lx;
	     d.in[dst2+j]=rx;
	  }
     }
   else if(!ltadd && !rtadd)
     {
        // no volume change
        for(j=0;j<cnt;j++)
	  {
	     lt=rt=d.in[src+j];
	     lt=(lt*ltmul)>>16;
	     rt=(rt*rtmul)>>16;
	     lx=lt+d.in[dst1+j];
	     rx=rt+d.in[dst2+j];
	     
	     d.in[dst1+j]=lx;
	     d.in[dst2+j]=rx;
	     lt=(lt*lteff)>>16;
	     rt=(rt*rteff)>>16;
	     d.in[eff1+j]+=lt;
	     d.in[eff2+j]+=rt;
	  }
     }
   else
     {
        for(i=0;i<cnt;i+=8)
	  {
	     jn=i+8;
	     for(j=i;j<jn;j++)
	       {
		  lt=rt=d.in[src+j];
		  lt=(lt*ltmul)>>16;
		  rt=(rt*rtmul)>>16;
		  lx=lt+d.in[dst1+j];
		  rx=rt+d.in[dst2+j];
		  
		  d.in[dst1+j]=lx;
		  d.in[dst2+j]=rx;
		  lt=(lt*lteff)>>16;
		  rt=(rt*rteff)>>16;
		  d.in[eff1+j]+=lt;
		  d.in[eff2+j]+=rt;
	       }
	     ltmul+=ltadd;
	     rtmul+=rtadd;
	     if(ltmul>65535)
	       {
		  ltmul=65535;
		  ltadd=0;
	       }
	     else if(ltmul<0)
	       {
		  ltmul=0;
		  ltadd=0;
	       }
	     if(rtmul>65535)
	       {
		  rtmul=65535;
		  rtadd=0;
	       }
	     else if(rtmul<0)
	       {
		  rtmul=0;
		  rtadd=0;
	       }
	  }
     }
}

static void LOADBUFF(void)
{
   unsigned long addr = (inst2&0xFFFFFF)/2;
   unsigned long buf_in = inst1 & 0xFFF;
   unsigned long buf_len = (inst1 >> 12) & 0xFFF;
   unsigned int i;
   for (i=0; i<buf_len/2; i++)
     d.in[(buf_in/2) + i] = ((short*)rsp.RDRAM)[(addr) + (i^S)];
}

static void SAVEBUFF(void)
{
   unsigned long addr = (inst2&0xFFFFFF)/2;
   unsigned long buf_out = inst1 & 0xFFF;
   unsigned long buf_len = (inst1 >> 12) & 0xFFF;
   unsigned int i;
   for (i=0; i<buf_len/2; i++)
     ((short*)rsp.RDRAM)[(addr) + (i^S)] = d.in[buf_out/2 +i];
}

static void ENVSET2(void)
{
   d.env_ltvol = inst2 & 0xffff;
   d.env_rtvol = (inst2 >> 16) & 0xffff;
}

static void (*audio_command[32])(void) =
{
   NI      , ADPCM     , CLEARBUFF, NI       ,
   MIXER   , RESAMPLE  , NI       , FILTER   ,
   SETBUFF , MEMLOOP   , DMEMMOVE , LOADADPCM,
   MIXER   , INTERLEAVE, BOOST    , SETLOOP  ,
   NI      , MEMHALVE  , ENVSET1  , ENVMIXER ,
   LOADBUFF, SAVEBUFF  , ENVSET2  , NI       ,
   NI      , NI        , NI       , NI       ,
   NI      , NI        , NI       , NI
};

void ucode3(OSTask_t *task)
{
   unsigned long *p_alist = (unsigned long*)(rsp.RDRAM + task->data_ptr);
   unsigned int i;
   
   for (i=0; i<(task->data_size/4); i+=2)
     {
	inst1 = p_alist[i];
	inst2 = p_alist[i+1];
	audio_command[inst1 >> 24]();
     }
}


#endif
