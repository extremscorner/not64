/**
 * Mupen64 - interupt.c
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

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#ifndef __WIN32__
//#include <SDL.h>
#include "../main/winlnxdefs.h"
#else
#include <windows.h>
#endif

#include "interupt.h"
#include "../gc_memory/memory.h"
#include "r4300.h"
#include "macros.h"
#include "exception.h"
#include "../main/plugin.h"
#include "../main/guifuncs.h"
#include "../main/savestates.h"

unsigned long next_vi;
int vi_field=0;

typedef struct _interupt_queue
{
   int type;
   unsigned long count;
   struct _interupt_queue *next;
} interupt_queue;

static interupt_queue *q = NULL;

void clear_queue()
{
   while(q != NULL)
     {
	interupt_queue *aux = q->next;
	free(q);
	q = aux;
     }
}

void print_queue()
{
   interupt_queue *aux;
   //if (Count < 0x7000000) return;
   printf("------------------ %x\n", (unsigned int)Count);
   aux = q;
   while (aux != NULL)
     {
	printf("Count:%x, %x\n", (unsigned int)aux->count, aux->type);
	aux = aux->next;
     }
   printf("------------------\n");
   //getchar();
}

static int SPECIAL_done = 0;

int before_event(unsigned long evt1, unsigned long evt2, int type2)
{
   if(evt1 - Count < 0x80000000)
     {
	if(evt2 - Count < 0x80000000)
	  {
	     if((evt1 - Count) < (evt2 - Count)) return 1;
	     else return 0;
	  }
	else
	  {
	     if((Count - evt2) < 0x10000000)
	       {
		  switch(type2)
		    {
		     case SPECIAL_INT:
		       if(SPECIAL_done) return 1;
		       else return 0;
		       break;
		     default:
		       return 0;
		    }
	       }
	     else return 1;
	  }
     }
   else return 0;
}

void add_interupt_event(int type, unsigned long delay)
{
   unsigned long count = Count + delay/**2*/;
   int special = 0;
   
   if(type == SPECIAL_INT /*|| type == COMPARE_INT*/) special = 1;
   if(Count > 0x80000000) SPECIAL_done = 0;
/*   
   if (get_event(type)) {
      printf("two events of type %x in queue\n", type);
   }*/
   interupt_queue *aux = q;
   
   //if (type == PI_INT)
     //{
	//delay = 0;
	//count = Count + delay/**2*/;
     //}
   
   if (q == NULL)
     {
	q = malloc(sizeof(interupt_queue));
	q->next = NULL;
	q->count = count;
	q->type = type;
	next_interupt = q->count;
	//print_queue();
	return;
     }
   
   if(before_event(count, q->count, q->type) && !special)
     {
	q = malloc(sizeof(interupt_queue));
	q->next = aux;
	q->count = count;
	q->type = type;
	next_interupt = q->count;
	//print_queue();
	return;
     }
   
   while (aux->next != NULL && (!before_event(count, aux->next->count, aux->next->type)
				|| special))
     aux = aux->next;
   
   if (aux->next == NULL)
     {
	aux->next = malloc(sizeof(interupt_queue));
	aux = aux->next;
	aux->next = NULL;
	aux->count = count;
	aux->type = type;
     }
   else
     {
	interupt_queue *aux2;
	if (type != SPECIAL_INT)
	  while(aux->next != NULL && aux->next->count == count)
	    aux = aux->next;
	aux2 = aux->next;
	aux->next = malloc(sizeof(interupt_queue));
	aux = aux->next;
	aux->next = aux2;
	aux->count = count;
	aux->type = type;
     }
   /*if (q->count > Count || (Count - q->count) < 0x80000000)
     next_interupt = q->count;
   else
     next_interupt = 0;*/
   //print_queue();
}

void add_interupt_event_count(int type, unsigned long count)
{
   add_interupt_event(type, (count - Count)/*/2*/);
}

void remove_interupt_event()
{
   interupt_queue *aux = q->next;
   if(q->type == SPECIAL_INT) SPECIAL_done = 1;
   free(q);
   q = aux;
   if (q != NULL && (q->count > Count || (Count - q->count) < 0x80000000))
     next_interupt = q->count;
   else
     next_interupt = 0;
}

