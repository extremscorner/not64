#ifdef __BIG_ENDIAN__

/**
 * Mupen64 hle rsp - ucode2.c
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

static short *data;
//static char stemp[1024];

static struct
{
   unsigned long seg[256];
   short in[0x1000];
   short adpcm[0x1000];
   
   // envmix params
   int      env_ltvol;
   int      env_rtvol;
   int      env_lttar;
   int      env_ltadd;
   int      env_rttar;
   int      env_rtadd;
   int      env_lteff;
   int      env_rteff;
   
   unsigned long loop_addr;
   unsigned long mp3_addr;
} d;

/*static void NI(void)
{}*/

static void NOP(void)
{
}

static void adpcm_block(short *out,int src)
{
   int a,s,r,i,j;
   int bookbase,magnitude;
   int last1,last2,mul1,mul2;
   int downshift=11;
   unsigned char buf[16];
   short *last = out - 16;
   
   i=0;
   s=src>>1;
   if(src&1)
     {
        // src was not short aligned
        buf[i++]=(unsigned char)d.in[s++];
     }
   for(;i<9;)
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
   int d=0;
   j=1;
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

static void ADPCM(void)
{
   unsigned long flags = (inst2 >> 28) & 0xF;
   unsigned long addr = inst1 & 0xFFFFFF;
   unsigned int i;
   int src = (inst2 >> 12) & 0xF, len, len0;
   short *cur = d.in + (inst2 & 0xFFF)/2 + 16;
   
   len = len0 = (((inst2 >> 16) & 0xFFF)+31) >> 5;
   
   // loading adpcm state
   if (flags & 1) // init
     memset((char*)d.in + (inst2 & 0xFFF), 0, 32);
   else if (flags & 2) // loop
     {
	for (i=0; i<16; i++)
	  d.in[(inst2 & 0xFFF)/2 + i] = ((short*)rsp.RDRAM)[(d.loop_addr/2) + (i^S)];
     }
   else
     {
	for (i=0; i<16; i++)
	  d.in[(inst2 & 0xFFF)/2 + i] = ((short*)rsp.RDRAM)[(addr/2) + (i^S)];
     }
   
   // main loop
   while (len--)
     {
	adpcm_block(cur,src);
	src += 9;
	cur += 16;
     }
   
   // save adpcm state
   for (i=0; i<16; i++)
     ((short*)rsp.RDRAM)[(addr/2) + (i^S)] = d.in[(inst2 & 0xFFF)/2 + 16*(len0) + i];
}

static void CLEARBUFF(void)
{
   unsigned long len = inst2 & 0xFFFF;
   unsigned long dmem = inst1 & 0xFFFF;
   
   memset((char*)d.in + dmem, 0, len);
}

static void ENVMIXER(void)
{
   unsigned long flags = (inst1 >> 16) & 0xFF;
   unsigned long addr = inst2 & 0xFFFFFF;
   unsigned long buf_len = 0x170;
   unsigned long buf_in = 0;
   unsigned long buf_out = 0x4e0;
   unsigned long aux_out = 0x650;
   unsigned long eff1 = 0x7c0;
   unsigned long eff2 = 0x930;
   int i,j,jn;
   int lteff,rteff;
   int ltmul,ltadd,lttar;
   int rtmul,rtadd,rttar;
   int lt,rt;
   
   if(d.env_ltadd> 65535) d.env_ltadd=65535;
   if(d.env_ltadd<-65535) d.env_ltadd=-65535;
   if(d.env_rtadd> 65535) d.env_rtadd=65535;
   if(d.env_rtadd<-65535) d.env_rtadd=-65535;
   d.env_rtvol=inst1 & 0xFFFF;
   
   if (!(flags & 1))
     {
	d.env_ltvol=*(int*)(rsp.RDRAM + addr + 0);
	d.env_rtvol=*(int*)(rsp.RDRAM + addr + 4);
	d.env_lteff=*(int*)(rsp.RDRAM + addr + 8);
	d.env_rteff=*(int*)(rsp.RDRAM + addr + 12);
	d.env_ltadd=*(int*)(rsp.RDRAM + addr + 16);
	d.env_rtadd=*(int*)(rsp.RDRAM + addr + 20);
	d.env_lttar=*(int*)(rsp.RDRAM + addr + 24);
	d.env_rttar=*(int*)(rsp.RDRAM + addr + 28);
     }
   
   buf_len>>=1;
   buf_in>>=1;
   buf_out>>=1;
   aux_out>>=1;
   eff1>>=1;
   eff2>>=1;
   
   lteff=d.env_lteff<<1;
   rteff=d.env_rteff<<1;
   ltmul=d.env_ltvol<<1;
   rtmul=d.env_rtvol<<1;
   ltadd=d.env_ltadd<<1;
   rtadd=d.env_rtadd<<1;
   lttar=d.env_lttar<<1;
   rttar=d.env_rttar<<1;
   
   if(ltadd==65535 && ltadd>0)
     {
        ltmul=lttar;
        ltadd=0;
     }
   if(rtadd==65535 && rtadd>0)
     {
        rtmul=rttar;
        rtadd=0;
     }
   
   if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul==0)
     {
        // no need to mix, volumes zero
        return;
     }
   
   if(!lteff && !rteff && !ltadd && !rtadd)
     {
        // no effect mix
        for(j=0;j<(int)buf_len;j++)
	  {
	     lt=rt=d.in[buf_in+j];
	     lt=(lt*ltmul)>>16;
	     rt=(rt*rtmul)>>16;
	     d.in[buf_out+j]+=lt;
	     d.in[aux_out+j]+=rt;
	  }
     }
   else if(!ltadd && !rtadd)
     {
        // no volume change
        for(j=0;j<(int)buf_len;j++)
	  {
	     lt=rt=d.in[buf_in+j];
	     lt=(lt*ltmul)>>16;
	     rt=(rt*rtmul)>>16;
	     d.in[buf_out+j]+=lt;
	     d.in[aux_out+j]+=rt;
	     lt=(lt*lteff)>>16;
	     rt=(rt*rteff)>>16;
	     d.in[eff1+j]+=lt;
	     d.in[eff2+j]+=rt;
	  }
     }
   else
     {
        // full mix (no target)
        for(i=0;i<(int)buf_len;i+=8)
	  {
	     jn=i+8;
	     for(j=i;j<jn;j++)
	       {
		  lt=rt=d.in[buf_in+j];
		  lt=(lt*ltmul)>>16;
		  rt=(rt*rtmul)>>16;
		  d.in[buf_out+j]+=lt;
		  d.in[aux_out+j]+=rt;
		  lt=(lt*lteff)>>16;
		  rt=(rt*rteff)>>16;
		  d.in[eff1+j]+=lt;
		  d.in[eff2+j]+=rt;
	       }
	     ltmul+=ltadd;
	     if(ltadd>0)
	       {
		  if(ltmul>lttar)
		    {
		       ltmul=lttar;
		       ltadd=0;
		    }
	       }
	     else
	       {
		  if(ltmul<lttar)
		    {
		       ltmul=lttar;
		       ltadd=0;
		    }
	       }
	     rtmul+=rtadd;
	     if(rtadd>0)
	       {
		  if(rtmul>rttar)
		    {
		       rtmul=rttar;
		       rtadd=0;
		    }
	       }
	     else
	       {
		  if(rtmul<rttar)
		    {
		       rtmul=rttar;
		       rtadd=0;
		    }
	       }
	  }
     }
   
   d.env_lteff=lteff>>1;
   d.env_rteff=rteff>>1;
   d.env_ltadd=ltadd>>1;
   d.env_rtadd=rtadd>>1;
   d.env_ltvol=ltmul>>1;
   d.env_rtvol=rtmul>>1;
   d.env_lttar=lttar>>1;
   d.env_rttar=rttar>>1;
   
   *(int*)(rsp.RDRAM + addr + 0) = d.env_ltvol;
   *(int*)(rsp.RDRAM + addr + 4) = d.env_rtvol;
   *(int*)(rsp.RDRAM + addr + 8) = d.env_lteff;
   *(int*)(rsp.RDRAM + addr + 12) = d.env_rteff;
   *(int*)(rsp.RDRAM + addr + 16) = d.env_ltadd;
   *(int*)(rsp.RDRAM + addr + 20) = d.env_rtadd;
   *(int*)(rsp.RDRAM + addr + 24) = d.env_lttar;
   *(int*)(rsp.RDRAM + addr + 28) = d.env_rttar;
   
   d.env_lteff=0;
   d.env_rteff=0;
   d.env_ltadd=0;
   d.env_rtadd=0;
   d.env_lttar=0;
   d.env_rttar=0;
   d.env_ltvol=0;
   d.env_rtvol=0;
}

