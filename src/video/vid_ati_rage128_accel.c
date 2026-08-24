/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          ATI Rage 128 synchronous 2D engine.
 *
 * Authors: Bryce Lanham
 *
 *          Copyright 2026 Bryce Lanham.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <86box/86box.h>

#include "vid_ati_rage128.h"

typedef struct rage128_surface_t {
    uint32_t offset;
    uint32_t stride;
    int      x;
    int      y;
} rage128_surface_t;

typedef struct rage128_rect_t {
    int left;
    int top;
    int right;
    int bottom;
} rage128_rect_t;

static uint32_t
rage128_rop3_eval(uint8_t rop, uint32_t pattern, uint32_t source, uint32_t dest)
{
    uint32_t out = 0;

    for (unsigned int i = 0; i < 8; i++) {
        uint32_t term = (i & 4) ? pattern : ~pattern;

        term &= (i & 2) ? source : ~source;
        term &= (i & 1) ? dest : ~dest;
        out |= term & -(uint32_t) ((rop >> i) & 1);
    }

    return out;
}

static bool
rage128_rop3_uses_pattern(uint8_t rop)
{
    return ((rop ^ (rop >> 4)) & 0x0f) != 0;
}

static bool
rage128_rop3_uses_source(uint8_t rop)
{
    return ((rop ^ (rop >> 2)) & 0x33) != 0;
}


static bool
rage128_brush_supported(uint32_t brush)
{
    switch (brush) {
        case R128_BRUSH_8X8_MONO_FG_BG:
        case R128_BRUSH_8X8_MONO_FG_LA:
        case R128_BRUSH_32X1_MONO_FG_BG:
        case R128_BRUSH_32X1_MONO_FG_LA:
        case R128_BRUSH_SOLIDCOLOR:
            return true;
        default:
            return false;
    }
}

/*
 * Resolve the pattern operand for one destination pixel.  A false return
 * means that a foreground-only monochrome brush selected its transparent
 * background bit.  This follows the register programming used by the
 * historical Rage 128 XAA driver.
 */
static bool
rage128_brush_pixel(const rage128_t *dev, uint32_t brush, int x, int y,
                    uint32_t line_index, bool line, uint32_t *color)
{
    unsigned int bit;
    bool         foreground;
    bool         lsb_first = !!(R128_REG(dev, R128_DP_DATATYPE) &
                                R128_DP_BYTE_PIX_ORDER);

    switch (brush) {
        case R128_BRUSH_SOLIDCOLOR:
            *color = R128_REG(dev, R128_DP_BRUSH_FRGD_CLR);
            return true;

        case R128_BRUSH_8X8_MONO_FG_BG:
        case R128_BRUSH_8X8_MONO_FG_LA: {
            uint32_t     origin = R128_REG(dev, R128_BRUSH_Y_X);
            unsigned int px = (unsigned int)(x - (int)(origin & 0xffU)) & 7U;
            unsigned int py = (unsigned int)(y - (int)((origin >> 8) & 0xffU)) & 7U;
            uint32_t     word = R128_REG(dev, R128_BRUSH_DATA0 +
                                             ((py >> 2) * 4U));
            uint8_t      row = word >> ((py & 3U) * 8U);

            bit        = lsb_first ? px : 7U - px;
            foreground = !!(row & (UINT8_C(1) << bit));
            break;
        }

        case R128_BRUSH_32X1_MONO_FG_BG:
        case R128_BRUSH_32X1_MONO_FG_LA: {
            unsigned int phase = R128_REG(dev, R128_BRUSH_Y_X) & 31U;
            unsigned int px = line ? line_index + phase :
                                     (unsigned int)(x - (int)phase);

            bit = px & 31U;
            if (!lsb_first)
                bit = 31U - bit;
            foreground = !!(R128_REG(dev, R128_BRUSH_DATA0) &
                             (UINT32_C(1) << bit));
            break;
        }

        default:
            return false;
    }

    if (!foreground &&
        (brush == R128_BRUSH_8X8_MONO_FG_LA ||
         brush == R128_BRUSH_32X1_MONO_FG_LA))
        return false;

    *color = foreground ? R128_REG(dev, R128_DP_BRUSH_FRGD_CLR) :
                          R128_REG(dev, R128_DP_BRUSH_BKGD_CLR);
    return true;
}

static bool
rage128_decode_format(const rage128_t *dev, unsigned int *storage_bpp,
                      unsigned int *bytes_per_pixel, uint32_t *pixel_mask)
{
    switch (R128_REG(dev, R128_DP_DATATYPE) & R128_DP_DST_DATATYPE) {
        case R128_DST_8BPP:
            *storage_bpp     = 8;
            *bytes_per_pixel = 1;
            *pixel_mask      = 0x000000ffU;
            return true;
        case R128_DST_15BPP:
            *storage_bpp     = 16;
            *bytes_per_pixel = 2;
            *pixel_mask      = 0x00007fffU;
            return true;
        case R128_DST_16BPP:
            *storage_bpp     = 16;
            *bytes_per_pixel = 2;
            *pixel_mask      = 0x0000ffffU;
            return true;
        case R128_DST_24BPP:
            *storage_bpp     = 24;
            *bytes_per_pixel = 3;
            *pixel_mask      = 0x00ffffffU;
            return true;
        case R128_DST_32BPP:
            *storage_bpp     = 32;
            *bytes_per_pixel = 4;
            *pixel_mask      = 0xffffffffU;
            return true;
        default:
            return false;
    }
}

