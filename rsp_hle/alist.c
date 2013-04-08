/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist.c                                         *
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

#include "wintypes.h"
#include "hle.h"
#include "alist_internal.h"
#include "../main/wii64config.h"

extern const acmd_callback_t ABI1[0x10];
extern const acmd_callback_t ABI2a[0x20];
extern const acmd_callback_t ABI2b[0x20];
extern const acmd_callback_t ABI2c[0x20];
extern const acmd_callback_t ABI2d[0x18];
extern const acmd_callback_t ABI2e[0x18];
extern const acmd_callback_t ABI2f[0x18];
extern const acmd_callback_t ABI2g[0x18];
extern const acmd_callback_t ABI3[0x10];

/* local functions */
static void alist_process(const acmd_callback_t abi[], unsigned int abi_size)
{
    u32 inst1, inst2;
    unsigned int acmd;
    const OSTask_t * const task = get_task();

    const unsigned int *alist = (unsigned int*)(rsp.RDRAM + task->data_ptr);
    const unsigned int * const alist_end = alist + (task->data_size >> 2);

    if (!audioEnabled)
        return;

    while (alist != alist_end)
    {
        inst1 = *(alist++);
        inst2 = *(alist++);

        acmd = inst1 >> 24;

        if (acmd < abi_size)
        {
            (*abi[acmd])(inst1, inst2);
        }
    }
}

/* global functions */
void alist_process_ABI1()
{
    alist_process(ABI1, 0x10);
}

void alist_process_ABI2a()
{
    alist_process(ABI2a, 0x20);
}

void alist_process_ABI2b()
{
    alist_process(ABI2b, 0x20);
}

void alist_process_ABI2c()
{
    alist_process(ABI2c, 0x20);
}

void alist_process_ABI2d()
{
    alist_process(ABI2d, 0x18);
}

void alist_process_ABI2e()
{
    alist_process(ABI2e, 0x18);
}

void alist_process_ABI2f()
{
    alist_process(ABI2f, 0x18);
}

void alist_process_ABI2g()
{
    alist_process(ABI2g, 0x18);
}

void alist_process_ABI3()
{
    alist_process(ABI3, 0x10);
}