unsigned long get_event(int type)
{
   interupt_queue *aux = q;
   if (q == NULL) return 0;
   if (q->type == type)
     return q->count;
   while (aux->next != NULL && aux->next->type != type)
     aux = aux->next;
   if (aux->next != NULL)
     return aux->next->count;
   return 0;
}

void remove_event(int type)
{
   interupt_queue *aux = q;
   if (q == NULL) return;
   if (q->type == type)
     {
	aux = aux->next;
	free(q);
	q = aux;
	return;
     }
   while (aux->next != NULL && aux->next->type != type)
     aux = aux->next;
   if (aux->next != NULL) // it's a type int
     {
	interupt_queue *aux2 = aux->next->next;
	free(aux->next);
	aux->next = aux2;
     }
}

void translate_event_queue(unsigned long base)
{
   interupt_queue *aux;
   remove_event(COMPARE_INT);
   remove_event(SPECIAL_INT);
   aux=q;
   while (aux != NULL)
     {
	aux->count = (aux->count - Count)+base;
	aux = aux->next;
     }
   add_interupt_event_count(COMPARE_INT, Compare);
   add_interupt_event_count(SPECIAL_INT, 0);
}

int save_eventqueue_infos(char *buf)
{
   int len = 0;
   interupt_queue *aux = q;
   if (q == NULL)
     {
	*((unsigned long*)&buf[0]) = 0xFFFFFFFF;
	return 4;
     }
   while (aux != NULL)
     {
	memcpy(buf+len  , &aux->type , 4);
	memcpy(buf+len+4, &aux->count, 4);
	len += 8;
	aux = aux->next;
     }
   *((unsigned long*)&buf[len]) = 0xFFFFFFFF;
   return len+4;
}

void load_eventqueue_infos(char *buf)
{
   int len = 0;
   clear_queue();
   while (*((unsigned long*)&buf[len]) != 0xFFFFFFFF)
     {
	int type = *((unsigned long*)&buf[len]);
	unsigned long count = *((unsigned long*)&buf[len+4]);
	add_interupt_event_count(type, count);
	len += 8;
     }
}

void init_interupt()
{
   SPECIAL_done = 1;
   next_vi = next_interupt = 5000;
   vi_register.vi_delay = next_vi;
   vi_field = 0;
   clear_queue();
   add_interupt_event_count(VI_INT, next_vi);
   add_interupt_event_count(SPECIAL_INT, 0);
}

void check_interupt()
{
   if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
     Cause = (Cause | 0x400) & 0xFFFFFF83;
   else
     Cause &= ~0x400;
   if ((Status & 7) != 1) return;
   if (Status & Cause & 0xFF00)
     {
	if(q == NULL)
	  {
	     q = malloc(sizeof(interupt_queue));
	     q->next = NULL;
	     q->count = Count;
	     q->type = CHECK_INT;
	  }
	else
	  {
	     interupt_queue* aux = malloc(sizeof(interupt_queue));
	     aux->next = q;
	     aux->count = Count;
	     aux->type = CHECK_INT;
	     q = aux;
	  }
	next_interupt = Count;
     }
}