static void
rage128_decode_scissor(const rage128_t *dev, rage128_rect_t *scissor,
                       unsigned int bytes_per_pixel)
{
    uint32_t left  = R128_REG(dev, R128_SC_LEFT) & 0x3fffU;
    uint32_t right = R128_REG(dev, R128_SC_RIGHT) & 0x3fffU;

    /*
     * Packed 24-bpp mode exposes horizontal scissor coordinates in bytes
     * while destination and source coordinates remain pixels.  The period
     * XFree86 driver therefore multiplies its clip edges by three.
     */
    if (bytes_per_pixel == 3) {
        left  = (left + 2U) / 3U;
        right = right / 3U;
    }

    scissor->left   = (int) left;
    scissor->top    = (int) (R128_REG(dev, R128_SC_TOP) & 0x3fffU);
    scissor->right  = (int) right;
    scissor->bottom = (int) (R128_REG(dev, R128_SC_BOTTOM) & 0x3fffU);
}

static bool
rage128_rect_intersect(rage128_rect_t *out, const rage128_rect_t *a,
                       const rage128_rect_t *b)
{
    out->left   = (a->left > b->left) ? a->left : b->left;
    out->top    = (a->top > b->top) ? a->top : b->top;
    out->right  = (a->right < b->right) ? a->right : b->right;
    out->bottom = (a->bottom < b->bottom) ? a->bottom : b->bottom;

    return out->left <= out->right && out->top <= out->bottom;
}

static bool
rage128_rect_in_vram(const rage128_t *dev, const rage128_surface_t *surface,
                     const rage128_rect_t *rect, unsigned int bytes_per_pixel)
{
    uint64_t first;
    uint64_t last;

    if (surface->stride == 0 || rect->left < 0 || rect->top < 0 ||
        rect->right < rect->left || rect->bottom < rect->top)
        return false;

    first = (uint64_t) surface->offset +
            (uint64_t) rect->top * surface->stride +
            (uint64_t) rect->left * bytes_per_pixel;
    last = (uint64_t) surface->offset +
           (uint64_t) rect->bottom * surface->stride +
           (uint64_t) (rect->right + 1) * bytes_per_pixel;

    return first < dev->vram_size && last <= dev->vram_size;
}

static uint32_t
rage128_pixel_read(const rage128_t *dev, uint32_t offset,
                   unsigned int bytes_per_pixel)
{
    uint32_t value = 0;

    for (unsigned int i = 0; i < bytes_per_pixel; i++)
        value |= (uint32_t) dev->svga.vram[offset + i] << (i * 8);

    return value;
}

static void
rage128_pixel_write(rage128_t *dev, uint32_t offset,
                    unsigned int bytes_per_pixel, uint32_t value)
{
    for (unsigned int i = 0; i < bytes_per_pixel; i++)
        dev->svga.vram[offset + i] = value >> (i * 8);
}

static void
rage128_dirty_row(rage128_t *dev, uint32_t first, uint32_t last)
{
    uint32_t first_page = first >> 12;
    uint32_t last_page  = last >> 12;

    for (uint32_t page = first_page; page <= last_page; page++)
        dev->svga.changedvram[page] = dev->svga.monitor->mon_changeframecount;
}

static void
rage128_warn_unsupported(uint8_t *guard, const char *message, uint32_t value)
{
    if (!*guard) {
        pclog("ATI Rage 128: %s (0x%08x)\n", message, value);
        *guard = 1;
    }
}

static bool
rage128_color_compare_supported(rage128_t *dev, bool source_available)
{
    uint32_t control  = R128_REG(dev, R128_CLR_CMP_CNTL);
    uint32_t function = control & R128_CLR_CMP_FN_MASK;

    if (function == 0)
        return true;

    if (function != R128_SRC_CMP_EQ_COLOR &&
        function != R128_SRC_CMP_NEQ_COLOR) {
        rage128_warn_unsupported(&dev->warned_color_compare,
                                 "color-compare function is not implemented",
                                 control);
        return false;
    }

    if ((control & R128_CLR_CMP_SRC_SOURCE) && !source_available) {
        rage128_warn_unsupported(&dev->warned_color_compare,
                                 "source color comparison has no source",
                                 control);
        return false;
    }

    return true;
}

static bool
rage128_color_compare_pass(const rage128_t *dev, uint32_t source,
                           uint32_t dest, uint32_t pixel_mask)
{
    uint32_t control  = R128_REG(dev, R128_CLR_CMP_CNTL);
    uint32_t function = control & R128_CLR_CMP_FN_MASK;
    uint32_t mask;
    uint32_t value;
    uint32_t reference;
    bool     equal;

    if (function == 0)
        return true;

    mask = R128_REG(dev, R128_CLR_CMP_MASK) & pixel_mask;
    if (control & R128_CLR_CMP_SRC_SOURCE) {
        value     = source;
        reference = R128_REG(dev, R128_CLR_CMP_CLR_SRC);
    } else {
        value     = dest;
        reference = R128_REG(dev, R128_CLR_CMP_CLR_DST);
    }

    equal = (value & mask) == (reference & mask);
    return function == R128_SRC_CMP_EQ_COLOR ? equal : !equal;
}

