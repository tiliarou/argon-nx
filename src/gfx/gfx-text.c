/*
 * Copyright (C) 2018 Guillem96
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
#include "libs/mcufont/mcufont.h"
#include "gfx/gfx.h"
#include "gfx/dejavu-font.h"
#include "mem/heap.h"
#include <string.h>

typedef struct {
    u8 *buffer;
    u16 width;
    u16 height;
    u16 y;
    u32 margin;
    u32 anchor;
    enum mf_align_t alignment;
    bool justify;
    bool light;
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
        
        if (s->light)
        {
            value += alpha;
            if (value > 255) 
                value = 0;
        }
        else 
        {
            value -= alpha;
            if (value < 0) 
                value = 0;
        }
        
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
    
    if (s->justify)
    {
        mf_render_justified(s->font, s->anchor, s->y,
                            s->width - s->margin * 2,
                            line, count, character_callback, state);
    }
    else

    {
        mf_render_aligned(s->font, s->anchor, s->y,
                          s->alignment, line, count,
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

void gfx_render_text(gfx_con_t *con, char* text, u32 x, u32 y, u32 width, bool light)
{
    u32 height;
    const struct mf_font_s *font;
    state_t state = {};
    struct mf_scaledfont_s scaledfont;

    font = (struct mf_font_s*)&dejavu;
    
    /* Scale the font if necessary */
    if (con->scale > 1)
    {
        mf_scale_font(&scaledfont, font, con->scale, con->scale);
        font = &scaledfont.font;
    }

    /* Count the number of lines that we need. */
    height = 0;
    mf_wordwrap(font, width - 2 * state.margin, text, count_lines, &height);
    height *= font->height;
    height += 4;

    /* TODO: Custom state */
    /* Init state and allocate and clear the image buffer */
    state.alignment = MF_ALIGN_LEFT;
    state.margin = 5;
    state.anchor = state.margin;
    state.width = width;
    state.height = height;
    state.buffer = malloc(width * height);
    state.y = 2;
    state.font = font;
    state.justify = false;
    state.light = light;
    
    /* Clear the buffer with 0s */
    memset(state.buffer, 0, width * height);
    
    /* Render the text */
    mf_wordwrap(font, width - 2 * state.margin, text, line_callback, &state);
    gfx_set_rect_grey(&g_gfx_ctxt, state.buffer, state.width, state.height, 10, 10);
    free(state.buffer);
}


