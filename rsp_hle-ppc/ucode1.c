
#ifdef __BIG_ENDIAN__

/**
 * Mupen64 hle rsp - ucode1.c
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
#include <string.h>
#ifndef SIXTYFORCE_PLUGIN
#include "Rsp_#1.1.h"
#else
#include "plugin_rsp.h"
#endif // SIXTYFORCE_PLUGIN

#include "hle.h"

static struct
{
   unsigned long seg[256]; // 800
   short    in[0x1000];
   short	 adpcm[0x1000];
   
   
   // last SETBUFF vars
   unsigned long buf_in;  // 0(r24) (+1472?)
   unsigned long buf_out; // 2(r24) (+1472?)
   unsigned long buf_len; // 4(r24)
   int      aux1;         // 10(r24) (+1472?)
   int      aux2;         // 12(r24) (+1472?)
   int      aux3;         // 14(r24) (+1472?)
    
   // envmix params
   int      env_ltvol;    // 6(r24)
   int      env_rtvol;    // 8(r24)
   int      env_lttar;    // 16(r24)
   int      env_ltadd;    // 20(r24)
   int      env_rttar;    // 22(r24)
   int      env_rtadd;    // 26(r24)
   int      env_lteff;    // 28(r24)
   int      env_rteff;    // 30(r24)
   
   unsigned long loop_addr; // 16(r24)
} d;

//static char stemp[1024];
static short *data;

/*static void NI(void)
{
   static int warned = 0;
   if (!warned)
     {
#ifdef DEBUG
	sprintf(stemp, "unknown ucode1 command:%x", inst1 >> 24);
#ifdef __WIN32
	MessageBox(NULL, stemp, "warning", MB_OK);
#else
	printf("%s\n", stemp);
#endif
#endif // DEBUG
	warned = 1;
     }
}*/

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
   int d =0;
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
   unsigned long flags = (inst1 >> 16) & 0xFF;
   unsigned long addr = (d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF));
   unsigned int i;
   int src = d.buf_in, len, len0;
   short *cur = d.in + d.buf_out/2 + 16;
   
   len = len0 = (d.buf_len+31) >> 5;
   
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
   while (len--)
     {
	adpcm_block(cur,src);
	src += 9;
	cur += 16;
     }
   
   // save adpcm state
   for (i=0; i<16; i++)
     ((short*)rsp.RDRAM)[(addr/2) + (i^S)] = d.in[(d.buf_out/2) + (16*(len0)) + i];
}

static void CLEARBUFF(void)
{
   unsigned long len = inst2 & 0xFFFF;
   unsigned long dmem = inst1 & 0xFFFF;
   
   if (len)
     {
//	unsigned long out = inst2 >> 16;
	
	memset((char*)d.in + dmem, 0, len);
     }
}

static void ENVMIXER(void)
{
   unsigned long flags = (inst1 >> 16) & 0xFF;
   unsigned long addr = d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF);
   unsigned long buf_len = d.buf_len;
   unsigned long buf_in = d.buf_in;
   unsigned long buf_out = d.buf_out;
   unsigned long aux_out = d.aux1;
   unsigned long eff1 = d.aux2;
   unsigned long eff2 = d.aux3;
   int i,j,jn;
   int lteff,rteff;
   int ltmul,ltadd,lttar;
   int rtmul,rtadd,rttar;
   int lt,rt;
   
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
   if (d.buf_len)
     {
	unsigned long addr = (d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF))/2;
	unsigned int i;
	for (i=0; i<d.buf_len/2; i++)
	  d.in[d.buf_in/2 + i] = ((short*)rsp.RDRAM)[addr + (i^S)];
     }
}

static void RESAMPLE(void)
{
   unsigned long buf_in = d.buf_in;
   unsigned long buf_len = d.buf_len;
   unsigned long buf_out = d.buf_out;
   unsigned long addr = d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF);
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