static bool
rage128_accel_prepare(rage128_t *dev, rage128_surface_t *src,
                      rage128_surface_t *dst, rage128_rect_t *src_rect,
                      rage128_rect_t *dst_rect, rage128_rect_t *visible,
                      unsigned int *bytes_per_pixel, uint32_t *pixel_mask,
                      uint8_t *rop, bool *needs_source)
{
    unsigned int storage_bpp;
    uint32_t     width  = R128_REG(dev, R128_DST_WIDTH) & 0x3fffU;
    uint32_t     height = R128_REG(dev, R128_DST_HEIGHT) & 0x3fffU;
    uint32_t     compare = R128_REG(dev, R128_CLR_CMP_CNTL);
    bool         compare_uses_source;
    bool         left_to_right;
    bool         top_to_bottom;
    rage128_rect_t scissor;

    if (!width || !height)
        return false;

    if (!rage128_decode_format(dev, &storage_bpp, bytes_per_pixel, pixel_mask))
        return false;

    if ((R128_REG(dev, R128_DP_CNTL) &
         (R128_DST_X_TILE_EN | R128_DST_Y_TILE_EN)) ||
        (R128_REG(dev, R128_DST_PITCH) & 0x00010000U) ||
        (R128_REG(dev, R128_SRC_PITCH) & 0x00010000U)) {
        rage128_warn_unsupported(&dev->warned_tiling,
                                 "tiled 2D surfaces are not implemented",
                                 R128_REG(dev, R128_DP_CNTL));
        return false;
    }

    *rop = (R128_REG(dev, R128_DP_MIX) & R128_DP_ROP3) >> 16;
    compare_uses_source = (compare & R128_CLR_CMP_FN_MASK) != 0 &&
                          !!(compare & R128_CLR_CMP_SRC_SOURCE);
    *needs_source = rage128_rop3_uses_source(*rop) || compare_uses_source;

    if (rage128_rop3_uses_pattern(*rop) &&
        !rage128_brush_supported(R128_REG(dev, R128_DP_DATATYPE) &
                                  R128_DP_BRUSH_DATATYPE)) {
        rage128_warn_unsupported(&dev->warned_brush,
                                 "2D brush datatype is not implemented",
                                 R128_REG(dev, R128_DP_DATATYPE));
        return false;
    }

    if (*needs_source &&
        (R128_REG(dev, R128_DP_MIX) & R128_DP_SRC_SOURCE) !=
            R128_DP_SRC_RECT) {
        rage128_warn_unsupported(&dev->warned_source,
                                 "2D source selector is not implemented",
                                 R128_REG(dev, R128_DP_MIX) &
                                     R128_DP_SRC_SOURCE);
        return false;
    }

    if (!rage128_color_compare_supported(dev, *needs_source))
        return false;

    left_to_right = !!(R128_REG(dev, R128_DP_CNTL) &
                       R128_DST_X_LEFT_TO_RIGHT);
    top_to_bottom = !!(R128_REG(dev, R128_DP_CNTL) &
                       R128_DST_Y_TOP_TO_BOTTOM);

    dst->offset = R128_REG(dev, R128_DST_OFFSET) & 0xfffffff0U;
    dst->stride = (R128_REG(dev, R128_DST_PITCH) & 0x3fffU) * storage_bpp;
    dst->x = left_to_right ? (int) (R128_REG(dev, R128_DST_X) & 0x3fffU) :
                             (int) (R128_REG(dev, R128_DST_X) & 0x3fffU) +
                                 1 - (int) width;
    dst->y = top_to_bottom ? (int) (R128_REG(dev, R128_DST_Y) & 0x3fffU) :
                             (int) (R128_REG(dev, R128_DST_Y) & 0x3fffU) +
                                 1 - (int) height;

    src->offset = R128_REG(dev, R128_SRC_OFFSET) & 0xfffffff0U;
    src->stride = (R128_REG(dev, R128_SRC_PITCH) & 0x3fffU) * storage_bpp;
    src->x = left_to_right ? (int) (R128_REG(dev, R128_SRC_X) & 0x3fffU) :
                             (int) (R128_REG(dev, R128_SRC_X) & 0x3fffU) +
                                 1 - (int) width;
    src->y = top_to_bottom ? (int) (R128_REG(dev, R128_SRC_Y) & 0x3fffU) :
                             (int) (R128_REG(dev, R128_SRC_Y) & 0x3fffU) +
                                 1 - (int) height;

    dst_rect->left   = dst->x;
    dst_rect->top    = dst->y;
    dst_rect->right  = dst->x + (int) width - 1;
    dst_rect->bottom = dst->y + (int) height - 1;

    rage128_decode_scissor(dev, &scissor, *bytes_per_pixel);

    if (!rage128_rect_intersect(visible, dst_rect, &scissor))
        return false;

    src_rect->left   = src->x + (visible->left - dst_rect->left);
    src_rect->top    = src->y + (visible->top - dst_rect->top);
    src_rect->right  = src_rect->left + (visible->right - visible->left);
    src_rect->bottom = src_rect->top + (visible->bottom - visible->top);

    if (!rage128_rect_in_vram(dev, dst, visible, *bytes_per_pixel))
        return false;
    if (*needs_source &&
        !rage128_rect_in_vram(dev, src, src_rect, *bytes_per_pixel))
        return false;

    return true;
}

static void
rage128_host_reset(rage128_t *dev)
{
    memset(&dev->host_data, 0, sizeof(dev->host_data));
}

