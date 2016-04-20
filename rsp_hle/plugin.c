/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - plugin.c                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#include <stdarg.h>
#include <stdio.h>

#include "wintypes.h"
#include "common.h"
#include "hle.h"
#include "hle_internal.h"

#include "RSPPlugin.h"
#include "Rsp_#1.1.h"
#include "Audio_#1.1.h"

/* local variables */
static struct hle_t g_hle;
static void (*l_CheckInterrupts)(void) = NULL;
static void (*l_ProcessDlistList)(void) = NULL;
static void (*l_ProcessAlistList)(void) = NULL;
static void (*l_ProcessRdpList)(void) = NULL;
static void (*l_ShowCFB)(void) = NULL;

/* Global functions needed by HLE core */
void HleVerboseMessage(void* UNUSED(user_defined), const char *message, ...)
{
    /* discard verbose message */
}

void HleErrorMessage(void* UNUSED(user_defined), const char *message, ...)
{
    /* discard error message */
}

void HleWarnMessage(void* UNUSED(user_defined), const char *message, ...)
{
    /* discard warning message */
}

void HleCheckInterrupts(void* UNUSED(user_defined))
{
    if (l_CheckInterrupts == NULL)
        return;

    (*l_CheckInterrupts)();
}

void HleProcessDlistList(void* UNUSED(user_defined))
{
    if (l_ProcessDlistList == NULL)
        return;

    (*l_ProcessDlistList)();
}

void HleProcessAlistList(void* UNUSED(user_defined))
{
    if (l_ProcessAlistList == NULL)
        return;

    (*l_ProcessAlistList)();
}

void HleProcessRdpList(void* UNUSED(user_defined))
{
    if (l_ProcessRdpList == NULL)
        return;

    (*l_ProcessRdpList)();
}

void HleShowCFB(void* UNUSED(user_defined))
{
    if (l_ShowCFB == NULL)
        return;

    (*l_ShowCFB)();
}


/* DLL-exported functions */
EXPORT void CALL CloseDLL(void)
{
}

EXPORT unsigned int CALL DoRspCycles(unsigned int Cycles)
{
    hle_execute(&g_hle);
    return Cycles;
}

EXPORT void CALL InitiateRSP(RSP_INFO Rsp_Info, unsigned int* UNUSED(CycleCount))
{
    hle_init(&g_hle,
             Rsp_Info.RDRAM,
             Rsp_Info.DMEM,
             Rsp_Info.IMEM,
             Rsp_Info.MI_INTR_REG,
             Rsp_Info.SP_MEM_ADDR_REG,
             Rsp_Info.SP_DRAM_ADDR_REG,
             Rsp_Info.SP_RD_LEN_REG,
             Rsp_Info.SP_WR_LEN_REG,
             Rsp_Info.SP_STATUS_REG,
             Rsp_Info.SP_DMA_FULL_REG,
             Rsp_Info.SP_DMA_BUSY_REG,
             Rsp_Info.SP_PC_REG,
             Rsp_Info.SP_SEMAPHORE_REG,
             Rsp_Info.DPC_START_REG,
             Rsp_Info.DPC_END_REG,
             Rsp_Info.DPC_CURRENT_REG,
             Rsp_Info.DPC_STATUS_REG,
             Rsp_Info.DPC_CLOCK_REG,
             Rsp_Info.DPC_BUFBUSY_REG,
             Rsp_Info.DPC_PIPEBUSY_REG,
             Rsp_Info.DPC_TMEM_REG,
             NULL);

    l_CheckInterrupts = Rsp_Info.CheckInterrupts;
    l_ProcessDlistList = Rsp_Info.ProcessDlistList;
    l_ProcessAlistList = Rsp_Info.ProcessAlistList;
    l_ProcessRdpList = Rsp_Info.ProcessRdpList;
    l_ShowCFB = Rsp_Info.ShowCFB;
}

EXPORT void CALL RomClosed(void)
{
    /* do nothing */
}
