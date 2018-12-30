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

#include <string.h>

#include "gfx/di.h"
#include "gfx/gfx.h"

#include "mem/heap.h"

#include "soc/hw_init.h"
#include "soc/t210.h"

#include "core/launcher.h"

#include "utils/util.h"
#include "utils/fs_utils.h"
#include "utils/btn.h"

#include "menu/gui/gui_argon_menu.h"

#include "minerva/minerva.h"

#include "libs/mcufont/mcufont.h"
#include "dejavu-font.h"

typedef struct {
    const char *fontname;
    const char *filename;
    const char *text;
    bool justify;
    enum mf_align_t alignment;
    int width;
    int margin;
    int anchor;
    int scale;
} options_t;

static const char default_text[] = 
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog. "
    "The quick brown fox jumps over the lazy dog.\n"
    "0123456789 !?()[]{}/\\+-*";

/********************************************
 * Callbacks to specify rendering behaviour *
 ********************************************/

typedef struct {
    options_t *options;
    u8 *buffer;
    u16 width;
    u16 height;
    u16 y;
    const struct mf_font_s *font;
} state_t;

/* Callback to write to a memory buffer. */
static void pixel_callback(s16 x, s16 y, u8 count, u8 alpha,
                           void *state)
{
    state_t *s = (state_t*)state;
    u32 pos;
    u16 value;
    
    if (y < 0 || y >= s->height) return;
    if (x < 0 || x + count >= s->width) return;
    
    while (count--)
    {
        pos = (u32)s->width * y + x;
        value = s->buffer[pos];
        value += alpha;
        if (value > 255) 
            value = 0;
        s->buffer[pos] = value;
        x++;
    }

}

/* Callback to render characters. */
static u8 character_callback(s16 x, s16 y, mf_char character,
                                  void *state)
{
    state_t *s = (state_t*)state;
    return mf_render_character(s->font, x, y, character, pixel_callback, state);
}

/* Callback to render lines. */
static bool line_callback(const char *line, u16 count, void *state)
{
    state_t *s = (state_t*)state;
    
    if (s->options->justify)
    {
        mf_render_justified(s->font, s->options->anchor, s->y,
                            s->width - s->options->margin * 2,
                            line, count, character_callback, state);
    }
    else

    {
        mf_render_aligned(s->font, s->options->anchor, s->y,
                          s->options->alignment, line, count,
                          character_callback, state);
    }
    s->y += s->font->line_height;
    return true;
}

/* Callback to just count the lines.
 * Used to decide the image height */
bool count_lines(const char *line, u16 count, void *state)
{
    int *linecount = (int*)state;
    (*linecount)++;
    return true;
}


extern void pivot_stack(u32 stack_top);

static inline void setup_gfx()
{
    u32 *fb = display_init_framebuffer();
    gfx_init_ctxt(&g_gfx_ctxt, fb, 1280, 720, 720);
    gfx_con_init(&g_gfx_con, &g_gfx_ctxt);
    gfx_con_setcol(&g_gfx_con, 0xFFCCCCCC, 1, BLACK);
}

#include <stdlib.h>


void render_font()
{
    int height;
    const struct mf_font_s *font;
    options_t options;
    state_t state = {};
    struct mf_scaledfont_s scaledfont;

    memset(&options, 0, sizeof(options_t));
    
    options.text = "Hello from ArgonNX! Using DejaVu font :)";
    options.width = 1200;
    options.margin = 5;
    options.scale = 3;
    options.alignment = MF_ALIGN_LEFT;
    options.anchor = options.margin;
    
    font = (struct mf_font_s*)&dejavu;
    
    if (options.scale > 1)
    {
        mf_scale_font(&scaledfont, font, options.scale, options.scale);
        font = &scaledfont.font;
    }
    /* Count the number of lines that we need. */
    height = 0;
    mf_wordwrap(font, options.width - 2 * options.margin,
                options.text, count_lines, &height);
    height *= font->height;
    height += 4;
    gfx_printf(&g_gfx_con, "Font height: %d\n", height);

    /* Allocate and clear the image buffer */
    state.options = &options;
    state.width = options.width;
    state.height = height;
    state.buffer = malloc(options.width * height);
    state.y = 2;
    state.font = font;
    
    gfx_clear_color(&g_gfx_ctxt, 0);
    
    /* Initialize image to white */
    memset(state.buffer, 0, options.width * height);
    
    /* Render the text */
    mf_wordwrap(font, options.width - 2 * options.margin,
                options.text, line_callback, &state);
    
    gfx_printf(&g_gfx_con, "Width: %d, Height: %d\n", state.width, state.height);
    gfx_set_rect_grey(&g_gfx_ctxt, state.buffer, state.width, state.height, 10, 10);
    free(state.buffer);
}


void ipl_main()
{
    /* Configure Switch Hardware (thanks to hekate project) */
    config_hw();

    /* Init the stack and the heap */
    pivot_stack(0x90010000);
    heap_init(0x90020000);

    /* Init display and gfx module */
    display_init();
    setup_gfx();
    display_backlight_pwm_init();
    display_backlight_brightness(100, 1000);
    
    /* Train DRAM */
    minerva();
    
    /* Double the font size */
    g_gfx_con.scale = 2;

    /* Mount Sd card and launch payload */
    if (sd_mount())
    {
        render_font();
        // bool cancel_auto_chainloading = btn_read() & BTN_VOL_DOWN;
        // bool load_menu = cancel_auto_chainloading || launch_payload("argon/payload.bin");
        
        // if (load_menu)
        //     gui_init_argon_menu();



    } else {
        gfx_printf(&g_gfx_con, "No sd card found...\n");
    }

    /* If payload launch fails wait for user input to reboot the switch */
    gfx_printf(&g_gfx_con, "Press power button to reboot into RCM...\n\n");
    wait_for_button_and_reboot();
}