static bool
rage128_host_begin(rage128_t *dev)
{
    rage128_host_data_t *host = &dev->host_data;
    rage128_rect_t       scissor;
    unsigned int         storage_bpp;
    unsigned int         bytes_per_pixel;
    uint32_t             pixel_mask;
    uint32_t             source_selector;
    uint32_t             source_datatype;
    uint8_t              rop;

    rage128_host_reset(dev);

    host->width  = R128_REG(dev, R128_DST_WIDTH) & 0x3fffU;
    host->height = R128_REG(dev, R128_DST_HEIGHT) & 0x3fffU;
    if (!host->width || !host->height)
        return false;

    if (!rage128_decode_format(dev, &storage_bpp, &bytes_per_pixel,
                               &pixel_mask))
        return false;

    if ((R128_REG(dev, R128_DP_CNTL) &
         (R128_DST_X_TILE_EN | R128_DST_Y_TILE_EN)) ||
        (R128_REG(dev, R128_DST_PITCH) & 0x00010000U)) {
        rage128_warn_unsupported(&dev->warned_tiling,
                                 "tiled HOST_DATA destinations are not implemented",
                                 R128_REG(dev, R128_DP_CNTL));
        return false;
    }

    source_selector = R128_REG(dev, R128_DP_MIX) & R128_DP_SRC_SOURCE;
    if (source_selector == R128_DP_SRC_HOST_BYTEALIGN) {
        rage128_warn_unsupported(&dev->warned_host_data,
                                 "byte-aligned HOST_DATA is not implemented",
                                 source_selector);
        return false;
    }
    if (source_selector != R128_DP_SRC_HOST)
        return false;

    if (!(R128_REG(dev, R128_DP_CNTL) & R128_DST_X_LEFT_TO_RIGHT) ||
        !(R128_REG(dev, R128_DP_CNTL) & R128_DST_Y_TOP_TO_BOTTOM)) {
        rage128_warn_unsupported(&dev->warned_host_direction,
                                 "reverse-direction HOST_DATA is not implemented",
                                 R128_REG(dev, R128_DP_CNTL));
        return false;
    }

    rop = (R128_REG(dev, R128_DP_MIX) & R128_DP_ROP3) >> 16;
    if (rage128_rop3_uses_pattern(rop) &&
        (R128_REG(dev, R128_DP_DATATYPE) & R128_DP_BRUSH_DATATYPE) !=
            R128_BRUSH_SOLIDCOLOR) {
        rage128_warn_unsupported(&dev->warned_brush,
                                 "non-solid HOST_DATA brushes are not implemented",
                                 R128_REG(dev, R128_DP_DATATYPE));
        return false;
    }

    source_datatype = R128_REG(dev, R128_DP_DATATYPE) &
                      R128_DP_SRC_DATATYPE;
    switch (source_datatype) {
        case R128_SRC_MONO_FRGD_BKGD:
            host->mono        = 1;
            host->transparent = 0;
            break;
        case R128_SRC_MONO_FRGD:
            host->mono        = 1;
            host->transparent = 1;
            break;
        case R128_SRC_COLOR:
            host->mono        = 0;
            host->transparent = 0;
            break;
        default:
            rage128_warn_unsupported(&dev->warned_host_data,
                                     "HOST_DATA source datatype is not implemented",
                                     source_datatype);
            return false;
    }

    if (!rage128_color_compare_supported(dev, true))
        return false;

    host->bytes_per_pixel = bytes_per_pixel;
    host->pixel_mask      = pixel_mask;
    host->rop             = rop;
    host->pattern         = R128_REG(dev, R128_DP_BRUSH_FRGD_CLR) &
                            pixel_mask;
    host->write_mask      = R128_REG(dev, R128_DP_WRITE_MASK) & pixel_mask;
    host->foreground      = R128_REG(dev, R128_DP_SRC_FRGD_CLR) & pixel_mask;
    host->background      = R128_REG(dev, R128_DP_SRC_BKGD_CLR) & pixel_mask;
    host->lsb_first       = !!(R128_REG(dev, R128_DP_DATATYPE) &
                              R128_DP_BYTE_PIX_ORDER);

    host->dst_offset = R128_REG(dev, R128_DST_OFFSET) & 0xfffffff0U;
    host->dst_stride = (R128_REG(dev, R128_DST_PITCH) & 0x3fffU) *
                       storage_bpp;
    host->dst_x      = (int32_t) (R128_REG(dev, R128_DST_X) & 0x3fffU);
    host->dst_y      = (int32_t) (R128_REG(dev, R128_DST_Y) & 0x3fffU);

    if (!host->dst_stride || host->dst_offset >= dev->vram_size)
        return false;

    rage128_decode_scissor(dev, &scissor, bytes_per_pixel);
    host->clip_left   = scissor.left;
    host->clip_top    = scissor.top;
    host->clip_right  = scissor.right;
    host->clip_bottom = scissor.bottom;

    host->active = 1;
    return true;
}

static bool
rage128_host_pixel_visible(const rage128_host_data_t *host, int x, int y)
{
    return x >= host->clip_left && x <= host->clip_right &&
           y >= host->clip_top && y <= host->clip_bottom;
}

static void
rage128_host_emit_pixel(rage128_t *dev, uint32_t source, bool write_pixel)
{
    rage128_host_data_t *host = &dev->host_data;
    int                  x;
    int                  y;

    if (host->complete)
        return;

    x = host->dst_x + (int) host->col;
    y = host->dst_y + (int) host->row;

    if (write_pixel && rage128_host_pixel_visible(host, x, y) &&
        x >= 0 && y >= 0) {
        uint64_t offset64 = (uint64_t) host->dst_offset +
                            (uint64_t) (unsigned int) y * host->dst_stride +
                            (uint64_t) (unsigned int) x *
                                host->bytes_per_pixel;

        if (offset64 + host->bytes_per_pixel <= dev->vram_size) {
            uint32_t offset = (uint32_t) offset64;
            uint32_t dest   = rage128_pixel_read(dev, offset,
                                                 host->bytes_per_pixel);
            uint32_t result;

            if (rage128_color_compare_pass(dev, source, dest,
                                           host->pixel_mask)) {
                result = rage128_rop3_eval(host->rop, host->pattern, source,
                                           dest & host->pixel_mask) &
                         host->pixel_mask;
                result = (result & host->write_mask) |
                         (dest & ~host->write_mask);
                rage128_pixel_write(dev, offset, host->bytes_per_pixel,
                                    result);
                rage128_dirty_row(dev, offset,
                                  offset + host->bytes_per_pixel - 1);
                dev->svga.fullchange =
                    dev->svga.monitor->mon_changeframecount;
            }
        }
    }

    host->col++;
    if (host->col < host->width)
        return;

    host->col = 0;
    host->row++;
    if (host->row >= host->height) {
        host->complete = 1;
        return;
    }

    if (host->mono)
        host->padding_remaining = (32U - (host->width & 31U)) & 31U;
    else {
        uint32_t row_bytes = host->width * host->bytes_per_pixel;
        host->padding_remaining = (4U - (row_bytes & 3U)) & 3U;
    }
}

