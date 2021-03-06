/*
 * Copyright (c) 2018 Guillem96
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/custom-gui.h"
#include "gfx/gfx.h"
#include "mem/heap.h"
#include "utils/fs_utils.h"

#include <string.h>

bool render_custom_background()
{
    u8* custom_bg = (u8*)sd_file_read(CUSTOM_BG_PATH);
    if (custom_bg == NULL)
        return false;
    
    gfx_render_bmp_arg_bitmap(&g_gfx_ctxt, custom_bg, 0, 0, 1280, 720);
    return true;
}

bool render_custom_title()
{
    u8* title_bmp = (u8*)sd_file_read(CUSTOM_TITLE_PATH);
    if (title_bmp == NULL)
        return false;

    u32 bmp_width = (title_bmp[0x12] | (title_bmp[0x13] << 8) | (title_bmp[0x14] << 16) | (title_bmp[0x15] << 24));
    u32 bmp_height = (title_bmp[0x16] | (title_bmp[0x17] << 8) | (title_bmp[0x18] << 16) | (title_bmp[0x19] << 24));
    gfx_render_bmp_arg_bitmap(&g_gfx_ctxt, (u8*)title_bmp, 420, 10, bmp_width, bmp_height);
    return true;
}

int screenshot(void* params)
{
    //width, height, and bitcount are the key factors:
    s32 width = 720;
    s32 height = 1280;
    u16 bitcount = 32;//<- 24-bit bitmap

    //take padding in to account
    int width_in_bytes = ((width * bitcount + 31) / 32) * 4;

    //total image size in bytes, not including header
    u32 imagesize = width_in_bytes * height;

    //this value is always 40, it's the sizeof(BITMAPINFOHEADER)
    const u32 biSize = 40;

    //bitmap bits start after headerfile, 
    //this is sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
    const u32 bfOffBits = 54; 

    //total file size:
    u32 filesize = 54 + imagesize;

    //number of planes is usually 1
    const u16 biPlanes = 1;

    //create header:
    //copy to buffer instead of BITMAPFILEHEADER and BITMAPINFOHEADER
    //to avoid problems with structure packing
    unsigned char header[54] = { 0 };
    memcpy(header, "BM", 2);
    memcpy(header + 2 , &filesize, 4);
    memcpy(header + 10, &bfOffBits, 4);
    memcpy(header + 14, &biSize, 4);
    memcpy(header + 18, &width, 4);
    memcpy(header + 22, &height, 4);
    memcpy(header + 26, &biPlanes, 2);
    memcpy(header + 28, &bitcount, 2);
    memcpy(header + 34, &imagesize, 4);

    u8* buff = (u8*)malloc(imagesize + 54);
    memcpy(buff, header, 54);
    memcpy(buff + 54, g_gfx_ctxt.fb, imagesize);
    sd_save_to_file(buff, imagesize + 54, "argon/screenshot.bmp");
    free(buff);

    g_gfx_con.scale = 2;
    gfx_con_setpos(&g_gfx_con, 0, 665);
    gfx_printf(&g_gfx_con, " Screenshot saved!\n Found it at argon/screenshot.bmp");
    return 0;
}