/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - main.c                                          *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <string.h>

#include "wintypes.h"
#include "hle.h"
#include "alist.h"
#include "cicx105.h"
#include "jpeg.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

/* some rsp status flags */
#define RSP_STATUS_HALT             0x1
#define RSP_STATUS_BROKE            0x2
#define RSP_STATUS_INTR_ON_BREAK    0x40
#define RSP_STATUS_TASKDONE         0x200

/* some rdp status flags */
#define DP_STATUS_FREEZE            0x2

/* some mips interface interrupt flags */
#define MI_INTR_SP                  0x1


/* helper functions prototypes */
static unsigned int sum_bytes(const unsigned char *bytes, unsigned int size);

/* global variables */
RSP_INFO rsp;

/* local variables */
static const int FORWARD_AUDIO = 0, FORWARD_GFX = 1;

/* local functions */


/**
 * Try to figure if the RSP was launched using osSpTask* functions
 * and not run directly (in which case DMEM[0xfc0-0xfff] is meaningless).
 *
 * Previously, the ucode_size field was used to determine this,
 * but it is not robust enough (hi Pokemon Stadium !) because games could write anything
 * in this field : most ucode_boot discard the value and just use 0xf7f anyway.
 *
 * Using ucode_boot_size should be more robust in this regard.
 **/
static int is_task()
{
    return (get_task()->ucode_boot_size <= 0x1000);
}

static void rsp_break(unsigned int setbits)
{
    *rsp.SP_STATUS_REG |= setbits | RSP_STATUS_BROKE | RSP_STATUS_HALT;

    if ((*rsp.SP_STATUS_REG & RSP_STATUS_INTR_ON_BREAK))
    {
        *rsp.MI_INTR_REG |= MI_INTR_SP;
        rsp.CheckInterrupts();
    }
}

static void forward_gfx_task()
{
    if (rsp.ProcessDlistList != NULL)
    {
        rsp.ProcessDlistList();
        *rsp.DPC_STATUS_REG &= ~DP_STATUS_FREEZE;
    }
}

static void forward_audio_task()
{
    if (rsp.ProcessAlistList != NULL)
    {
        rsp.ProcessAlistList();
    }
}

static void show_cfb()
{
    if (rsp.ShowCFB != NULL)
    {
        rsp.ShowCFB();
    }
}

static int try_fast_audio_dispatching()
{
    /* identify audio ucode by using the content of ucode_data */
    const OSTask_t * const task = get_task();
    const unsigned char * const udata_ptr = rsp.RDRAM + task->ucode_data;

    if (*(unsigned int*)(udata_ptr + 0) == 0x00000001)
    {
        if (*(unsigned int*)(udata_ptr + 0x30) == 0xf0000f00)
        {
            /**
            * Many games including:
            * Super Mario 64, Diddy Kong Racing, BlastCorp, GoldenEye, ... (most common)
            **/
            alist_process_ABI1(); return 1;
        }
        else
        {
            /* use first ACMD offset to discriminate between ABIs */
            u16 v = *(u16*)(udata_ptr + (0x10 ^ S16));

            switch (v)
            {
            /* Mario Kart / Wave Race */
            case 0x1118: alist_process_ABI2a(); return 1;

            /* LylatWars */
            case 0x1104: alist_process_ABI2b(); return 1;

            /* FZeroX */
            case 0x1cd0: alist_process_ABI2c(); return 1;

            case 0x1f08: /* Yoshi Story */
            case 0x1f38: /* 1080° Snowboarding */
                alist_process_ABI2d(); return 1;

            /* Zelda Ocarina of Time */
            case 0x1f68: alist_process_ABI2e(); return 1;

            /* Zelda Majoras Mask / Pokemon Stadium 2 */
            case 0x1f80: alist_process_ABI2f(); return 1;

            /* Animal Crossing */
            case 0x1eac: alist_process_ABI2g(); return 1;
            }
        }
    }
    else
    {
        if (*(unsigned int*)(udata_ptr + 0x10) == 0x00000001)
        {
            /**
             * Musyx ucode found in following games:
             * RogueSquadron, ResidentEvil2, SnowCrossPolaris, TheWorldIsNotEnough,
             * RugratsInParis, NBAShowTime, HydroThunder, Tarzan,
             * GauntletLegend, Rush2049, IndianaJones, BattleForNaboo
             * TODO: implement ucode
             **/
            return 1;
        }
        else
        {
            /**
             * Many games including:
             * Pokemon Stadium, Banjo Kazooie, Donkey Kong, Banjo Tooie, Jet Force Gemini,
             * Mickey SpeedWay USA, Perfect Dark, Conker Bad Fur Day ...
             **/
            alist_process_ABI3(); return 1;
        }
    }

    return 0;
}

static int try_fast_task_dispatching()
{
    /* identify task ucode by its type */
    const OSTask_t * const task = get_task();

    switch (task->type)
    {
        case 1: if (FORWARD_GFX) { forward_gfx_task(); return 1; } break;

        case 2:
            if (FORWARD_AUDIO) { forward_audio_task(); return 1; }
            else if (try_fast_audio_dispatching()) { return 1; }
            break;

        case 7: show_cfb(); return 1;
    }

    return 0;
}

static void normal_task_dispatching()
{
    const OSTask_t * const task = get_task();
    const unsigned int sum =
        sum_bytes(rsp.RDRAM + task->ucode, min(task->ucode_size, 0xf80) >> 1);

    switch (sum)
    {
        /* StoreVe12: found in Zelda Ocarina of Time [misleading task->type == 4] */
        case 0x278: /* Nothing to emulate */ return;

        /* GFX: Twintris [misleading task->type == 0] */                                         
        case 0x212ee:
            if (FORWARD_GFX) { forward_gfx_task(); return; }
            break;

        /* JPEG: found in Pokemon Stadium J */ 
        case 0x2c85a: jpeg_decode_PS0(); return;

        /* JPEG: found in Zelda Ocarina of Time, Pokemon Stadium 1, Pokemon Stadium 2 */
        case 0x2caa6: jpeg_decode_PS(); return;

        /* JPEG: found in Ogre Battle, Bottom of the 9th */
        case 0x130de: jpeg_decode_OB(); return;
    }
}

static void non_task_dispatching()
{
    const unsigned int sum = sum_bytes(rsp.IMEM, 0x1000 >> 1);

    switch(sum)
    {
        /* CIC x105 ucode (used during boot of CIC x105 games) */
        case 0x9e2: /* CIC 6105 */
        case 0x9f2: /* CIC 7105 */
            cicx105_ucode(); return;
    }
}


/* DLL-exported functions */
EXPORT void CALL CloseDLL(void)
{
}

EXPORT unsigned int CALL DoRspCycles(unsigned int Cycles)
{
    if (is_task())
    {
        if (!try_fast_task_dispatching()) { normal_task_dispatching(); }
        rsp_break(RSP_STATUS_TASKDONE);
    }
    else
    {
        non_task_dispatching();
        rsp_break(0);
    }

    return Cycles;
}

EXPORT void CALL InitiateRSP(RSP_INFO Rsp_Info, unsigned int *CycleCount)
{
    rsp = Rsp_Info;
}

EXPORT void CALL RomClosed(void)
{
    memset(rsp.DMEM, 0, 0x1000);
    memset(rsp.IMEM, 0, 0x1000);
}


/* local helper functions */
static unsigned int sum_bytes(const unsigned char *bytes, unsigned int size)
{
    unsigned int sum = 0;
    const unsigned char * const bytes_end = bytes + size;

    while (bytes != bytes_end)
        sum += *bytes++;

    return sum;
}