static void
rage128_host_consume_mono_byte(rage128_t *dev, uint8_t value)
{
    rage128_host_data_t *host = &dev->host_data;

    for (unsigned int bit = 0; bit < 8; bit++) {
        bool foreground;

        if (host->complete)
            return;

        if (host->padding_remaining) {
            host->padding_remaining--;
            continue;
        }

        foreground = !!(value & (UINT8_C(1) <<
                                 (host->lsb_first ? bit : 7U - bit)));
        rage128_host_emit_pixel(dev,
                                foreground ? host->foreground :
                                             host->background,
                                foreground || !host->transparent);
    }
}

static void
rage128_host_consume_color_byte(rage128_t *dev, uint8_t value)
{
    rage128_host_data_t *host = &dev->host_data;

    if (host->complete)
        return;

    if (host->padding_remaining) {
        host->padding_remaining--;
        return;
    }

    host->pixel_accumulator |= (uint32_t) value <<
                               (host->pixel_bytes_used * 8U);
    host->pixel_bytes_used++;
    if (host->pixel_bytes_used < host->bytes_per_pixel)
        return;

    rage128_host_emit_pixel(dev, host->pixel_accumulator & host->pixel_mask,
                            true);
    host->pixel_accumulator = 0;
    host->pixel_bytes_used  = 0;
}

static void
rage128_host_push_word(rage128_t *dev, uint32_t value, bool final)
{
    rage128_host_data_t *host = &dev->host_data;

    if (!host->active) {
        rage128_warn_unsupported(&dev->warned_host_data,
                                 "HOST_DATA written without an active transfer",
                                 value);
        return;
    }

    for (unsigned int byte = 0; byte < 4; byte++) {
        uint8_t data = value >> (byte * 8U);

        if (host->mono)
            rage128_host_consume_mono_byte(dev, data);
        else
            rage128_host_consume_color_byte(dev, data);
    }

    if (final) {
        if (!host->complete)
            rage128_warn_unsupported(&dev->warned_host_early_end,
                                     "HOST_DATA_LAST ended an incomplete transfer",
                                     (host->row << 16) | host->col);
        host->active = 0;
    }
}

static int32_t
rage128_bres_term(uint32_t value)
{
    value &= 0x3ffffU;
    return (value & 0x20000U) ? (int32_t)(value | 0xfffc0000U) :
                                (int32_t)value;
}