static void LOADBUFF(void)
{
   unsigned long addr = inst2/2;
   unsigned long buf_in = inst1 & 0xFFF;
   unsigned long buf_len = (inst1 >> 12) & 0xFFF;
   unsigned int i;
   for (i=0; i<buf_len/2; i++)
     d.in[(buf_in/2) + i] = ((short*)rsp.RDRAM)[addr + (i^S)];
}

static void RESAMPLE(void)
{
   unsigned long buf_in = (inst2 >> 2) & 0xFFF;
   unsigned long buf_len = 0x170;
   unsigned long buf_out = (inst2 & 3) ? 0x170 : 0;
   unsigned long addr = inst1 & 0xFFFFFF;
   int pitch = (inst2 >> 14) & 0xFFFF;
   unsigned long flags = (inst2 >> 30) & 3;
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

static void SAVEBUFF(void)
{
   unsigned long addr = inst2/2;
   unsigned long buf_out = inst1 & 0xFFF;
   unsigned long buf_len = (inst1 >> 12) & 0xFFF;
   unsigned int i;
   for (i=0; i<buf_len/2; i++)
     ((short*)rsp.RDRAM)[addr + (i^S)] = d.in[(buf_out/2) +i];
}

static void mp3_func(short *m, short *mem)
{
   int i;
   m[16*8+0] = m[0*8+0] + m[8*8+0];
   m[17*8+0] = m[1*8+0] + m[9*8+0];
   m[18*8+0] = m[2*8+0] + m[10*8+0];
   m[19*8+0] = m[3*8+0] + m[11*8+0];
   m[20*8+0] = m[4*8+0] + m[12*8+0];
   m[21*8+0] = m[5*8+0] + m[13*8+0];
   m[22*8+0] = m[6*8+0] + m[14*8+0];
   m[23*8+0] = m[7*8+0] + m[15*8+0];
   
   m[24*8+0] = m[0*8+0] - m[8*8+0];
   m[25*8+0] = m[1*8+0] - m[9*8+0];
   m[26*8+0] = m[2*8+0] - m[10*8+0];
   m[27*8+0] = m[3*8+0] - m[11*8+0];
   m[28*8+0] = m[4*8+0] - m[12*8+0];
   m[29*8+0] = m[5*8+0] - m[13*8+0];
   m[30*8+0] = m[6*8+0] - m[14*8+0];
   m[31*8+0] = m[7*8+0] - m[15*8+0];
   for (i=0; i<8; i++)
     {
	m[8*8+i] = mem[8+i];
	m[9*8+i] = mem[16+i];
     }
   m[24*8+0] = (short)(((long)m[24*8+0] * (unsigned short)m[8*8+0])>>16);
   m[25*8+0] = (short)(((long)m[25*8+0] * (unsigned short)m[8*8+2])>>16);
   m[26*8+0] = (short)(((long)m[26*8+0] * (unsigned short)m[8*8+6])>>16);
   m[27*8+0] = (short)(((long)m[27*8+0] * (unsigned short)m[8*8+4])>>16);
   m[28*8+0] = (short)(((long)m[28*8+0] * (unsigned short)m[9*8+5])>>16);
   m[29*8+0] = (short)(((long)m[29*8+0] * (unsigned short)m[9*8+3])>>16);
   m[30*8+0] = (short)(((long)m[30*8+0] * (unsigned short)m[8*8+7])>>16);
   m[31*8+0] = (short)(((long)m[31*8+0] * (unsigned short)m[9*8+1])>>16);
   
   m[4*8+0] = m[16*8+0] - m[20*8+0];
   m[5*8+0] = m[17*8+0] - m[21*8+0];
   m[0*8+0] = m[16*8+0] + m[20*8+0];
   m[1*8+0] = m[17*8+0] + m[21*8+0];
   m[4*8+0] = (short)(((long)m[4*8+0] * (unsigned short)m[8*8+1])>>16);
   m[5*8+0] = (short)(((long)m[5*8+0] * (unsigned short)m[8*8+5])>>16);
   
   m[6*8+0] = m[18*8+0] - m[22*8+0];
   m[7*8+0] = m[19*8+0] - m[23*8+0];
   m[2*8+0] = m[18*8+0] + m[22*8+0];
   m[3*8+0] = m[19*8+0] + m[23*8+0];
   m[6*8+0] = (short)(((long)m[6*8+0] * (unsigned short)m[9*8+4])>>16);
   m[7*8+0] = (short)(((long)m[7*8+0] * (unsigned short)m[9*8+0])>>16);
   
   m[12*8+0] = m[24*8+0] - m[28*8+0];
   m[13*8+0] = m[25*8+0] - m[29*8+0];
   m[10*8+0] = m[26*8+0] + m[30*8+0];
   m[11*8+0] = m[27*8+0] + m[31*8+0];
   m[14*8+0] = m[26*8+0] - m[30*8+0];
   m[15*8+0] = m[27*8+0] - m[31*8+0];
   m[12*8+0] = (short)(((long)m[12*8+0] * (unsigned short)m[8*8+1])>>16);
   m[13*8+0] = (short)(((long)m[13*8+0] * (unsigned short)m[8*8+5])>>16);
   m[14*8+0] = (short)(((long)m[14*8+0] * (unsigned short)m[9*8+4])>>16);
   m[15*8+0] = (short)(((long)m[15*8+0] * (unsigned short)m[9*8+0])>>16);
   
   m[8*8+0] = m[24*8+0] + m[28*8+0];
   m[9*8+0] = m[25*8+0] + m[29*8+0];
   m[16*8+0] = m[0*8+0] + m[2*8+0];
   m[17*8+0] = m[1*8+0] + m[3*8+0];
   m[18*8+0] = m[0*8+0] - m[2*8+0];
   for (i=0; i<8; i++)
     {
	m[0*8+i] = mem[1*8+i];
	m[2*8+i] = mem[2*8+i];
     }
   m[19*8+0] = m[1*8+0] - m[3*8+0];
   m[20*8+0] = m[4*8+0] + m[6*8+0];
   m[21*8+0] = m[5*8+0] + m[7*8+0];
   m[22*8+0] = m[4*8+0] - m[6*8+0];
   m[23*8+0] = m[5*8+0] - m[7*8+0];
   
   m[18*8+0] = (short)(((long)m[18*8+0] * (unsigned short)m[0*8+3])>>16);
   m[19*8+0] = (short)(((long)m[19*8+0] * (unsigned short)m[2*8+2])>>16);
   m[22*8+0] = (short)(((long)m[22*8+0] * (unsigned short)m[0*8+3])>>16);
   m[23*8+0] = (short)(((long)m[23*8+0] * (unsigned short)m[2*8+2])>>16);
   
   m[26*8+0] = m[8*8+0] - m[10*8+0];
   m[27*8+0] = m[9*8+0] - m[11*8+0];
   m[24*8+0] = m[8*8+0] + m[10*8+0];
   m[25*8+0] = m[9*8+0] + m[11*8+0];
   m[26*8+0] = (short)(((long)m[26*8+0] * (unsigned short)m[0*8+3])>>16);
   m[27*8+0] = (short)(((long)m[27*8+0] * (unsigned short)m[2*8+2])>>16);
   
   m[30*8+0] = m[12*8+0] - m[14*8+0];
   m[31*8+0] = m[13*8+0] - m[15*8+0];
   m[28*8+0] = m[12*8+0] + m[14*8+0];
   m[29*8+0] = m[13*8+0] + m[15*8+0];
   m[30*8+0] = (short)(((long)m[30*8+0] * (unsigned short)m[0*8+3])>>16);
   m[31*8+0] = (short)(((long)m[31*8+0] * (unsigned short)m[2*8+2])>>16);
}

static void MP3(void)
{
   int cpt = 0;
   int i;
   short mem[0x1000];
   short m[32*8];
   int r1, r2, r3, r8, r9, r10, r11, r12, r13, r14, r19, r20, r21, r22;
   
   for (i=0; i<8; i++) mem[i] = data[i^S];
   for (i=0; i<2192/2; i++) mem[8+i] = data[(704/2) + (i^S)];
   for (i=0; i<1088/2; i++) mem[2208/2+i] = *(unsigned short*)(rsp.RDRAM + d.mp3_addr + (i^S)*2);
   
   r14 = 2208;
   r13 = r14 + 544;
   r12 = inst1 & 30;
   r20 = 1152;
   r3 = 392;
   r2 = inst2&0xFFFFFF;
   r1 = 3304;
   r22 = r21 = inst2 & 0xFFFFFF;
   r21 += 8;
   
   do
     {
	for (i=0; i<r3/2; i++) mem[r1/2+i] = *(unsigned short*)(rsp.RDRAM + r2 + (i^S)*2);
	r3 = 384;
	r20 -= 384;
	r19 = 3696;
	cpt = 0;
	do
	  {
	     r14 = (r14 & 0xFFFFFFE0) | r12;
	     r13 = (r13 & 0xFFFFFFE0) | r12;
	     
	     m[0*8+0] = mem[3312/2+32*cpt+0];
	     m[31*8+0] = mem[3312/2+32*cpt+31];
	     m[1*8+0] = mem[3312/2+32*cpt+1];
	     m[30*8+0] = mem[3312/2+32*cpt+30];
	     m[2*8+0] = mem[3312/2+32*cpt+3];
	     m[28*8+0] = mem[3312/2+32*cpt+28];
	     m[0*8+0] += m[31*8+0];
	     m[3*8+0] = mem[3312/2+32*cpt+2];
	     m[29*8+0] = mem[3312/2+32*cpt+29];
	     m[1*8+0] += m[30*8+0];
	     m[4*8+0] = mem[3312/2+32*cpt+7];
	     m[24*8+0] = mem[3312/2+32*cpt+24];
	     m[2*8+0] += m[28*8+0];
	     m[5*8+0] = mem[3312/2+32*cpt+6];
	     m[25*8+0] = mem[3312/2+32*cpt+25];
	     m[3*8+0] += m[29*8+0];
	     m[6*8+0] = mem[3312/2+32*cpt+4];
	     m[27*8+0] = mem[3312/2+32*cpt+27];
	     m[4*8+0] += m[24*8+0];
	     m[7*8+0] = mem[3312/2+32*cpt+5];
	     m[26*8+0] = mem[3312/2+32*cpt+26];
	     m[5*8+0] += m[25*8+0];
	     m[8*8+0] = mem[3312/2+32*cpt+15];
	     m[16*8+0] = mem[3312/2+32*cpt+16];
	     m[6*8+0] += m[27*8+0];
	     m[9*8+0] = mem[3312/2+32*cpt+14];
	     m[17*8+0] = mem[3312/2+32*cpt+17];
	     m[7*8+0] += m[26*8+0];
	     m[10*8+0] = mem[3312/2+32*cpt+12];
	     m[19*8+0] = mem[3312/2+32*cpt+19];
	     m[8*8+0] += m[16*8+0];
	     m[11*8+0] = mem[3312/2+32*cpt+13];
	     m[18*8+0] = mem[3312/2+32*cpt+18];
	     m[9*8+0] += m[17*8+0];
	     m[12*8+0] = mem[3312/2+32*cpt+8];
	     m[23*8+0] = mem[3312/2+32*cpt+23];
	     m[10*8+0] += m[19*8+0];
	     m[13*8+0] = mem[3312/2+32*cpt+9];
	     m[22*8+0] = mem[3312/2+32*cpt+22];
	     m[11*8+0] += m[18*8+0];
	     m[14*8+0] = mem[3312/2+32*cpt+11];
	     m[20*8+0] = mem[3312/2+32*cpt+20];
	     m[12*8+0] += m[23*8+0];
	     m[15*8+0] = mem[3312/2+32*cpt+10];
	     m[21*8+0] = mem[3312/2+32*cpt+21];
	     m[13*8+0] += m[22*8+0];
	     m[14*8+0] += m[20*8+0];
	     m[15*8+0] += m[21*8+0];
	     
	     mp3_func(m, mem);
	     
	     r8  = r14 + 256;
	     r9  = r14 + 512;
	     r10 = r13 + 256;
	     r11 = r13 + 512;
	     
	     for (i=0; i<8; i++) m[0*8+i] = mem[5*8+i];
	     m[11*8+0] = m[16*8+0] - m[17*8+0];
	     m[16*8+0] = m[16*8+0] + m[17*8+0];
	     m[1*8+0] = 0;
	     m[16*8+0] = m[1*8+0] - m[16*8+0];
	     m[11*8+0] = (short)(((long)m[11*8+0] * (unsigned short)m[0*8+0])>>16);
	     m[2*8+0] = m[18*8+0] + m[19*8+0];
	     m[3*8+0] = m[18*8+0] - m[19*8+0];
	     
	     mem[r14/2] = m[11*8+0];
	     m[11*8+0] = m[1*8+0] - m[11*8+0];
	     mem[r11/2] = m[16*8+0];
	     mem[r13/2] = m[11*8+0];
	     m[2*8+0] = m[1*8+0] - m[2*8+0];
	     m[3*8+0] = (short)((((long)m[3*8+0] * (unsigned short)m[0*8+3])>>16)
				+((long)m[3*8+0] * (long)m[0*8+2]));
	     mem[r10/2] = m[2*8+0];
	     m[3*8+0] = m[3*8+0] + m[2*8+0];
	     mem[r8/2] = m[3*8+0];
	     m[5*8+0] = m[20*8+0] - m[21*8+0];
	     m[4*8+0] = m[20*8+0] + m[21*8+0];
	     m[6*8+0] = m[22*8+0] + m[23*8+0];
	     m[7*8+0] = m[22*8+0] - m[23*8+0];
	     m[5*8+0] = (short)((((long)m[5*8+0] * (unsigned short)m[0*8+3])>>16)
				+((long)m[5*8+0] * (long)m[0*8+2]));
	     m[4*8+0] = m[1*8+0] - m[4*8+0];
	     m[7*8+0] = (short)((((long)m[7*8+0] * (unsigned short)m[0*8+5])>>16)
				+((long)m[7*8+0] * (long)m[0*8+4]));
	     mem[r11/2-64] = m[4*8+0];
	     m[5*8+0] = m[5*8+0] - m[4*8+0];
	     m[4*8+0] = m[1*8+0] - m[4*8+0];
	     m[6*8+0] = m[6*8+0] + m[6*8+0];
	     m[7*8+0] = m[7*8+0] - m[5*8+0];
	     m[4*8+0] = m[4*8+0] - m[6*8+0];
	     m[5*8+0] = m[5*8+0] - m[6*8+0];
	     mem[r9/2-64] = m[7*8+0];
	     mem[r10/2-64] = m[4*8+0];
	     mem[r8/2-64] = m[5*8+0];
	     
	     m[9*8+0] = m[24*8+0] - m[25*8+0];
	     m[8*8+0] = m[24*8+0] + m[25*8+0];
	     m[9*8+0] = (short)((((long)m[9*8+0] * (unsigned short)m[0*8+3])>>16)
				+((long)m[9*8+0] * (long)m[0*8+2]));
	     m[11*8+0] = m[26*8+0] - m[27*8+0];
	     m[10*8+0] = m[26*8+0] + m[27*8+0];
	     m[13*8+0] = m[28*8+0] - m[29*8+0];
	     m[2*8+0] = m[8*8+0] + m[9*8+0];
	     m[11*8+0] = (short)((((long)m[11*8+0] * (unsigned short)m[0*8+5])>>16)
				 +((long)m[11*8+0] * (long)m[0*8+4]));
	     m[13*8+0] = (short)((((long)m[13*8+0] * (unsigned short)m[0*8+5])>>16)
				 +((long)m[13*8+0] * (long)m[0*8+4]));
	     m[12*8+0] = m[28*8+0] + m[29*8+0];
	     m[11*8+0] = m[11*8+0] + m[2*8+0];
	     m[10*8+0] = m[10*8+0] + m[10*8+0];
	     m[13*8+0] = m[13*8+0] - m[2*8+0];
	     m[12*8+0] = m[12*8+0] + m[12*8+0];
	     m[14*8+0] = m[30*8+0] + m[31*8+0];
	     m[15*8+0] = m[30*8+0] - m[31*8+0];
	     m[3*8+0] = m[8*8+0] + m[10*8+0];
	     m[14*8+0] = m[14*8+0] + m[14*8+0];
	     m[13*8+0] = m[13*8+0] + m[12*8+0];
	     m[15*8+0] = (short)((((long)m[15*8+0] * (unsigned short)m[0*8+7])>>16)
				 +((long)m[15*8+0] * (long)m[0*8+6]));
	     m[14*8+0] = m[14*8+0] + m[14*8+0];
	     m[15*8+0] = m[15*8+0] - m[11*8+0];
	     m[14*8+0] = m[14*8+0] - m[3*8+0];
	     m[14*8+0] = m[1*8+0] - m[14*8+0];
	     m[17*8+0] = m[13*8+0] - m[10*8+0];
	     m[9*8+0] = m[9*8+0] + m[14*8+0];
	     m[11*8+0] = m[11*8+0] - m[2*8+0];
	     
	     mem[r14/2+32] = m[9*8+0];
	     m[11*8+0] = m[11*8+0] - m[13*8+0];
	     mem[r8/2-32] = m[17*8+0];
	     m[12*8+0] = m[8*8+0] - m[12*8+0];
	     mem[r8/2+32] = m[11*8+0];
	     m[8*8+0] = m[1*8+0] - m[8*8+0];
	     mem[r9/2-32] = m[15*8+0];
	     m[10*8+0] = m[1*8+0] - m[10*8+0];
	     mem[r10/2+32] = m[12*8+0];
	     mem[r11/2-32] = m[8*8+0];
	     m[10*8+0] = m[10*8+0] - m[12*8+0];
	     mem[r13/2+32] = m[14*8+0];
	     mem[r10/2-32] = m[10*8+0];
	     m[0*8+0] = mem[3312/2+32*cpt+0];
	     m[31*8+0] = mem[3312/2+32*cpt+31];
	     m[1*8+0] = mem[3312/2+32*cpt+1];
	     m[30*8+0] = mem[3312/2+32*cpt+30];
	     m[2*8+0] = mem[3312/2+32*cpt+3];
	     m[28*8+0] = mem[3312/2+32*cpt+28];
	     m[3*8+0] = mem[3312/2+32*cpt+2];
	     m[29*8+0] = mem[3312/2+32*cpt+29];
	     m[0*8+0] = m[0*8+0] - m[31*8+0];
	     for (i=0; i<8; i++) m[31*8+i] = mem[3*8+i];
	     m[1*8+0] = m[1*8+0] - m[30*8+0];
	     m[2*8+0] = m[2*8+0] - m[28*8+0];
	     m[4*8+0] = mem[3312/2+32*cpt+7];
	     m[3*8+0] = m[3*8+0] - m[29*8+0];
	     m[24*8+0] = mem[3312/2+32*cpt+24];
	     m[0*8+0] = (short)(((long)m[0*8+0] * (unsigned short)m[31*8+0])>>16);
	     m[5*8+0] = mem[3312/2+32*cpt+6];
	     m[1*8+0] = (short)(((long)m[1*8+0] * (unsigned short)m[31*8+1])>>16);
	     m[25*8+0] = mem[3312/2+32*cpt+25];
	     m[2*8+0] = (short)(((long)m[2*8+0] * (unsigned short)m[31*8+3])>>16);
	     m[6*8+0] = mem[3312/2+32*cpt+4];
	     m[3*8+0] = (short)(((long)m[3*8+0] * (unsigned short)m[31*8+2])>>16);
	     m[27*8+0] = mem[3312/2+32*cpt+27];
	     m[0*8+0] = m[0*8+0] + m[0*8+0];
	     m[7*8+0] = mem[3312/2+32*cpt+5];
	     m[1*8+0] = m[1*8+0] + m[1*8+0];
	     m[26*8+0] = mem[3312/2+32*cpt+26];
	     m[2*8+0] = m[2*8+0] + m[2*8+0];
	     for (i=0; i<8; i++) m[30*8+i] = mem[4*8+i];
	     m[3*8+0] = m[3*8+0] + m[3*8+0];
	     m[8*8+0] = mem[3312/2+32*cpt+15];
	     m[4*8+0] = m[4*8+0] - m[24*8+0];
	     m[16*8+0] = mem[3312/2+32*cpt+16];
	     m[5*8+0] = m[5*8+0] - m[25*8+0];
	     m[9*8+0] = mem[3312/2+32*cpt+14];
	     m[6*8+0] = m[6*8+0] - m[27*8+0];
	     m[17*8+0] = mem[3312/2+32*cpt+17];
	     m[7*8+0] = m[7*8+0] - m[26*8+0];
	     m[10*8+0] = mem[3312/2+32*cpt+12];
	     m[4*8+0] = (short)(((long)m[4*8+0] * (unsigned short)m[31*8+7])>>16);
	     m[19*8+0] = mem[3312/2+32*cpt+19];
	     m[5*8+0] = (short)(((long)m[5*8+0] * (unsigned short)m[31*8+6])>>16);
	     m[11*8+0] = mem[3312/2+32*cpt+13];
	     m[6*8+0] = (short)(((long)m[6*8+0] * (unsigned short)m[31*8+4])>>16);
	     m[18*8+0] = mem[3312/2+32*cpt+18];
	     
	     m[7*8+0] = (short)(((long)m[7*8+0] * (unsigned short)m[31*8+5])>>16);
	     m[4*8+0] = m[4*8+0] + m[4*8+0];
	     m[5*8+0] = m[5*8+0] + m[5*8+0];
	     m[6*8+0] = m[6*8+0] + m[6*8+0];
	     m[7*8+0] = m[7*8+0] + m[7*8+0];
	     
	     m[12*8+0] = mem[3312/2+32*cpt+8];
	     m[8*8+0] = m[8*8+0] - m[16*8+0];
	     m[23*8+0] = mem[3312/2+32*cpt+23];
	     m[9*8+0] = m[9*8+0] - m[17*8+0];
	     m[13*8+0] = mem[3312/2+32*cpt+9];
	     m[10*8+0] = m[10*8+0] - m[19*8+0];
	     m[22*8+0] = mem[3312/2+32*cpt+22];
	     m[11*8+0] = m[11*8+0] - m[18*8+0];
	     m[14*8+0] = mem[3312/2+32*cpt+11];
	     m[8*8+0] = (short)(((long)m[8*8+0] * (unsigned short)m[30*8+7])>>16);
	     m[20*8+0] = mem[3312/2+32*cpt+20];
	     m[9*8+0] = (short)(((long)m[9*8+0] * (unsigned short)m[30*8+6])>>16);
	     m[15*8+0] = mem[3312/2+32*cpt+10];
	     m[10*8+0] = (short)(((long)m[10*8+0] * (unsigned short)m[30*8+4])>>16);
	     m[21*8+0] = mem[3312/2+32*cpt+21];
	     m[11*8+0] = (short)(((long)m[11*8+0] * (unsigned short)m[30*8+5])>>16);
	     m[12*8+0] = m[12*8+0] - m[23*8+0];
	     m[13*8+0] = m[13*8+0] - m[22*8+0];
	     m[14*8+0] = m[14*8+0] - m[20*8+0];
	     m[15*8+0] = m[15*8+0] - m[21*8+0];
	     m[12*8+0] = (short)(((long)m[12*8+0] * (unsigned short)m[30*8+0])>>16);
	     m[13*8+0] = (short)(((long)m[13*8+0] * (unsigned short)m[30*8+1])>>16);
	     m[14*8+0] = (short)(((long)m[14*8+0] * (unsigned short)m[30*8+3])>>16);
	     m[15*8+0] = (short)(((long)m[15*8+0] * (unsigned short)m[30*8+2])>>16);
	     m[12*8+0] = m[12*8+0] + m[12*8+0];
	     m[13*8+0] = m[13*8+0] + m[13*8+0];
	     m[15*8+0] = m[15*8+0] + m[15*8+0];
	     
	     mp3_func(m, mem);
	     
	     for (i=0; i<8; i++)
	       {
		  m[2*8+i] = m[18*8+i] + m[19*8+i];
		  m[3*8+i] = m[18*8+i] - m[19*8+i];
		  m[19*8+i] = mem[5*8+i];
		  m[18*8+i] = mem[2*8+i];
	       }
	     for (i=0; i<8; i++)
	       {
		  m[1*8+i] = (short)(((long)m[16*8+i] * (unsigned short)m[19*8+0]
				      + (long)m[17*8+i] * (long)m[19*8+1]*2)>>16);
		  m[0*8+i] = (short)(((long)m[16*8+i] * (unsigned short)m[18*8+6]
				      + (long)m[17*8+i] * (unsigned short)m[18*8+6])>>16);
		  m[16*8+i] = 0;
		  m[2*8+i] = m[16*8+i] - m[2*8+i];
		  m[3*8+i] = (short)((((long)m[3*8+i] * (unsigned short)m[19*8+3])>>16)
				     +((long)m[3*8+i] * (long)m[19*8+2]));
		  m[4*8+i] = m[20*8+i] + m[21*8+i];
		  m[4*8+i] = m[4*8+i] + m[0*8+i];
		  m[5*8+i] = m[20*8+i] - m[21*8+i];
		  m[5*8+i] = (short)((((long)m[5*8+i] * (unsigned short)m[19*8+3])>>16)
				     +((long)m[5*8+i] * (long)m[19*8+2]));
		  m[5*8+i] = m[5*8+i] + m[1*8+i];
		  m[6*8+i] = m[22*8+i] + m[23*8+i];
		  m[6*8+i] = m[6*8+i] + m[6*8+i];
		  m[6*8+i] = m[6*8+i] + m[0*8+i];
		  m[6*8+i] = m[6*8+i] - m[2*8+i];
		  m[7*8+i] = m[22*8+i] - m[23*8+i];
		  m[7*8+i] = (short)((((long)m[7*8+i] * (unsigned short)m[19*8+5])>>16)
				     +((long)m[7*8+i] * (long)m[19*8+4]));
		  m[7*8+i] = m[7*8+i] + m[0*8+i];
		  m[0*8+i] = m[16*8+i] - m[0*8+i];
		  m[7*8+i] = m[7*8+i] + m[1*8+i];
	       }
	     mem[r11/2-16] = m[0*8+0];
	     for (i=0; i<8; i++)
	       {
		  m[7*8+i] = m[7*8+i] + m[3*8+i];
		  m[0*8+i] = m[16*8+i] - m[0*8+i];
		  m[8*8+i] = m[24*8+i] + m[25*8+i];
		  m[9*8+i] = m[24*8+i] - m[25*8+i];
		  m[9*8+i] = (short)((((long)m[9*8+i] * (unsigned short)m[19*8+3])>>16)
				     +((long)m[9*8+i] * (long)m[19*8+2]));
		  m[10*8+i] = m[26*8+i] + m[27*8+i];
		  m[10*8+i] = m[10*8+i] + m[10*8+i];
		  m[10*8+i] = m[10*8+i] + m[8*8+i];
		  m[11*8+i] = m[26*8+i] - m[27*8+i];
		  m[11*8+i] = (short)((((long)m[11*8+i] * (unsigned short)m[19*8+5])>>16)
				      +((long)m[11*8+i] * (long)m[19*8+4]));
		  m[11*8+i] = m[11*8+i] + m[8*8+i];
		  m[11*8+i] = m[11*8+i] + m[9*8+i];
		  m[12*8+i] = m[28*8+i] + m[29*8+i];
		  m[12*8+i] = m[12*8+i] + m[12*8+i];
		  m[12*8+i] = m[4*8+i] - m[12*8+i];
		  m[13*8+i] = m[28*8+i] - m[29*8+i];
		  m[13*8+i] = (short)((((long)m[13*8+i] * (unsigned short)m[19*8+5])>>16)
				      +((long)m[13*8+i] * (long)m[19*8+4]));
	       }
	     mem[r10/2+16] = m[12*8+0];
	     for (i=0; i<8; i++)
	       {
		  m[13*8+i] = m[13*8+i] - m[12*8+i];
		  m[13*8+i] = m[13*8+i] - m[5*8+i];
		  m[14*8+i] = m[30*8+i] + m[31*8+i];
		  m[14*8+i] = m[14*8+i] + m[14*8+i];
		  m[14*8+i] = m[14*8+i] + m[14*8+i];
		  m[14*8+i] = m[6*8+i] - m[14*8+i];
		  m[15*8+i] = m[30*8+i] - m[31*8+i];
		  m[15*8+i] = (short)((((long)m[15*8+i] * (unsigned short)m[19*8+7])>>16)
				      +((long)m[15*8+i] * (long)m[19*8+6]));
	       }
	     mem[r13/2+16] = m[14*8+0];
	     for (i=0; i<8; i++)
	       {
		  m[15*8+i] = m[15*8+i] - m[7*8+i];
		  m[14*8+i] = m[14*8+i] + m[1*8+i];
	       }
	     mem[r14/2+16] = m[14*8+0];
	     mem[r9/2-16] = m[15*8+0];
	     for (i=0; i<8; i++)
	       {
		  m[9*8+i] = m[9*8+i] + m[10*8+i];
		  m[1*8+i] = m[1*8+i] + m[6*8+i];
		  m[6*8+i] = m[10*8+i] - m[6*8+i];
		  m[1*8+i] = m[9*8+i] - m[1*8+i];
	       }
	     mem[r13/2+48] = m[6*8+0];
	     for (i=0; i<8; i++)
	       {
		  m[10*8+i] = m[10*8+i] + m[2*8+i];
		  m[10*8+i] = m[4*8+i] - m[10*8+i];
	       }
	     mem[r10/2-48] = m[10*8+0];
	     for (i=0; i<8; i++) m[12*8+i] = m[2*8+i] - m[12*8+i];
	     mem[r10/2-16] = m[12*8+0];
	     for (i=0; i<8; i++) m[5*8+i] = m[4*8+i] + m[5*8+i];
	     for (i=0; i<8; i++) m[4*8+i] = m[8*8+i] - m[4*8+i];
	     mem[r10/2+48] = m[4*8+0];
	     for (i=0; i<8; i++) m[0*8+i] = m[0*8+i] - m[8*8+i];
	     mem[r11/2-48] = m[0*8+0];
	     for (i=0; i<8; i++) m[7*8+i] = m[7*8+i] - m[11*8+i];
	     mem[r9/2-48] = m[7*8+0];
	     for (i=0; i<8; i++) m[11*8+i] = m[11*8+i] - m[3*8+i];
	     mem[r14/2+48] = m[1*8+0];
	     for (i=0; i<8; i++) m[11*8+i] = m[11*8+i] - m[5*8+i];
	     mem[r8/2+48] = m[11*8+0];
	     for (i=0; i<8; i++) m[3*8+i] = m[3*8+i] - m[13*8+i];
	     mem[r8/2+16] = m[3*8+0];
	     for (i=0; i<8; i++) m[13*8+i] = m[13*8+i] + m[2*8+i];
	     mem[r8/2-16] = m[13*8+0];
	     for (i=0; i<8; i++) m[2*8+i] = m[5*8+i] - m[2*8+i];
	     for (i=0; i<8; i++) m[2*8+i] = m[2*8+i] - m[9*8+i];
	     mem[r8/2-48] = m[2*8+0];
	     
	     r9 = r14 & 0xFFFFFFE0;
	     r10 = 128 - r12;
	     
	     for (i=0; i<8; i++)
	       {
		  m[2*8+i] = mem[r9/2+i];
		  m[1*8+i] = mem[r10/2+i];
		  m[4*8+i] = mem[r9/2+1*8+i];
		  m[3*8+i] = mem[r10/2+1*8+i];
		  m[6*8+i] = mem[r9/2+2*8+i];
		  m[5*8+i] = mem[r10/2+4*8+i];
		  m[8*8+i] = mem[r9/2+3*8+i];
		  m[7*8+i] = mem[r10/2+5*8+i];
		  
		  m[2*8+i] = (((long)m[2*8+i] * (long)m[1*8+i]*2)+0x8000)>>16;
		  m[4*8+i] = (((long)m[4*8+i] * (long)m[3*8+i]*2)+0x8000)>>16;
		  m[6*8+i] = (((long)m[6*8+i] * (long)m[5*8+i]*2)+0x8000)>>16;
		  m[8*8+i] = (((long)m[8*8+i] * (long)m[7*8+i]*2)+0x8000)>>16;
	       }
	     
	     r11 = 3304;
	     for (i=0; i<4; i++) m[20*8+i] = mem[r11/2+i];
	     r11 = 8;
	     
	     do
	       {
		  r10 += 128;
		  r9 += 64;
		  
		  for (i=0; i<8; i++)
		    {
		       m[9*8+i] = m[2*8+i] + m[2*8+((i&(~3))|2)];
		       m[10*8+i] = m[4*8+i] + m[4*8+((i&(~3))|2)];
		       m[1*8+i] = mem[r10/2+i];
		       m[11*8+i] = m[6*8+i] + m[6*8+((i&(~3))|2)];
		       m[12*8+i] = m[8*8+i] + m[8*8+((i&(~3))|2)];
		       m[3*8+i] = mem[r10/2+1*8+i];
		       m[13*8+i] = m[2*8+i] + m[2*8+(i|3)];
		       m[14*8+i] = m[4*8+i] + m[4*8+(i|3)];
		       m[5*8+i] = mem[r10/2+4*8+i];
		       m[15*8+i] = m[6*8+i] + m[6*8+(i|3)];
		       m[16*8+i] = m[8*8+i] + m[8*8+(i|3)];
		       m[7*8+i] = mem[r10/2+5*8+i];
		    }
		  for (i=0; i<8; i++)
		    {
		       m[0*8+i] = m[9*8+i] + m[13*8+((i&(~3))|1)];
		       m[17*8+i] = m[10*8+i] + m[14*8+((i&(~3))|1)];
		       m[2*8+i] = mem[r9/2+0*8+i];
		       m[18*8+i] = m[11*8+i] + m[15*8+((i&(~3))|1)];
		       m[4*8+i] = mem[r9/2+1*8+i];
		       m[19*8+i] = m[12*8+i] + m[16*8+((i&(~3))|1)];
		       m[6*8+i] = mem[r9/2+2*8+i];
		       m[0*8+i] = m[0*8+i] + m[17*8+i];
		       m[8*8+i] = mem[r9/2+3*8+i];
		       m[18*8+i] = m[18*8+i] + m[19*8+i];
		       m[2*8+i] = (((long)m[2*8+i] * (long)m[1*8+i]*2)+0x8000)>>16;
		    }
		    {
		       short m0_4 = m[0*8+4];
		       short m18_4 = m[18*8+4];
		       for (i=0; i<8; i++)
			 {
			    m[0*8+i] = m[0*8+i] + m0_4;
			    m[4*8+i] = (((long)m[4*8+i] * (long)m[3*8+i]*2)+0x8000)>>16;
			    m[18*8+i] = m[18*8+i] + m18_4;
			    m[6*8+i] = (((long)m[6*8+i] * (long)m[5*8+i]*2)+0x8000)>>16;
			    m[8*8+i] = (((long)m[8*8+i] * (long)m[7*8+i]*2)+0x8000)>>16;
			 }
		    }
		  mem[r19/2] = m[0*8+0];
		  mem[r19/2+1] = m[18*8+0];
		  r11--;
		  r19+=4;
	       } while (r11 > 0);
	     
	     r8 = r12 & 2;
	     r9 -= 64;
	     r11 = 3304;
	     
	     for (i=0; i<8; i++)
	       {
		  m[9*8+i] = m[2*8+i] + m[2*8+((i&(~3))|2)];
		  m[10*8+i] = m[4*8+i] + m[4*8+((i&(~3))|2)];
		  m[13*8+i] = m[2*8+i] + m[2*8+(i|3)];
		  m[14*8+i] = m[4*8+i] + m[4*8+(i|3)];
		  m[0*8+i] = m[9*8+i] + m[10*8+i];
	       }
	       {
		  short m0_4 = m[0*8+4];
		  for (i=0; i<8; i++)
		    {
		       m[0*8+i] = m[0*8+i] + m0_4;
		       m[17*8+i] = m[13*8+i] + m[14*8+i];
		    }
	       }
	       {
		  short m17_5 = m[17*8+5];
		  for (i=0; i<8; i++)
		    {
		       m[17*8+i] = m[17*8+i] + m17_5;
		       m[0*8+i] = (short)((((long)m[0*8+i] * (unsigned short)m[20*8+1])>>16)
					  +((long)m[0*8+i] * (long)m[20*8+0]));
		       m[17*8+i] = (short)((((long)m[17*8+i] * (unsigned short)m[20*8+1])>>16)
					   +((long)m[17*8+i] * (long)m[20*8+0]));
		    }
	       }
	     mem[r19/2] = m[0*8+0];
	     if (!r8)
	       {
		  m[20*8+2] = mem[r11/2];
		  m[20*8+3] = mem[r11/2+1];
		  mem[r19/2] = m[17*8+1];
	       }
	     r10 = 1214 - r12;
	     
	     for (i=0; i<8; i++)
	       {
		  m[2*8+i] = mem[r9/2+2*8+i];
		  m[1*8+i] = mem[r10/2+0*8+i];
		  m[4*8+i] = mem[r9/2+3*8+i];
		  m[3*8+i] = mem[r10/2+1*8+i];
		  m[6*8+i] = mem[r9/2+0*8+i];
		  m[5*8+i] = mem[r10/2+4*8+i];
		  m[8*8+i] = mem[r9/2+1*8+i];
		  m[7*8+i] = mem[r10/2+5*8+i];
		  m[2*8+i] = (((long)m[2*8+i] * (long)m[1*8+i]*2)+0x8000)>>16;
		  m[4*8+i] = (((long)m[4*8+i] * (long)m[3*8+i]*2)+0x8000)>>16;
		  m[6*8+i] = (((long)m[6*8+i] * (long)m[5*8+i]*2)+0x8000)>>16;
		  m[8*8+i] = (((long)m[8*8+i] * (long)m[7*8+i]*2)+0x8000)>>16;
	       }
	     r11 = 8;
	     do
	       {
		  r10 += 128;
		  r9 -= 64;
		  
		  for (i=0; i<8; i++)
		    {
		       m[9*8+i] = m[2*8+i] + m[2*8+((i&(~3))|2)];
		       m[10*8+i] = m[4*8+i] + m[4*8+((i&(~3))|2)];
		       m[1*8+i] = mem[r10/2+i];
		       m[11*8+i] = m[6*8+i] + m[6*8+((i&(~3))|2)];
		       m[12*8+i] = m[8*8+i] + m[8*8+((i&(~3))|2)];
		       m[3*8+i] = mem[r10/2+1*8+i];
		       m[13*8+i] = m[2*8+i] + m[2*8+(i|3)];
		       m[14*8+i] = m[4*8+i] + m[4*8+(i|3)];
		       m[5*8+i] = mem[r10/2+4*8+i];
		       m[15*8+i] = m[6*8+i] + m[6*8+(i|3)];
		       m[16*8+i] = m[8*8+i] + m[8*8+(i|3)];
		       m[7*8+i] = mem[r10/2+5*8+i];
		    }
		  for (i=0; i<8; i++)
		    {
		       m[0*8+i] = m[9*8+i] - m[13*8+((i&(~3))|1)];
		       m[17*8+i] = m[10*8+i] - m[14*8+((i&(~3))|1)];
		       m[2*8+i] = mem[r9/2+2*8+i];
		       m[18*8+i] = m[11*8+i] - m[15*8+((i&(~3))|1)];
		       m[4*8+i] = mem[r9/2+3*8+i];
		       m[19*8+i] = m[12*8+i] - m[16*8+((i&(~3))|1)];
		       m[6*8+i] = mem[r9/2+0*8+i];
		       m[0*8+i] = m[0*8+i] + m[17*8+i];
		       m[8*8+i] = mem[r9/2+1*8+i];
		       m[18*8+i] = m[18*8+i] + m[19*8+i];
		       m[2*8+i] = (((long)m[2*8+i] * (long)m[1*8+i]*2)+0x8000)>>16;
		    }
		    {
		       short m0_4 = m[0*8+4];
		       short m18_4 = m[18*8+4];
		       for (i=0; i<8; i++)
			 {
			    m[0*8+i] = m[0*8+i] + m0_4;
			    m[4*8+i] = (((long)m[4*8+i] * (long)m[3*8+i]*2)+0x8000)>>16;
			    m[18*8+i] = m[18*8+i] + m18_4;
			    m[6*8+i] = (((long)m[6*8+i] * (long)m[5*8+i]*2)+0x8000)>>16;
			    m[8*8+i] = (((long)m[8*8+i] * (long)m[7*8+i]*2)+0x8000)>>16;
			 }
		    }
		  mem[r19/2+1] = m[0*8+0];
		  mem[r19/2+2] = m[18*8+0];
		  r11--;
		  r19+=4;
	       } while (r11 > 0);
	     r8 = r19 + 2;
	     
	     for (i=0; i<8; i++)
	       {
		  m[0*8+i] = mem[r19/2-4*8+i];
		  m[17*8+i] = mem[r19/2-3*8+i];
		  m[2*8+i] = mem[r8/2-2*8+i];
		  m[4*8+i] = mem[r8/2-1*8+i];
		  m[0*8+i] = (short)((((long)m[0*8+i] * (unsigned short)m[20*8+1])>>16)
				     +((long)m[0*8+i] * (long)m[20*8+0]));
		  mem[r19/2-4*8+i] = m[0*8+i];
		  m[2*8+i] = (short)((((long)m[2*8+i] * (unsigned short)m[20*8+3])>>16)
				     +((long)m[2*8+i] * (long)m[20*8+2]));
		  mem[r8/2-2*8+i] = m[2*8+i];
		  m[17*8+i] = (short)((((long)m[17*8+i] * (unsigned short)m[20*8+1])>>16)
				      +((long)m[17*8+i] * (long)m[20*8+0]));
		  mem[r19/2-3*8+i] = m[17*8+i];
		  m[4*8+i] = (short)((((long)m[4*8+i] * (unsigned short)m[20*8+3])>>16)
				     +((long)m[4*8+i] * (long)m[20*8+2]));
		  mem[r8/2-1*8+i] = m[4*8+i];
	       }
	     r11 = r14;
	     r14 = r13;
	     r13 = r11;
	     r12 -= 2;
	     r12 &= 30;
	     
	     cpt++;
	  } while (cpt < 384/64);
	for (i=0; i<r3/2; i++) *(unsigned short*)(rsp.RDRAM + r22 + (i^S)*2) = mem[3696/2+i];
	r22 += 384;
	r21 += 384;
	r2 = r21;
	r1 = 3312;
     } while (r20 > 0);
   
   for (i=0; i<1088/2; i++) *(unsigned short*)(rsp.RDRAM + d.mp3_addr + (i^S)*2) = mem[2208/2+i];
}

static void MP3DATA(void)
{
   d.mp3_addr = inst2 & 0xFFFFFF;
}

static void SETVOL(void)
{
   unsigned int flags = ((inst1 >> 16) & 0xFF);
   unsigned short vol = (short)(inst1 & 0xFFFF);
   unsigned short voltgt = (short)(inst2 >> 16);
   unsigned short volrate = (short)(inst2 & 0xFFFF);
   
   switch(flags)
     {
      case 6:
	d.env_lteff=volrate;
	d.env_rteff=volrate;
	d.env_ltvol=vol;
	break;
      case 0:
	d.env_ltadd=8*(short)voltgt;
	d.env_lttar=vol;
	break;
      case 4:
	d.env_rtadd=8*(short)voltgt;
	d.env_rttar=vol;
	break;
     }
}

static void DMEMMOVE(void)
{
   unsigned long len = inst2 & 0xFFFF;
   unsigned long in = inst1 & 0xFFFF;
   unsigned long out = inst2 >> 16;
   
   memcpy((char*)d.in + out, (char*)d.in + in, len);
}

static void LOADADPCM(void)
{
   unsigned long addr = inst2/2;
   unsigned long count = (inst1 & 0xFFF)>>1;
   unsigned int i;
   for (i=0; i<count; i++)
     d.adpcm[i] = ((short*)rsp.RDRAM)[addr + (i^S)];
}

static void MIXER(void)
{
}

static void INTERLEAVE(void)
{
   unsigned int i;
   
   for (i=0; i<(0x170/2); i++)
     {
	d.in[(i*2)  ] = d.in[0x4e0/2+i];
	d.in[(i*2)+1] = d.in[0x650/2+i];
     }
}

static void POLEF(void)
{
}

static void SETLOOP(void)
{
   d.loop_addr = inst2 & 0xFFFFFF;
}

static void (*audio_command[16])(void) =
{
   NOP     , ADPCM     , CLEARBUFF, ENVMIXER , 
   LOADBUFF, RESAMPLE  , SAVEBUFF , MP3      , 
   MP3DATA , SETVOL    , DMEMMOVE , LOADADPCM, 
   MIXER   , INTERLEAVE, POLEF    , SETLOOP
};

void ucode2(OSTask_t *task)
{
   unsigned long *p_alist = (unsigned long*)(rsp.RDRAM + task->data_ptr);
   unsigned int i;
   
   data = (short*)(rsp.RDRAM + task->ucode_data);
   
   for (i=0; i<(task->data_size/4); i+=2)
     {
	inst1 = p_alist[i];
	inst2 = p_alist[i+1];
	audio_command[inst1 >> 24]();
     }
}

void init_ucode2()
{
   int i;
   for (i=0; i<256; i++) d.seg[i] = 0;
   for (i=0; i<0x1000; i++) d.in[i] = 0;
   for (i=0; i<0x1000; i++) d.adpcm[i] = 0;
}

#endif