void gen_interupt()
{
   //if (!skip_jump)
     //printf("interrupt:%x (%x)\n", q->type, Count);
   if (stop == 1) dyna_stop();
   
   if (savestates_job & LOADSTATE) 
     {
	savestates_load();
	savestates_job &= ~LOADSTATE;
	return;
     }
   
   if (skip_jump /*&& !dynacore*/)
     {
	if (q->count > Count || (Count - q->count) < 0x80000000)
	  next_interupt = q->count;
	else
	  next_interupt = 0;
	if(dynacore || interpcore)
	  {
	     /*if ((Cause & (2 << 2)) && (Cause & 0x80000000))
	       interp_addr = skip_jump+4;
	     else*/
	       interp_addr = skip_jump;
	     last_addr = interp_addr;
	  }
	else
	  {
	     /*if ((Cause & (2 << 2)) && (Cause & 0x80000000))
	       jump_to(skip_jump+4);
	     else*/
	     unsigned long dest = skip_jump;
	     skip_jump=0;
	     jump_to(dest);
	     last_addr = PC->addr;
	  }
	skip_jump=0;
	return;
     } 
   
   switch(q->type)
     {
      case SPECIAL_INT:
	if (Count > 0x10000000) return;
	remove_interupt_event();
	add_interupt_event_count(SPECIAL_INT, 0);
	return;
	break;
	
      case VI_INT:
	updateScreen();
#ifdef PROFILE
	refresh_stat();
#endif
	new_vi();
	if (vi_register.vi_v_sync == 0) vi_register.vi_delay = 500000;
	else vi_register.vi_delay = ((vi_register.vi_v_sync + 1)*1500);
	next_vi += vi_register.vi_delay;
	if (vi_register.vi_status&0x40) vi_field=1-vi_field;
	else vi_field=0;
	
	remove_interupt_event();
	add_interupt_event_count(VI_INT, next_vi);
	
	MI_register.mi_intr_reg |= 0x08;
	if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	  Cause = (Cause | 0x400) & 0xFFFFFF83;
	else
	  return;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      case COMPARE_INT:
	remove_interupt_event();
	Count+=2;
	add_interupt_event_count(COMPARE_INT, Compare);
	Count-=2;
	
	Cause = (Cause | 0x8000) & 0xFFFFFF83;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      case CHECK_INT:
	remove_interupt_event();
	break;
	
      case SI_INT:
#ifndef __WIN32__
	//SDL_PumpEvents();
#endif
	PIF_RAMb[0x3F] = 0x0;
	remove_interupt_event();
	MI_register.mi_intr_reg |= 0x02;
	si_register.si_status |= 0x1000;
	if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	  Cause = (Cause | 0x400) & 0xFFFFFF83;
	else
	  return;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      case PI_INT:
	remove_interupt_event();
	MI_register.mi_intr_reg |= 0x10;
	pi_register.read_pi_status_reg &= ~3;
	if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	  Cause = (Cause | 0x400) & 0xFFFFFF83;
	else
	  return;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      case AI_INT:
	if (ai_register.ai_status & 0x80000000) // full
	  {
	     unsigned long ai_event = get_event(AI_INT);
	     remove_interupt_event();
	     ai_register.ai_status &= ~0x80000000;
	     ai_register.current_delay = ai_register.next_delay;
	     ai_register.current_len = ai_register.next_len;
	     //add_interupt_event(AI_INT, ai_register.next_delay/**2*/);
	     add_interupt_event_count(AI_INT, ai_event+ai_register.next_delay);
	     
	     MI_register.mi_intr_reg |= 0x04;
	     if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	       Cause = (Cause | 0x400) & 0xFFFFFF83;
	     else
	       return;
	     if ((Status & 7) != 1) return;
	     if (!(Status & Cause & 0xFF00)) return;
	  }
	else
	  {
	     remove_interupt_event();
	     ai_register.ai_status &= ~0x40000000;

	     //-------
	     MI_register.mi_intr_reg |= 0x04;
	      if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	      Cause = (Cause | 0x400) & 0xFFFFFF83;
	      else
	      return;
	      if ((Status & 7) != 1) return;
	      if (!(Status & Cause & 0xFF00)) return;
	     //return;
	  }
	break;
	
      case SP_INT:
	remove_interupt_event();
	sp_register.sp_status_reg |= 0x303;
	//sp_register.signal1 = 1;
	sp_register.signal2 = 1;
	sp_register.broke = 1;
	sp_register.halt = 1;
	
	if (!sp_register.intr_break) return;
	MI_register.mi_intr_reg |= 0x01;
	if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	  Cause = (Cause | 0x400) & 0xFFFFFF83;
	else
	  return;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      case DP_INT:
	remove_interupt_event();
	dpc_register.dpc_status &= ~2;
	dpc_register.dpc_status |= 0x81;
	MI_register.mi_intr_reg |= 0x20;
	if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg)
	  Cause = (Cause | 0x400) & 0xFFFFFF83;
	else
	  return;
	if ((Status & 7) != 1) return;
	if (!(Status & Cause & 0xFF00)) return;
	break;
	
      default:
	remove_interupt_event();
	break;
     }
   exception_general();
   
   if (savestates_job & SAVESTATE) 
     {
	savestates_save();
	savestates_job &= ~SAVESTATE;
     }
}