static void
rage128_accel_line(rage128_t *dev)
{
    rage128_rect_t scissor;
    unsigned int storage_bpp;
    unsigned int bytes_per_pixel;
    uint32_t pixel_mask;
    uint32_t flags = R128_REG(dev, R128_DP_CNTL_XDIR_YDIR_YMAJOR);
    uint32_t length = R128_REG(dev, R128_DST_BRES_LNTH) & 0x7fffU;
    uint8_t rop;
    uint32_t brush;
    uint32_t pattern;
    uint32_t write_mask;
    uint32_t dst_offset;
    uint32_t dst_stride;
    int32_t error = rage128_bres_term(R128_REG(dev, R128_DST_BRES_ERR));
    int32_t increment = rage128_bres_term(R128_REG(dev, R128_DST_BRES_INC));
    int32_t decrement = rage128_bres_term(R128_REG(dev, R128_DST_BRES_DEC));
    int x = (int)(R128_REG(dev, R128_DST_X) & 0x3fffU);
    int y = (int)(R128_REG(dev, R128_DST_Y) & 0x3fffU);
    int x_step = (flags & R128_DST_LINE_X_LEFT_TO_RIGHT) ? 1 : -1;
    int y_step = (flags & R128_DST_LINE_Y_TOP_TO_BOTTOM) ? 1 : -1;
    bool y_major = !!(flags & R128_DST_LINE_Y_MAJOR);
    bool wrote = false;

    rage128_host_reset(dev);

    if (!length ||
        !rage128_decode_format(dev, &storage_bpp, &bytes_per_pixel,
                               &pixel_mask))
        return;

    if ((R128_REG(dev, R128_DP_CNTL) &
         (R128_DST_X_TILE_EN | R128_DST_Y_TILE_EN)) ||
        (R128_REG(dev, R128_DST_PITCH) & 0x00010000U)) {
        rage128_warn_unsupported(&dev->warned_tiling,
                                 "tiled line destinations are not implemented",
                                 R128_REG(dev, R128_DP_CNTL));
        return;
    }

    rop = (R128_REG(dev, R128_DP_MIX) & R128_DP_ROP3) >> 16;
    brush = R128_REG(dev, R128_DP_DATATYPE) & R128_DP_BRUSH_DATATYPE;
    if (rage128_rop3_uses_pattern(rop) &&
        !rage128_brush_supported(brush)) {
        rage128_warn_unsupported(&dev->warned_brush,
                                 "line brush datatype is not implemented",
                                 R128_REG(dev, R128_DP_DATATYPE));
        return;
    }
    if (rage128_rop3_uses_source(rop)) {
        rage128_warn_unsupported(&dev->warned_source,
                                 "source-dependent line ROP is not implemented",
                                 rop);
        return;
    }
    if (!rage128_color_compare_supported(dev, false))
        return;

    dst_offset = R128_REG(dev, R128_DST_OFFSET) & 0xfffffff0U;
    dst_stride = (R128_REG(dev, R128_DST_PITCH) & 0x3fffU) * storage_bpp;
    if (!dst_stride || dst_offset >= dev->vram_size)
        return;

    rage128_decode_scissor(dev, &scissor, bytes_per_pixel);
    pattern = R128_REG(dev, R128_DP_BRUSH_FRGD_CLR) & pixel_mask;
    write_mask = R128_REG(dev, R128_DP_WRITE_MASK) & pixel_mask;

    for (uint32_t i = 0; i < length; i++) {
        if (x >= scissor.left && x <= scissor.right &&
            y >= scissor.top && y <= scissor.bottom &&
            x >= 0 && y >= 0) {
            uint64_t offset64 = (uint64_t)dst_offset +
                                (uint64_t)(unsigned int)y * dst_stride +
                                (uint64_t)(unsigned int)x * bytes_per_pixel;

            if (offset64 + bytes_per_pixel <= dev->vram_size) {
                uint32_t offset = (uint32_t)offset64;
                uint32_t dest = rage128_pixel_read(dev, offset,
                                                   bytes_per_pixel);

                {
                    uint32_t pixel_pattern = pattern;
                    bool draw = !rage128_rop3_uses_pattern(rop) ||
                                rage128_brush_pixel(dev, brush, x, y, i,
                                                    true, &pixel_pattern);

                    if (draw &&
                        rage128_color_compare_pass(dev, 0, dest, pixel_mask)) {
                        uint32_t result = rage128_rop3_eval(rop, pixel_pattern,
                                                           0,
                                                           dest & pixel_mask) &
                                          pixel_mask;
                        result = (result & write_mask) |
                                 (dest & ~write_mask);
                        rage128_pixel_write(dev, offset, bytes_per_pixel,
                                            result);
                        rage128_dirty_row(dev, offset,
                                          offset + bytes_per_pixel - 1);
                        wrote = true;
                    }
                }
            }
        }

        if (y_major) {
            y += y_step;
            if (error >= 0) {
                error += decrement;
                x += x_step;
            } else {
                error += increment;
            }
        } else {
            x += x_step;
            if (error >= 0) {
                error += decrement;
                y += y_step;
            } else {
                error += increment;
            }
        }
    }

    R128_REG(dev, R128_DST_X) = (uint32_t)x & 0x3fffU;
    R128_REG(dev, R128_DST_Y) = (uint32_t)y & 0x3fffU;
    R128_REG(dev, R128_DST_BRES_ERR) = (uint32_t)error & 0x3ffffU;
    if (wrote)
        dev->svga.fullchange = dev->svga.monitor->mon_changeframecount;
}

static void
rage128_accel_run(rage128_t *dev)
{
    rage128_surface_t src;
    rage128_surface_t dst;
    rage128_rect_t    src_rect;
    rage128_rect_t    dst_rect;
    rage128_rect_t    visible;
    unsigned int      bytes_per_pixel;
    uint32_t          pixel_mask;
    uint8_t           rop;
    bool              needs_source;
    bool              left_to_right;
    bool              top_to_bottom;
    uint32_t          brush;
    uint32_t          pattern;
    uint32_t          write_mask;
    uint32_t          source_selector;
    int               rows;
    int               cols;

    source_selector = R128_REG(dev, R128_DP_MIX) & R128_DP_SRC_SOURCE;
    if (source_selector == R128_DP_SRC_HOST ||
        source_selector == R128_DP_SRC_HOST_BYTEALIGN) {
        rage128_host_begin(dev);
        return;
    }

    rage128_host_reset(dev);

    if (!rage128_accel_prepare(dev, &src, &dst, &src_rect, &dst_rect,
                               &visible, &bytes_per_pixel, &pixel_mask,
                               &rop, &needs_source))
        return;

    left_to_right = !!(R128_REG(dev, R128_DP_CNTL) &
                       R128_DST_X_LEFT_TO_RIGHT);
    top_to_bottom = !!(R128_REG(dev, R128_DP_CNTL) &
                       R128_DST_Y_TOP_TO_BOTTOM);
    brush         = R128_REG(dev, R128_DP_DATATYPE) &
                    R128_DP_BRUSH_DATATYPE;
    pattern       = R128_REG(dev, R128_DP_BRUSH_FRGD_CLR) & pixel_mask;
    write_mask    = R128_REG(dev, R128_DP_WRITE_MASK) & pixel_mask;
    rows          = visible.bottom - visible.top + 1;
    cols          = visible.right - visible.left + 1;

    for (int row = 0; row < rows; row++) {
        int dy = top_to_bottom ? visible.top + row : visible.bottom - row;
        int sy = top_to_bottom ? src_rect.top + row : src_rect.bottom - row;
        uint32_t dirty_first = UINT32_MAX;
        uint32_t dirty_last  = 0;

        for (int col = 0; col < cols; col++) {
            int dx = left_to_right ? visible.left + col : visible.right - col;
            int sx = left_to_right ? src_rect.left + col : src_rect.right - col;
            uint32_t dst_offset = dst.offset + (uint32_t) dy * dst.stride +
                                  (uint32_t) dx * bytes_per_pixel;
            uint32_t source = 0;
            uint32_t pixel_pattern = pattern;
            uint32_t dest;
            uint32_t result;

            if (rage128_rop3_uses_pattern(rop) &&
                !rage128_brush_pixel(dev, brush, dx, dy, 0, false,
                                    &pixel_pattern))
                continue;

            if (needs_source) {
                uint32_t src_offset = src.offset + (uint32_t) sy * src.stride +
                                      (uint32_t) sx * bytes_per_pixel;
                source = rage128_pixel_read(dev, src_offset, bytes_per_pixel) &
                         pixel_mask;
            }

            dest = rage128_pixel_read(dev, dst_offset, bytes_per_pixel);
            if (!rage128_color_compare_pass(dev, source, dest, pixel_mask))
                continue;

            result = rage128_rop3_eval(rop, pixel_pattern, source,
                                       dest & pixel_mask) & pixel_mask;
            result = (result & write_mask) | (dest & ~write_mask);
            rage128_pixel_write(dev, dst_offset, bytes_per_pixel, result);

            if (dst_offset < dirty_first)
                dirty_first = dst_offset;
            if (dst_offset + bytes_per_pixel - 1 > dirty_last)
                dirty_last = dst_offset + bytes_per_pixel - 1;
        }

        if (dirty_first != UINT32_MAX)
            rage128_dirty_row(dev, dirty_first, dirty_last);
    }

    dev->svga.fullchange = dev->svga.monitor->mon_changeframecount;
}