static void SAVEBUFF(void)
{
   if (d.buf_len)
     {
	unsigned long addr = (d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF))/2;
	unsigned int i;
	for (i=0; i<d.buf_len/2; i++)
	  ((short*)rsp.RDRAM)[(addr) + (i^S)] = d.in[(d.buf_out/2) +i];
     }
}

static void SEGMENT(void)
{
   d.seg[inst2>>24] = inst2 & 0xFFFFFF;
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

static void SETVOL(void)
{
   unsigned int flags = ((inst1 >> 16) & 0xFF);
   unsigned short vol = (short)(inst1 & 0xFFFF);
   unsigned short voltgt = (short)(inst2 >> 16);
   unsigned short volrate = (short)(inst2 & 0xFFFF);
   
   switch(flags)
     {
      case  8:
	d.env_lteff=volrate;
	d.env_rteff=volrate;
	// rate should also affect something, pan?
	break;
      case  6:
	d.env_ltvol=vol;
	break;
      case  4:
	d.env_rtvol=vol;
	break;
      case  2:
	d.env_lttar=vol;
	if(voltgt)
	  d.env_ltadd=8*(volrate&0xffff);
	else
	  d.env_ltadd=8*((short)volrate);
	if(d.env_ltadd>65535)
	  d.env_ltadd=65535;
	if(d.env_ltadd<-65535)
	  d.env_ltadd=-65535;
	break;
      case  0:
	d.env_rttar=vol;
	if(voltgt)
	  d.env_rtadd=8*(volrate&0xffff);
	else
	  d.env_rtadd=8*((short)volrate);
	if(d.env_rtadd>65535)
	  d.env_rtadd=65535;
	if(d.env_rtadd<-65535)
	  d.env_rtadd=-65535;
	break;
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
   unsigned long addr = (d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF))/2;
   unsigned long count = (inst1 & 0xFFFF)>>1;
   unsigned int i;
   for (i=0; i<count; i++)
     d.adpcm[i] = ((short*)rsp.RDRAM)[(addr) + (i^S)];
}

static void MIXER(void)
{
   if (d.buf_len)
     {
	short gain = (short)(inst1 & 0xFFFF);
	unsigned long dmemo = (inst2 & 0xFFFF)/2;
	unsigned long dmemi = (inst2 >> 16)/2;
	unsigned int i;
	
	for (i=0; i<(d.buf_len/2); i++)
	  {
	     d.in[dmemo+i] = (((long)d.in[dmemo+i] * (long)data[6^S]*2)+0x8000
			      +((long)d.in[dmemi+i] * (long)gain     *2))>>16;
	  }
     }
}

static void INTERLEAVE(void)
{
   if (d.buf_len)
     {
	unsigned long inL = (inst2 >> 16)/2;
	unsigned long inR = (inst2 & 0xFFFF)/2;
	unsigned int i;
	
	for (i=0; i<(d.buf_len/2); i++)
	  {
	     d.in[d.buf_out/2+(i*2)  ] = d.in[inL+i];
	     d.in[d.buf_out/2+(i*2)+1] = d.in[inR+i];
	  }
     }
}

static void POLEF(void)
{
}

static void SETLOOP(void)
{
   d.loop_addr = (d.seg[inst2 >> 24] + (inst2 & 0xFFFFFF));
}

static void (*audio_command[16])(void) =
{
   NOP     , ADPCM     , CLEARBUFF, ENVMIXER ,
   LOADBUFF, RESAMPLE  , SAVEBUFF , SEGMENT  ,
   SETBUFF , SETVOL    , DMEMMOVE , LOADADPCM,
   MIXER   , INTERLEAVE, POLEF    , SETLOOP
};

void ucode1(OSTask_t *task)
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

void init_ucode1()
{
   int i;
   for (i=0; i<256; i++) d.seg[i] = 0;
   for (i=0; i<0x1000; i++) d.in[i] = 0;
   for (i=0; i<0x1000; i++) d.adpcm[i] = 0;
}
#endif