void
rage128_accel_reset(rage128_t *dev)
{
    memset(&dev->regs[R128_REG_INDEX(R128_DST_OFFSET)], 0,
           (R128_REG_INDEX(R128_HOST_DATA_LAST) -
            R128_REG_INDEX(R128_DST_OFFSET) + 1) *
               sizeof(dev->regs[0]));
    R128_REG(dev, R128_PC_NGUI_CTLSTAT) = 0;

    R128_REG(dev, R128_DP_CNTL)       = R128_DST_X_LEFT_TO_RIGHT |
                                         R128_DST_Y_TOP_TO_BOTTOM;
    R128_REG(dev, R128_DP_DATATYPE)   = R128_DST_8BPP |
                                         R128_BRUSH_SOLIDCOLOR |
                                         R128_SRC_COLOR;
    R128_REG(dev, R128_DP_MIX)        = (R128_ROP3_SRCCOPY << 16) |
                                         R128_DP_SRC_RECT;
    R128_REG(dev, R128_DP_WRITE_MASK) = 0xffffffffU;

    R128_REG(dev, R128_DEFAULT_SC_BOTTOM_RIGHT) = 0x3fff3fffU;
    R128_REG(dev, R128_SC_TOP_LEFT)              = 0x00000000U;
    R128_REG(dev, R128_SC_BOTTOM_RIGHT)          = 0x3fff3fffU;
    R128_REG(dev, R128_SRC_SC_BOTTOM_RIGHT)      = 0x3fff3fffU;
    R128_REG(dev, R128_SC_LEFT)                  = 0;
    R128_REG(dev, R128_SC_TOP)                   = 0;
    R128_REG(dev, R128_SC_RIGHT)                 = 0x3fffU;
    R128_REG(dev, R128_SC_BOTTOM)                = 0x3fffU;
    R128_REG(dev, R128_SRC_SC_RIGHT)             = 0x3fffU;
    R128_REG(dev, R128_SRC_SC_BOTTOM)            = 0x3fffU;
    R128_REG(dev, R128_CLR_CMP_CNTL)             = 0;
    R128_REG(dev, R128_CLR_CMP_MASK)             = 0xffffffffU;

    rage128_host_reset(dev);

    dev->warned_brush          = 0;
    dev->warned_source         = 0;
    dev->warned_tiling         = 0;
    dev->warned_host_data      = 0;
    dev->warned_host_direction = 0;
    dev->warned_host_early_end = 0;
    dev->warned_color_compare  = 0;
}

void
rage128_accel_reg_written(rage128_t *dev, uint32_t reg)
{
    uint32_t value = R128_REG(dev, reg);

    switch (reg) {
        case R128_SRC_PITCH_OFFSET:
            R128_REG(dev, R128_SRC_OFFSET) = (value & 0x001fffffU) << 5;
            R128_REG(dev, R128_SRC_PITCH)  =
                ((value & 0x7fe00000U) >> 21) |
                ((value >> 15) & 0x00010000U);
            break;

        case R128_DST_PITCH_OFFSET:
            R128_REG(dev, R128_DST_OFFSET) = (value & 0x001fffffU) << 5;
            R128_REG(dev, R128_DST_PITCH)  =
                ((value & 0x7fe00000U) >> 21) |
                ((value >> 15) & 0x00010000U);
            break;

        case R128_SRC_Y_X:
            R128_REG(dev, R128_SRC_X) = value & 0x3fffU;
            R128_REG(dev, R128_SRC_Y) = (value >> 16) & 0x3fffU;
            break;

        case R128_DST_Y_X:
            R128_REG(dev, R128_DST_X) = value & 0x3fffU;
            R128_REG(dev, R128_DST_Y) = (value >> 16) & 0x3fffU;
            break;

        case R128_SRC_X_Y:
            R128_REG(dev, R128_SRC_Y) = value & 0x3fffU;
            R128_REG(dev, R128_SRC_X) = (value >> 16) & 0x3fffU;
            break;

        case R128_DST_X_Y:
            R128_REG(dev, R128_DST_Y) = value & 0x3fffU;
            R128_REG(dev, R128_DST_X) = (value >> 16) & 0x3fffU;
            break;

        case R128_DST_HEIGHT_WIDTH:
        case R128_DST_HEIGHT_WIDTH_8:
        case R128_DST_HEIGHT_WIDTH_BW:
            R128_REG(dev, R128_DST_WIDTH)  = value & 0x3fffU;
            R128_REG(dev, R128_DST_HEIGHT) = (value >> 16) & 0x3fffU;
            rage128_accel_run(dev);
            break;

        case R128_DST_WIDTH_HEIGHT:
            R128_REG(dev, R128_DST_HEIGHT) = value & 0x3fffU;
            R128_REG(dev, R128_DST_WIDTH)  = (value >> 16) & 0x3fffU;
            rage128_accel_run(dev);
            break;

        case R128_DST_WIDTH_X:
            R128_REG(dev, R128_DST_X)     = value & 0x3fffU;
            R128_REG(dev, R128_DST_WIDTH) = (value >> 16) & 0x3fffU;
            rage128_accel_run(dev);
            break;

        case R128_DST_HEIGHT_Y:
            R128_REG(dev, R128_DST_Y)      = value & 0x3fffU;
            R128_REG(dev, R128_DST_HEIGHT) = (value >> 16) & 0x3fffU;
            break;

        case R128_SC_TOP_LEFT:
            R128_REG(dev, R128_SC_LEFT) = value & 0x3fffU;
            R128_REG(dev, R128_SC_TOP)  = (value >> 16) & 0x3fffU;
            break;

        case R128_SC_BOTTOM_RIGHT:
            R128_REG(dev, R128_SC_RIGHT)  = value & 0x3fffU;
            R128_REG(dev, R128_SC_BOTTOM) = (value >> 16) & 0x3fffU;
            break;

        case R128_SRC_SC_BOTTOM_RIGHT:
            R128_REG(dev, R128_SRC_SC_RIGHT)  = value & 0x3fffU;
            R128_REG(dev, R128_SRC_SC_BOTTOM) = (value >> 16) & 0x3fffU;
            break;

        case R128_DEFAULT_SC_BOTTOM_RIGHT:
            if (!(R128_REG(dev, R128_DP_GUI_MASTER_CNTL) &
                  R128_GMC_SRC_CLIPPING)) {
                R128_REG(dev, R128_SRC_SC_RIGHT)  = value & 0x3fffU;
                R128_REG(dev, R128_SRC_SC_BOTTOM) =
                    (value >> 16) & 0x3fffU;
            }
            if (!(R128_REG(dev, R128_DP_GUI_MASTER_CNTL) &
                  R128_GMC_DST_CLIPPING)) {
                R128_REG(dev, R128_SC_LEFT)   = 0;
                R128_REG(dev, R128_SC_TOP)    = 0;
                R128_REG(dev, R128_SC_RIGHT)  = value & 0x3fffU;
                R128_REG(dev, R128_SC_BOTTOM) =
                    (value >> 16) & 0x3fffU;
            }
            break;

        case R128_DP_GUI_MASTER_CNTL:
            R128_REG(dev, R128_DP_DATATYPE) =
                ((value & 0x00000f00U) >> 8) |
                ((value & 0x000030f0U) << 4) |
                ((value & 0x00004000U) << 16);
            R128_REG(dev, R128_DP_MIX) =
                (value & R128_GMC_ROP3_MASK) |
                ((value & 0x07000000U) >> 16);

            if (!(value & R128_GMC_SRC_PITCH_OFFSET_CNTL)) {
                R128_REG(dev, R128_SRC_OFFSET) =
                    R128_REG(dev, R128_DEFAULT_OFFSET);
                R128_REG(dev, R128_SRC_PITCH) =
                    R128_REG(dev, R128_DEFAULT_PITCH);
            }
            if (!(value & R128_GMC_DST_PITCH_OFFSET_CNTL)) {
                R128_REG(dev, R128_DST_OFFSET) =
                    R128_REG(dev, R128_DEFAULT_OFFSET);
                R128_REG(dev, R128_DST_PITCH) =
                    R128_REG(dev, R128_DEFAULT_PITCH);
            }
            if (!(value & R128_GMC_SRC_CLIPPING)) {
                uint32_t sc = R128_REG(dev, R128_DEFAULT_SC_BOTTOM_RIGHT);
                R128_REG(dev, R128_SRC_SC_RIGHT)  = sc & 0x3fffU;
                R128_REG(dev, R128_SRC_SC_BOTTOM) = (sc >> 16) & 0x3fffU;
            }
            if (!(value & R128_GMC_DST_CLIPPING)) {
                uint32_t sc = R128_REG(dev, R128_DEFAULT_SC_BOTTOM_RIGHT);
                R128_REG(dev, R128_SC_LEFT)   = 0;
                R128_REG(dev, R128_SC_TOP)    = 0;
                R128_REG(dev, R128_SC_RIGHT)  = sc & 0x3fffU;
                R128_REG(dev, R128_SC_BOTTOM) = (sc >> 16) & 0x3fffU;
            }
            break;

        case R128_DST_WIDTH:
            rage128_accel_run(dev);
            break;

        case R128_DST_BRES_LNTH:
            rage128_accel_line(dev);
            break;

        case R128_HOST_DATA0 ... R128_HOST_DATA_LAST:
            rage128_host_push_word(dev, value, reg == R128_HOST_DATA_LAST);
            break;

        default:
            break;
    }
}
