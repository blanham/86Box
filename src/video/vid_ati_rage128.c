/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          ATI Rage 128 Pro graphics card emulation.
 *
 *          This first implementation models the PCI/AGP identity, VGA
 *          compatibility, linear framebuffer, display controller, palette,
 *          hardware cursor, vblank interrupt, and the synchronous 2D engine.
 *          CCE/PM4 command submission and a deterministic software
 *          fixed-function 3D reference path are also modeled. Texturing,
 *          fog, stencil, overlays, and capture remain fail-closed.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <86box/86box.h>
#include <86box/device.h>
#include <86box/i2c.h>
#include <86box/io.h>
#include <86box/mem.h>
#include <86box/pci.h>
#include <86box/plat_unused.h>
#include <86box/rom.h>
#include <86box/timer.h>
#include <86box/video.h>
#include <86box/vid_ddc.h>
#include <86box/vid_svga.h>
#include <86box/vid_svga_render.h>
#include "cpu.h"

#include "vid_ati_rage128.h"

#define R128_DEFAULT_BIOS_PF \
    "roms/video/ati_rage128/rage128pro.bin"
#define R128_DEFAULT_BIOS_RE_XPERT \
    "roms/video/ati_rage128/113-57403-102.rom"
#define R128_DEFAULT_BIOS_RE_AIW \
    "roms/video/ati_rage128/113-53008-100.rom"

/*
 * This development fork carries the verified ROM images below.
 *
 * 113-57403-102.rom (1002:5245 / 1002:0008):
 *   SHA-256 91837fab2f2a71df54d3f031fd15bc5ed658b148d2df806b05e30f370fe60afe
 * 113-53008-100.rom (1002:5245 / 1002:0068):
 *   SHA-256 9100ea06532a08e50afd496ddddb943ff69e9b8dbf9b03bd375a4de17a85741b
 */

#define R128_PCI_BAR0_MASK 0xfc000000U
#define R128_PCI_BAR1_MASK 0xffffff00U
#define R128_PCI_BAR2_MASK 0xffffc000U

#define R128_PCI_BAR0_FLAGS 0x08U /* prefetchable memory */
#define R128_PCI_BAR1_FLAGS 0x01U /* I/O space */

#define R128_SOFT_RESET_GUI 0x00000001U

static video_timings_t timing_rage128_agp = {
    .type    = VIDEO_AGP,
    .write_b = 2,
    .write_w = 2,
    .write_l = 1,
    .read_b  = 20,
    .read_w  = 20,
    .read_l  = 21
};

static video_timings_t timing_rage128_pci = {
    .type    = VIDEO_PCI,
    .write_b = 2,
    .write_w = 2,
    .write_l = 1,
    .read_b  = 20,
    .read_w  = 20,
    .read_l  = 21
};

static uint8_t  rage128_pci_read(int func, int addr, int len, void *priv);
static void     rage128_pci_write(int func, int addr, int len, uint8_t val,
                                  void *priv);
static uint32_t rage128_mmio_read_access(rage128_t *dev, uint32_t addr,
                                         unsigned int size);
static void     rage128_mmio_write_access(rage128_t *dev, uint32_t addr,
                                          uint32_t value,
                                          unsigned int size);
static uint8_t  rage128_io_readb(uint16_t port, void *priv);
static uint16_t rage128_io_readw(uint16_t port, void *priv);
static uint32_t rage128_io_readl(uint16_t port, void *priv);
static void     rage128_io_writeb(uint16_t port, uint8_t val, void *priv);
static void     rage128_io_writew(uint16_t port, uint16_t val, void *priv);
static void     rage128_io_writel(uint16_t port, uint32_t val, void *priv);
static void     rage128_recalc_mapping(rage128_t *dev);

static uint32_t
rage128_load_le(const uint8_t *ptr, unsigned int size)
{
    uint32_t value = 0;

    for (unsigned int i = 0; i < size; i++)
        value |= (uint32_t) ptr[i] << (i * 8);

    return value;
}

static void
rage128_store_le(uint8_t *ptr, unsigned int size, uint32_t value)
{
    for (unsigned int i = 0; i < size; i++)
        ptr[i] = value >> (i * 8);
}

static uint8_t
rage128_pm_cap_offset(const rage128_t *dev)
{
    return dev->is_agp ? R128_PCI_PM_CAP_OFFSET_PF :
                         R128_PCI_PM_CAP_OFFSET_RE;
}

static const char *
rage128_default_bios(const rage128_t *dev)
{
    if (dev->device_id == R128_PCI_DEVICE_ID_RE) {
        return dev->subsystem_id == R128_PCI_SUBSYSTEM_AIW ?
               R128_DEFAULT_BIOS_RE_AIW : R128_DEFAULT_BIOS_RE_XPERT;
    }
    return R128_DEFAULT_BIOS_PF;
}

static void
rage128_gpio_monid_update(rage128_t *dev)
{
    uint32_t value = R128_REG(dev, R128_GPIO_MONID);

    /*
     * The Rage 128 MONID pins are open-drain. EN selects output drive and
     * the corresponding A bit supplies the driven level; a disabled output
     * releases the line high. The historical r128 driver uses pins 2/1 for
     * the VGA DDC clock/data pair and enables MASK_1 before transactions.
     */
    if (dev->i2c && (value & R128_GPIO_MONID_MASK_1)) {
        uint8_t scl = !(value & R128_GPIO_MONID_EN_2) ||
                      !!(value & R128_GPIO_MONID_A_2);
        uint8_t sda = !(value & R128_GPIO_MONID_EN_1) ||
                      !!(value & R128_GPIO_MONID_A_1);

        i2c_gpio_set(dev->i2c, scl, sda);
    }

    value &= ~R128_GPIO_MONID_Y_MASK;
    if (!dev->i2c || i2c_gpio_get_sda(dev->i2c))
        value |= R128_GPIO_MONID_Y_1;
    if (!dev->i2c || i2c_gpio_get_scl(dev->i2c))
        value |= R128_GPIO_MONID_Y_2;
    R128_REG(dev, R128_GPIO_MONID) = value;
}

static bool
rage128_vram_range_valid(const rage128_t *dev, uint32_t offset,
                         unsigned int size)
{
    return offset < dev->vram_size && size <= dev->vram_size - offset;
}

static uint32_t
rage128_vram_read(rage128_t *dev, uint32_t offset, unsigned int size)
{
    if (!rage128_vram_range_valid(dev, offset, size))
        return (size == 1) ? 0xffU : (size == 2) ? 0xffffU : 0xffffffffU;

    return rage128_load_le(&dev->svga.vram[offset], size);
}

static void
rage128_vram_write(rage128_t *dev, uint32_t offset, uint32_t value,
                   unsigned int size)
{
    uint32_t first_page;
    uint32_t last_page;

    if (!rage128_vram_range_valid(dev, offset, size))
        return;

    rage128_store_le(&dev->svga.vram[offset], size, value);

    first_page = offset >> 12;
    last_page  = (offset + size - 1) >> 12;
    for (uint32_t page = first_page; page <= last_page; page++)
        dev->svga.changedvram[page] =
            dev->svga.monitor->mon_changeframecount;
}

void
rage128_mark_vram_dirty(rage128_t *dev, uint64_t start, uint64_t length)
{
    uint64_t end;
    uint32_t first_page;
    uint32_t last_page;

    if (!length || start >= dev->vram_size)
        return;
    end = start + length;
    if (end < start || end > dev->vram_size)
        end = dev->vram_size;
    first_page = (uint32_t)(start >> 12);
    last_page = (uint32_t)((end - 1) >> 12);
    for (uint32_t page = first_page; page <= last_page; page++)
        dev->svga.changedvram[page] =
            dev->svga.monitor->mon_changeframecount;
    dev->svga.fullchange = dev->svga.monitor->mon_changeframecount;
}

static uint32_t
rage128_linear_offset(const rage128_t *dev, uint32_t addr)
{
    return (addr - dev->linear_base) & (R128_LINEAR_APERTURE_SIZE - 1);
}

static uint8_t
rage128_linear_readb(uint32_t addr, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_read_b;
    return rage128_vram_read(dev, rage128_linear_offset(dev, addr), 1);
}

static uint16_t
rage128_linear_readw(uint32_t addr, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_read_w;
    return rage128_vram_read(dev, rage128_linear_offset(dev, addr), 2);
}

static uint32_t
rage128_linear_readl(uint32_t addr, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_read_l;
    return rage128_vram_read(dev, rage128_linear_offset(dev, addr), 4);
}

static void
rage128_linear_writeb(uint32_t addr, uint8_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_write_b;
    rage128_vram_write(dev, rage128_linear_offset(dev, addr), val, 1);
}

static void
rage128_linear_writew(uint32_t addr, uint16_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_write_w;
    rage128_vram_write(dev, rage128_linear_offset(dev, addr), val, 2);
}

static void
rage128_linear_writel(uint32_t addr, uint32_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    cycles -= dev->svga.monitor->mon_video_timing_write_l;
    rage128_vram_write(dev, rage128_linear_offset(dev, addr), val, 4);
}

static void
rage128_update_irq(rage128_t *dev)
{
    bool disabled = dev->power_state != R128_PCI_PM_STATE_D0 ||
                    !!(dev->pci_regs[PCI_REG_COMMAND_H] &
                       PCI_COMMAND_H_INT_DIS);
    bool pending  = !!(R128_REG(dev, R128_GEN_INT_STATUS) &
                       R128_REG(dev, R128_GEN_INT_CNTL));

    if (pending && !disabled)
        pci_set_irq(dev->pci_slot, PCI_INTA, &dev->irq_state);
    else
        pci_clear_irq(dev->pci_slot, PCI_INTA, &dev->irq_state);
}

static void
rage128_vblank_start(svga_t *svga)
{
    rage128_t *dev = (rage128_t *) svga->priv;

    R128_REG(dev, R128_GEN_INT_STATUS) |= R128_CRTC_VBLANK_INT;
    rage128_update_irq(dev);
}

static double
rage128_pixel_clock(const rage128_t *dev)
{
    static const unsigned int post_dividers[8] = { 1, 2, 4, 8, 3, 6, 12, 1 };
    uint32_t ref_reg = dev->pll_regs[R128_PPLL_REF_DIV];
    uint32_t div_reg = dev->pll_regs[R128_PPLL_DIV_3];
    uint32_t ref_div = ref_reg & R128_PPLL_REF_DIV_MASK;
    uint32_t fb_div  = div_reg & R128_PPLL_FB3_DIV_MASK;
    uint32_t post_sel = (div_reg & R128_PPLL_POST3_DIV_MASK) >> 16;

    if (!ref_div || !fb_div)
        return 25175000.0;

    return 27000000.0 * (double) fb_div /
           ((double) ref_div * post_dividers[post_sel & 7]);
}

static void
rage128_update_cursor(rage128_t *dev)
{
    svga_t  *svga = &dev->svga;
    uint32_t pos  = R128_REG(dev, R128_CUR_HORZ_VERT_POSN);
    uint32_t offs = R128_REG(dev, R128_CUR_HORZ_VERT_OFF);
    uint32_t base = R128_REG(dev, R128_CUR_OFFSET) & 0x07fffff0U;
    uint32_t adjust;

    svga->hwcursor.x         = (pos >> 16) & 0x3fffU;
    svga->hwcursor.y         = pos & 0x0fffU;
    svga->hwcursor.xoff      = (offs >> 16) & 0x3fU;
    svga->hwcursor.yoff      = offs & 0x3fU;
    svga->hwcursor.cur_xsize = 64;
    svga->hwcursor.cur_ysize = 64;
    svga->hwcursor.ena       = !!(R128_REG(dev, R128_CRTC_GEN_CNTL) &
                                  R128_CRTC_CUR_EN);

    adjust = (uint32_t) svga->hwcursor.xoff +
             (uint32_t) svga->hwcursor.yoff * 16U;
    svga->hwcursor.addr = (base >= adjust) ? base - adjust : base;
}

static void
rage128_hwcursor_draw(svga_t *svga, int displine)
{
    const rage128_t *dev = (const rage128_t *) svga->priv;
    uint64_t         and_bits = 0;
    uint64_t         xor_bits = 0;
    uint32_t         address  = svga->hwcursor_latch.addr;
    int              cursor_x = svga->hwcursor_latch.x -
                                svga->hwcursor_latch.xoff;
    uint32_t         color0;
    uint32_t         color1;
    uint32_t        *line;

    if (address > dev->vram_size - 16 || displine < 0 ||
        displine >= buffer32->h)
        return;

    for (unsigned int i = 0; i < 8; i++) {
        and_bits = (and_bits << 8) | svga->vram[address + i];
        xor_bits = (xor_bits << 8) | svga->vram[address + 8 + i];
    }

    color0 = makecol32((R128_REG(dev, R128_CUR_CLR0) >> 16) & 0xff,
                       (R128_REG(dev, R128_CUR_CLR0) >> 8) & 0xff,
                       R128_REG(dev, R128_CUR_CLR0) & 0xff);
    color1 = makecol32((R128_REG(dev, R128_CUR_CLR1) >> 16) & 0xff,
                       (R128_REG(dev, R128_CUR_CLR1) >> 8) & 0xff,
                       R128_REG(dev, R128_CUR_CLR1) & 0xff);
    line = buffer32->line[displine];

    for (unsigned int i = 0; i < 64; i++) {
        uint64_t mask = UINT64_C(1) << (63 - i);
        int      x    = cursor_x + (int) i;
        int      out_x = svga->x_add + x;
        bool     a = !!(and_bits & mask);
        bool     b = !!(xor_bits & mask);

        if (x < svga->hwcursor_latch.x || out_x < 0 || out_x >= buffer32->w)
            continue;

        if (a) {
            if (b)
                line[out_x] ^= 0x00ffffffU;
        } else {
            line[out_x] = b ? color1 : color0;
        }
    }

    svga->hwcursor_latch.addr += 16;
}

static void
rage128_recalctimings(svga_t *svga)
{
    rage128_t *dev = (rage128_t *) svga->priv;
    uint32_t   gen = R128_REG(dev, R128_CRTC_GEN_CNTL);
    bool       extended;
    int        bpp = 0;
    int        bytes_per_pixel = 1;
    int        hdisp;
    int        vdisp;
    int        htotal;
    int        vtotal;
    uint32_t   pitch_pixels;
    uint32_t   offset;

    extended = !!(gen & R128_CRTC_EXT_DISP_EN) &&
               !!(gen & R128_CRTC_EN) &&
               !(R128_REG(dev, R128_CRTC_EXT_CNTL) &
                 R128_CRTC_DISPLAY_DIS);

    svga->vram_display_mask = dev->vram_mask;
    rage128_update_cursor(dev);

    if (!extended) {
        svga->fb_only       = 0;
        svga->hoverride     = 0;
        svga->panning_blank = 0;
        svga->packed_4bpp   = 0;
        svga->adv_flags    &= ~FLAG_NO_SHIFT3;
        return;
    }

    switch (gen & R128_CRTC_PIX_WIDTH_MASK) {
        case R128_CRTC_PIX_WIDTH_4BPP:
            bpp = 4;
            break;
        case R128_CRTC_PIX_WIDTH_8BPP:
            bpp = 8;
            break;
        case R128_CRTC_PIX_WIDTH_15BPP:
            bpp = 15;
            bytes_per_pixel = 2;
            break;
        case R128_CRTC_PIX_WIDTH_16BPP:
            bpp = 16;
            bytes_per_pixel = 2;
            break;
        case R128_CRTC_PIX_WIDTH_24BPP:
            bpp = 24;
            bytes_per_pixel = 3;
            break;
        case R128_CRTC_PIX_WIDTH_32BPP:
            bpp = 32;
            bytes_per_pixel = 4;
            break;
        default:
            return;
    }

    hdisp = (int) (((R128_REG(dev, R128_CRTC_H_TOTAL_DISP) >> 16) &
                    0x07ffU) + 1) * 8;
    vdisp = (int) (((R128_REG(dev, R128_CRTC_V_TOTAL_DISP) >> 16) &
                    0x0fffU) + 1);
    htotal = (int) ((R128_REG(dev, R128_CRTC_H_TOTAL_DISP) & 0x07ffU) +
                    1) * 8;
    vtotal = (int) ((R128_REG(dev, R128_CRTC_V_TOTAL_DISP) & 0x0fffU) +
                    1);

    if (hdisp <= 8)
        hdisp = 640;
    if (vdisp <= 1)
        vdisp = 480;
    if (htotal <= hdisp)
        htotal = hdisp + MAX(160, hdisp / 4);
    if (vtotal <= vdisp)
        vtotal = vdisp + 45;

    pitch_pixels = (R128_REG(dev, R128_CRTC_PITCH) & 0x07ffU) * 8U;
    if (!pitch_pixels)
        pitch_pixels = hdisp;

    offset = R128_REG(dev, R128_CRTC_OFFSET) & 0x07ffffffU;
    if (offset >= dev->vram_size)
        offset = 0;

    svga->char_width    = 1;
    svga->dots_per_clock = 1;
    svga->clock         = (cpuclock * (double) (UINT64_C(1) << 32)) /
                          rage128_pixel_clock(dev);
    svga->hdisp         = hdisp;
    svga->hdisp_time    = hdisp;
    svga->htotal        = htotal;
    svga->h_total       = htotal;
    svga->hblankstart   = hdisp;
    svga->hblankend     = htotal - 1;
    svga->dispend       = vdisp;
    svga->vdisp         = vdisp;
    svga->vtotal        = vtotal;
    svga->vblankstart   = vdisp;
    svga->vblankend     = vtotal - 1;
    svga->vsyncstart    = (R128_REG(dev, R128_CRTC_V_SYNC_STRT_WID) &
                           0x0fffU) + 1;
    svga->split         = 0xffffff;
    svga->rowcount      = 0;
    svga->linedbl       = !!(gen & R128_CRTC_DBL_SCAN_EN);
    svga->interlace     = !!(gen & R128_CRTC_INTERLACE_EN);
    svga->lowres        = 0;
    svga->hoverride     = 1;
    svga->panning_blank = 1;
    svga->fb_only       = 1;
    svga->bpp           = bpp;
    svga->packed_4bpp   = (bpp == 4);
    svga->adv_flags    |= FLAG_NO_SHIFT3;
    svga->memaddr_latch = offset;
    svga->rowoffset     = (bpp == 4) ? (pitch_pixels / 2) :
                                      (pitch_pixels * bytes_per_pixel);
    svga->dpms          = 0;

    switch (bpp) {
        case 4:
            svga->render = svga_render_4bpp_highres;
            break;
        case 8:
            svga->render = svga_render_8bpp_clone_highres;
            break;
        case 15:
            svga->render = svga_render_15bpp_highres;
            break;
        case 16:
            svga->render = svga_render_16bpp_highres;
            break;
        case 24:
            svga->render = svga_render_24bpp_highres;
            break;
        case 32:
            svga->render = svga_render_32bpp_highres;
            break;
        default:
            break;
    }

    svga->fullchange = svga->monitor->mon_changeframecount;
}

static void
rage128_vga_out(uint16_t addr, uint8_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    svga_out(addr, val, &dev->svga);
    if (!(dev->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_MEM))
        mem_mapping_disable(&dev->svga.mapping);
}

static uint8_t
rage128_vga_in(uint16_t addr, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    return svga_in(addr, &dev->svga);
}

static void
rage128_unmap_io(rage128_t *dev)
{
    if (dev->vga_io_mapped) {
        io_removehandler(0x03c0, 0x0020, rage128_vga_in, NULL, NULL,
                         rage128_vga_out, NULL, NULL, dev);
        dev->vga_io_mapped = 0;
    }

    if (dev->io_bar_mapped) {
        io_removehandler(dev->mapped_io_base, R128_IO_SIZE,
                         rage128_io_readb, rage128_io_readw,
                         rage128_io_readl, rage128_io_writeb,
                         rage128_io_writew, rage128_io_writel, dev);
        dev->io_bar_mapped = 0;
        dev->mapped_io_base = 0;
    }
}

static uint8_t
rage128_io_readb(uint16_t port, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    return rage128_mmio_read_access(dev, port - dev->io_base, 1);
}

static uint16_t
rage128_io_readw(uint16_t port, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    return rage128_mmio_read_access(dev, port - dev->io_base, 2);
}

static uint32_t
rage128_io_readl(uint16_t port, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    return rage128_mmio_read_access(dev, port - dev->io_base, 4);
}

static void
rage128_io_writeb(uint16_t port, uint8_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    rage128_mmio_write_access(dev, port - dev->io_base, val, 1);
}

static void
rage128_io_writew(uint16_t port, uint16_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    rage128_mmio_write_access(dev, port - dev->io_base, val, 2);
}

static void
rage128_io_writel(uint16_t port, uint32_t val, void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    rage128_mmio_write_access(dev, port - dev->io_base, val, 4);
}

static void
rage128_map_io(rage128_t *dev)
{
    rage128_unmap_io(dev);

    if (dev->power_state != R128_PCI_PM_STATE_D0 ||
        !(dev->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_IO))
        return;

    if (!(R128_REG(dev, R128_CNFG_CNTL) & R128_CFG_VGA_IO_DIS)) {
        io_sethandler(0x03c0, 0x0020, rage128_vga_in, NULL, NULL,
                      rage128_vga_out, NULL, NULL, dev);
        dev->vga_io_mapped = 1;
    }

    if (dev->io_base && dev->io_base <= 0xff00U && dev->io_base != 0xff00U) {
        io_sethandler((uint16_t) dev->io_base, R128_IO_SIZE,
                      rage128_io_readb, rage128_io_readw, rage128_io_readl,
                      rage128_io_writeb, rage128_io_writew,
                      rage128_io_writel, dev);
        dev->mapped_io_base = (uint16_t) dev->io_base;
        dev->io_bar_mapped  = 1;
    }
}

static void
rage128_recalc_mapping(rage128_t *dev)
{
    bool memory_enabled =
        dev->power_state == R128_PCI_PM_STATE_D0 &&
        !!(dev->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_MEM);

    mem_mapping_disable(&dev->linear_mapping);
    mem_mapping_disable(&dev->mmio_mapping);
    if (dev->has_bios)
        mem_mapping_disable(&dev->bios_rom.mapping);

    if (memory_enabled) {
        mem_mapping_enable(&dev->svga.mapping);

        if (dev->linear_base && dev->linear_base != R128_PCI_BAR0_MASK)
            mem_mapping_set_addr(&dev->linear_mapping, dev->linear_base,
                                 R128_LINEAR_APERTURE_SIZE);

        if (dev->mmio_base && dev->mmio_base != R128_PCI_BAR2_MASK)
            mem_mapping_set_addr(&dev->mmio_mapping, dev->mmio_base,
                                 R128_MMIO_SIZE);

        if (dev->has_bios && dev->rom_enabled && dev->rom_base &&
            dev->rom_base != dev->rom_mask)
            mem_mapping_set_addr(&dev->bios_rom.mapping, dev->rom_base,
                                 dev->rom_size);
    } else {
        mem_mapping_disable(&dev->svga.mapping);
    }

    rage128_map_io(dev);
}

static uint32_t
rage128_pci_bar_value(const rage128_t *dev, unsigned int bar)
{
    switch (bar) {
        case 0:
            return dev->linear_base | R128_PCI_BAR0_FLAGS;
        case 1:
            return dev->io_base | R128_PCI_BAR1_FLAGS;
        case 2:
            return dev->mmio_base;
        default:
            return 0;
    }
}

static uint8_t
rage128_pci_read(int func, int addr, UNUSED(int len), void *priv)
{
    const rage128_t *dev = (const rage128_t *) priv;
    uint32_t         value;

    if (func != 0)
        return 0xff;

    switch (addr) {
        case PCI_REG_VENDOR_ID_L:
            return R128_PCI_VENDOR_ID & 0xff;
        case PCI_REG_VENDOR_ID_H:
            return R128_PCI_VENDOR_ID >> 8;
        case PCI_REG_DEVICE_ID_L:
            return dev->device_id & 0xff;
        case PCI_REG_DEVICE_ID_H:
            return dev->device_id >> 8;
        case PCI_REG_COMMAND_L:
            return dev->pci_regs[PCI_REG_COMMAND_L];
        case PCI_REG_COMMAND_H:
            return dev->pci_regs[PCI_REG_COMMAND_H];
        case PCI_REG_STATUS_L:
        {
            uint8_t status = PCI_STATUS_L_CAPAB |
                             (dev->irq_state ? PCI_STATUS_L_INT : 0);

            if (dev->is_agp)
                status |= PCI_STATUS_L_66MHZ | PCI_STATUS_L_FAST_B2B;
            return status;
        }
        case PCI_REG_STATUS_H:
            return PCI_DEVSEL_MEDIUM;
        case PCI_REG_REVISION:
            return 0;
        case PCI_REG_PROG_IF:
            return 0;
        case PCI_REG_SUBCLASS:
            return 0x00;
        case PCI_REG_CLASS:
            return 0x03;
        case PCI_REG_HEADER_TYPE:
            return 0x00;

        case PCI_REG_BAR0_BYTE0 ... PCI_REG_BAR0_BYTE3:
            value = rage128_pci_bar_value(dev, 0);
            return value >> ((addr - PCI_REG_BAR0_BYTE0) * 8);
        case PCI_REG_BAR1_BYTE0 ... PCI_REG_BAR1_BYTE3:
            value = rage128_pci_bar_value(dev, 1);
            return value >> ((addr - PCI_REG_BAR1_BYTE0) * 8);
        case PCI_REG_BAR2_BYTE0 ... PCI_REG_BAR2_BYTE3:
            value = rage128_pci_bar_value(dev, 2);
            return value >> ((addr - PCI_REG_BAR2_BYTE0) * 8);

        case PCI_REG_SUBVEN_ID_L:
            return R128_PCI_VENDOR_ID & 0xff;
        case PCI_REG_SUBVEN_ID_H:
            return R128_PCI_VENDOR_ID >> 8;
        case PCI_REG_SUBSYS_ID_L:
            return dev->subsystem_id & 0xff;
        case PCI_REG_SUBSYS_ID_H:
            return dev->subsystem_id >> 8;
        case PCI_REG_CAPS_PTR:
            return dev->pci_regs[PCI_REG_CAPS_PTR];

        case PCI_REG_ROM_BAR_BYTE0 ... PCI_REG_ROM_BAR_BYTE3:
            value = dev->rom_base | (dev->rom_enabled ? 1U : 0U);
            return value >> ((addr - PCI_REG_ROM_BAR_BYTE0) * 8);

        case PCI_REG_INT_LINE:
            return dev->int_line;
        case PCI_REG_INT_PIN:
            return PCI_INTA;
        case PCI_REG_MIN_GRANT:
            return 8;
        case PCI_REG_MAX_LAT:
            return 0;
        default:
            return dev->pci_regs[addr & 0xff];
    }
}

static uint32_t
rage128_pci_bytes(const rage128_t *dev, unsigned int first)
{
    return (uint32_t) dev->pci_regs[first] |
           ((uint32_t) dev->pci_regs[first + 1] << 8) |
           ((uint32_t) dev->pci_regs[first + 2] << 16) |
           ((uint32_t) dev->pci_regs[first + 3] << 24);
}

static void
rage128_pci_store16(rage128_t *dev, unsigned int first, uint16_t value)
{
    dev->pci_regs[first]     = value;
    dev->pci_regs[first + 1] = value >> 8;
}

static void
rage128_pci_store32(rage128_t *dev, unsigned int first, uint32_t value)
{
    dev->pci_regs[first]     = value;
    dev->pci_regs[first + 1] = value >> 8;
    dev->pci_regs[first + 2] = value >> 16;
    dev->pci_regs[first + 3] = value >> 24;
}

static void
rage128_pci_reset_profile(rage128_t *dev)
{
    uint8_t pm_cap = rage128_pm_cap_offset(dev);

    memset(&dev->pci_regs[R128_PCI_AGP_CAP_OFFSET], 0,
           R128_PCI_PM_CAP_OFFSET_PF + 8 -
           R128_PCI_AGP_CAP_OFFSET);
    dev->pci_regs[PCI_REG_CACHELINE_SIZE] = 0;
    dev->pci_regs[PCI_REG_LATENCY_TIMER]  = 0;

    if (dev->is_agp) {
        dev->pci_regs[PCI_REG_CAPS_PTR] = R128_PCI_AGP_CAP_OFFSET;
        dev->pci_regs[R128_PCI_AGP_CAP_OFFSET] = R128_PCI_CAP_ID_AGP;
        dev->pci_regs[R128_PCI_AGP_CAP_OFFSET + 1] = pm_cap;
        dev->pci_regs[R128_PCI_AGP_CAP_OFFSET + 2] = 0x20;
        rage128_pci_store32(dev, R128_PCI_AGP_CAP_OFFSET + 4,
                            R128_PCI_AGP_STATUS);
        rage128_pci_store32(dev, R128_PCI_AGP_CAP_OFFSET + 8,
                            R128_PCI_AGP_COMMAND_RESET);
    } else {
        dev->pci_regs[PCI_REG_CAPS_PTR] = pm_cap;
    }

    dev->pci_regs[pm_cap]     = R128_PCI_CAP_ID_PM;
    dev->pci_regs[pm_cap + 1] = 0;
    rage128_pci_store16(dev, pm_cap + 2,
                        R128_PCI_PM_CAPABILITIES);
    rage128_pci_store16(dev, pm_cap + 4, 0);
    dev->pci_regs[pm_cap + 6] = 0;
    dev->pci_regs[pm_cap + 7] = 0;
    dev->power_state = R128_PCI_PM_STATE_D0;
}

static void
rage128_pci_set_power_state(rage128_t *dev, uint8_t requested)
{
    uint8_t old_state = dev->power_state;
    uint8_t new_state = requested & R128_PCI_PM_STATE_MASK;

    if (new_state == R128_PCI_PM_STATE_D2 ||
        (old_state != R128_PCI_PM_STATE_D0 &&
         new_state != R128_PCI_PM_STATE_D0 && new_state < old_state))
        new_state = old_state;

    dev->power_state = new_state;
    rage128_pci_store16(dev, rage128_pm_cap_offset(dev) + 4,
                        new_state);
    if (new_state != old_state) {
        if (new_state != R128_PCI_PM_STATE_D0)
            pci_clear_irq(dev->pci_slot, PCI_INTA, &dev->irq_state);
        rage128_recalc_mapping(dev);
        rage128_update_irq(dev);
    }
}

static void
rage128_pci_write(int func, int addr, UNUSED(int len), uint8_t val,
                  void *priv)
{
    rage128_t *dev = (rage128_t *) priv;
    uint32_t   raw;

    if (func != 0)
        return;

    {
        uint8_t pm_cap = rage128_pm_cap_offset(dev);

        if (addr == PCI_REG_CAPS_PTR)
            return;

        if (addr >= pm_cap && addr < pm_cap + 8) {
            if (addr < pm_cap + 4 || addr >= pm_cap + 6)
                return;
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev, pm_cap + 4);
            rage128_pci_set_power_state(dev, raw);
            return;
        }

        if (dev->is_agp && addr >= R128_PCI_AGP_CAP_OFFSET &&
            addr < R128_PCI_AGP_CAP_OFFSET + 12) {
            if (addr < R128_PCI_AGP_CAP_OFFSET + 8)
                return;
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev,
                                    R128_PCI_AGP_CAP_OFFSET + 8);
            rage128_pci_store32(dev,
                                R128_PCI_AGP_CAP_OFFSET + 8,
                                raw & R128_PCI_AGP_COMMAND_MASK);
            return;
        }
    }

    switch (addr) {
        case PCI_REG_COMMAND_L:
            dev->pci_regs[PCI_REG_COMMAND_L] = val &
                (PCI_COMMAND_L_IO | PCI_COMMAND_L_MEM | PCI_COMMAND_L_BM |
                 PCI_COMMAND_L_VGASNOOP | PCI_COMMAND_L_PARITY);
            rage128_recalc_mapping(dev);
            return;
        case PCI_REG_COMMAND_H:
            dev->pci_regs[PCI_REG_COMMAND_H] = val &
                (PCI_COMMAND_H_SERR | PCI_COMMAND_H_FAST_B2B |
                 PCI_COMMAND_H_INT_DIS);
            rage128_update_irq(dev);
            return;

        case PCI_REG_CACHELINE_SIZE:
        case PCI_REG_LATENCY_TIMER:
            dev->pci_regs[addr] = val;
            return;


        case PCI_REG_BAR0_BYTE0 ... PCI_REG_BAR0_BYTE3:
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev, PCI_REG_BAR0_BYTE0);
            dev->linear_base = raw & R128_PCI_BAR0_MASK;
            rage128_recalc_mapping(dev);
            return;

        case PCI_REG_BAR1_BYTE0 ... PCI_REG_BAR1_BYTE3:
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev, PCI_REG_BAR1_BYTE0);
            dev->io_base = raw & R128_PCI_BAR1_MASK;
            rage128_recalc_mapping(dev);
            return;

        case PCI_REG_BAR2_BYTE0 ... PCI_REG_BAR2_BYTE3:
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev, PCI_REG_BAR2_BYTE0);
            dev->mmio_base = raw & R128_PCI_BAR2_MASK;
            rage128_recalc_mapping(dev);
            return;

        case PCI_REG_ROM_BAR_BYTE0 ... PCI_REG_ROM_BAR_BYTE3:
            dev->pci_regs[addr] = val;
            raw = rage128_pci_bytes(dev, PCI_REG_ROM_BAR_BYTE0);
            dev->rom_enabled = !!(raw & 1);
            dev->rom_base    = raw & dev->rom_mask;
            rage128_recalc_mapping(dev);
            return;

        case PCI_REG_INT_LINE:
            dev->int_line = val;
            return;

        default:
            dev->pci_regs[addr & 0xff] = val;
            return;
    }
}

static uint32_t
rage128_mmio_register_read(rage128_t *dev, uint32_t reg)
{
    if (reg >= 0x0f00U && reg < 0x1000U) {
        uint32_t pci_addr = reg - 0x0f00U;
        return (uint32_t) rage128_pci_read(0, pci_addr, 1, dev) |
               ((uint32_t) rage128_pci_read(0, pci_addr + 1, 1, dev) << 8) |
               ((uint32_t) rage128_pci_read(0, pci_addr + 2, 1, dev) << 16) |
               ((uint32_t) rage128_pci_read(0, pci_addr + 3, 1, dev) << 24);
    }

    switch (reg) {
        case R128_GPIO_MONID:
            rage128_gpio_monid_update(dev);
            return R128_REG(dev, reg);
        case R128_CLOCK_CNTL_DATA:
            return dev->pll_regs[dev->pll_index & 0x3fU];
        case R128_CNFG_MEMSIZE:
            return dev->vram_size;
        case R128_CONFIG_APER_0_BASE:
        case R128_CONFIG_APER_1_BASE:
            return dev->linear_base;
        case R128_CONFIG_APER_SIZE:
            return R128_LINEAR_APERTURE_SIZE / 2;
        case R128_CONFIG_REG_1_BASE:
            return dev->mmio_base;
        case R128_CONFIG_REG_APER_SIZE:
            return R128_MMIO_SIZE / 2;
        case R128_HOST_PATH_CNTL:
            return 1U << 23;
        case R128_PC_NGUI_CTLSTAT:
            return R128_REG(dev, reg) & ~R128_PC_BUSY;
        case R128_MC_STATUS:
            return 5;
        case R128_RBBM_STATUS:
        case R128_GUI_STAT:
            return rage128_pm4_gui_status(dev);
        case R128_PC_GUI_CTLSTAT:
            return 0;
        default:
            return R128_REG(dev, reg);
    }
}

static uint32_t
rage128_merge_access(uint32_t old_value, uint32_t addr, uint32_t value,
                     unsigned int size)
{
    unsigned int shift = (addr & 3U) * 8U;
    uint32_t mask;

    if (size == 4)
        return value;

    mask = ((UINT32_C(1) << (size * 8U)) - 1U) << shift;
    return (old_value & ~mask) | ((value << shift) & mask);
}

static uint32_t
rage128_extract_access(uint32_t value, uint32_t addr, unsigned int size)
{
    unsigned int shift = (addr & 3U) * 8U;

    value >>= shift;
    if (size == 1)
        return value & 0xffU;
    if (size == 2)
        return value & 0xffffU;
    return value;
}

static void
rage128_palette_update(rage128_t *dev)
{
    uint32_t value = R128_REG(dev, R128_PALETTE_DATA);

    svga_out(0x03c9, (value >> 16) & 0xff, &dev->svga);
    svga_out(0x03c9, (value >> 8) & 0xff, &dev->svga);
    svga_out(0x03c9, value & 0xff, &dev->svga);
}

static void
rage128_register_written(rage128_t *dev, uint32_t reg)
{
    switch (reg) {
        case R128_MM_INDEX:
            R128_REG(dev, reg) &= ~3U;
            break;
        case R128_CLOCK_CNTL_INDEX:
            dev->pll_index = R128_REG(dev, reg) & 0x3fU;
            break;
        case R128_GPIO_MONID:
            R128_REG(dev, reg) &= R128_GPIO_MONID_A_MASK |
                                  R128_GPIO_MONID_EN_MASK |
                                  R128_GPIO_MONID_MASK_MASK;
            rage128_gpio_monid_update(dev);
            break;

        case R128_GEN_INT_CNTL:
            rage128_update_irq(dev);
            break;

        case R128_CRTC_H_TOTAL_DISP:
            R128_REG(dev, reg) &= 0x07ff07ffU;
            svga_recalctimings(&dev->svga);
            break;
        case R128_CRTC_H_SYNC_STRT_WID:
            R128_REG(dev, reg) &= 0x17bf1fffU;
            svga_recalctimings(&dev->svga);
            break;
        case R128_CRTC_V_TOTAL_DISP:
            R128_REG(dev, reg) &= 0x0fff0fffU;
            svga_recalctimings(&dev->svga);
            break;
        case R128_CRTC_V_SYNC_STRT_WID:
            R128_REG(dev, reg) &= 0x009f0fffU;
            svga_recalctimings(&dev->svga);
            break;
        case R128_CRTC_OFFSET:
            R128_REG(dev, reg) &= 0x87fffff8U;
            svga_recalctimings(&dev->svga);
            break;
        case R128_CRTC_OFFSET_CNTL:
        case R128_CRTC_PITCH:
        case R128_CRTC_GEN_CNTL:
        case R128_CRTC_EXT_CNTL:
            if (reg == R128_CRTC_PITCH)
                R128_REG(dev, reg) &= 0x07ff07ffU;
            rage128_update_cursor(dev);
            svga_recalctimings(&dev->svga);
            break;

        case R128_CUR_OFFSET:
            R128_REG(dev, reg) &= 0x87fffff0U;
            rage128_update_cursor(dev);
            break;
        case R128_CUR_HORZ_VERT_POSN:
            R128_REG(dev, reg) &= 0x3fff0fffU;
            rage128_update_cursor(dev);
            break;
        case R128_CUR_HORZ_VERT_OFF:
            R128_REG(dev, reg) &= 0x003f003fU;
            rage128_update_cursor(dev);
            break;
        case R128_CUR_CLR0:
        case R128_CUR_CLR1:
            R128_REG(dev, reg) &= 0x00ffffffU;
            break;

        case R128_DAC_CNTL:
            R128_REG(dev, reg) &= 0xffffe3ffU;
            svga_set_ramdac_type(&dev->svga,
                                 (R128_REG(dev, reg) & R128_DAC_8BIT_EN) ?
                                     RAMDAC_8BIT : RAMDAC_6BIT);
            break;
        case R128_PALETTE_INDEX:
            svga_out(0x03c7, (R128_REG(dev, reg) >> 16) & 0xff,
                     &dev->svga);
            svga_out(0x03c8, R128_REG(dev, reg) & 0xff, &dev->svga);
            break;
        case R128_PALETTE_DATA:
            rage128_palette_update(dev);
            break;

        case R128_CNFG_CNTL:
            rage128_map_io(dev);
            break;
        case R128_PC_NGUI_CTLSTAT:
            /* Synchronous 2D completes cache flushes immediately. */
            R128_REG(dev, reg) &= R128_PC_FLUSH_ALL;
            break;
        case R128_GEN_RESET_CNTL:
            if (R128_REG(dev, reg) & R128_SOFT_RESET_GUI)
                rage128_pm4_reset(dev, false);
            break;

        case R128_DST_OFFSET:
        case R128_SRC_OFFSET:
        case R128_DEFAULT_OFFSET:
            R128_REG(dev, reg) &= 0xfffffff0U;
            rage128_accel_reg_written(dev, reg);
            break;
        case R128_DST_PITCH:
        case R128_SRC_PITCH:
        case R128_DEFAULT_PITCH:
            R128_REG(dev, reg) &= 0x00013fffU;
            rage128_accel_reg_written(dev, reg);
            break;
        case R128_DP_DATATYPE:
            R128_REG(dev, reg) &= 0xe0070f0fU;
            rage128_accel_reg_written(dev, reg);
            break;
        case R128_DP_MIX:
            R128_REG(dev, reg) &= 0x00ff0700U;
            rage128_accel_reg_written(dev, reg);
            break;
        case R128_DP_GUI_MASTER_CNTL:
            rage128_accel_reg_written(dev, reg);
            R128_REG(dev, reg) &= 0xf800000fU;
            break;
        case R128_DST_WIDTH:
        case R128_DST_HEIGHT:
        case R128_SRC_X:
        case R128_SRC_Y:
        case R128_DST_X:
        case R128_DST_Y:
            R128_REG(dev, reg) &= 0x00003fffU;
            rage128_accel_reg_written(dev, reg);
            break;

        default:
            if (reg >= R128_DST_OFFSET && reg <= R128_HOST_DATA_LAST)
                rage128_accel_reg_written(dev, reg);
            break;
    }
}

static uint32_t
rage128_mmio_read_access(rage128_t *dev, uint32_t addr, unsigned int size)
{
    uint32_t reg;
    uint32_t value;

    addr &= R128_MMIO_SIZE - 1;

    if ((addr & 3U) + size > 4U) {
        value = 0;
        for (unsigned int i = 0; i < size; i++)
            value |= rage128_mmio_read_access(dev, addr + i, 1) << (i * 8);
        return value;
    }

    if (rage128_pm4_mm_read(dev, addr, size, &value))
        return value;

    if (addr >= R128_MM_DATA && addr < R128_MM_DATA + 4U) {
        uint32_t indexed = R128_REG(dev, R128_MM_INDEX);
        uint32_t lane    = addr - R128_MM_DATA;

        if (indexed & 0x80000000U)
            return rage128_vram_read(dev,
                                     (indexed & 0x7fffffffU) + lane, size);
        if ((indexed & ~3U) > R128_MM_DATA + 3U)
            return rage128_mmio_read_access(dev, indexed + lane, size);
        return 0;
    }

    reg   = addr & ~3U;
    value = rage128_mmio_register_read(dev, reg);
    return rage128_extract_access(value, addr, size);
}

static bool
rage128_mmio_register_is_read_only(uint32_t reg)
{
    if (reg >= 0x0f00U && reg < 0x1000U)
        return true;

    switch (reg) {
        case R128_CNFG_MEMSIZE:
        case R128_CONFIG_APER_0_BASE:
        case R128_CONFIG_APER_1_BASE:
        case R128_CONFIG_APER_SIZE:
        case R128_CONFIG_REG_1_BASE:
        case R128_CONFIG_REG_APER_SIZE:
        case R128_HOST_PATH_CNTL:
        case R128_MC_STATUS:
        case R128_RBBM_STATUS:
        case R128_GUI_STAT:
        case R128_PC_GUI_CTLSTAT:
            return true;
        default:
            return false;
    }
}

static void
rage128_mmio_write_access(rage128_t *dev, uint32_t addr, uint32_t value,
                          unsigned int size)
{
    uint32_t reg;
    uint32_t old_value;
    uint32_t merged;
    uint32_t written_bits;

    addr &= R128_MMIO_SIZE - 1;

    if ((addr & 3U) + size > 4U) {
        for (unsigned int i = 0; i < size; i++)
            rage128_mmio_write_access(dev, addr + i, value >> (i * 8), 1);
        return;
    }

    if (rage128_pm4_mm_write(dev, addr, value, size))
        return;

    if (addr >= R128_MM_DATA && addr < R128_MM_DATA + 4U) {
        uint32_t indexed = R128_REG(dev, R128_MM_INDEX);
        uint32_t lane    = addr - R128_MM_DATA;

        if (indexed & 0x80000000U) {
            rage128_vram_write(dev, (indexed & 0x7fffffffU) + lane,
                               value, size);
            return;
        }
        if ((indexed & ~3U) > R128_MM_DATA + 3U) {
            rage128_mmio_write_access(dev, indexed + lane, value, size);
            return;
        }
        return;
    }

    reg = addr & ~3U;

    if (size == 4)
        written_bits = value;
    else {
        unsigned int shift = (addr & 3U) * 8U;
        uint32_t mask = (size == 1) ? 0xffU : 0xffffU;
        written_bits = (value & mask) << shift;
    }

    if (reg == R128_GEN_INT_STATUS) {
        R128_REG(dev, reg) &= ~written_bits;
        rage128_update_irq(dev);
        return;
    }

    if (reg == R128_CLOCK_CNTL_DATA) {
        old_value = dev->pll_regs[dev->pll_index & 0x3fU];
        merged = rage128_merge_access(old_value, addr, value, size);
        if (R128_REG(dev, R128_CLOCK_CNTL_INDEX) & R128_PLL_WR_EN) {
            dev->pll_regs[dev->pll_index & 0x3fU] = merged;
            svga_recalctimings(&dev->svga);
        }
        return;
    }

    if (rage128_mmio_register_is_read_only(reg))
        return;

    old_value = rage128_mmio_register_read(dev, reg);
    merged    = rage128_merge_access(old_value, addr, value, size);
    R128_REG(dev, reg) = merged;
    rage128_register_written(dev, reg);
}

void
rage128_mmio_write_reg(rage128_t *dev, uint32_t addr, uint32_t data)
{
    rage128_mmio_write_access(dev, addr, data, 4);
}

static uint8_t
rage128_mmio_readb(uint32_t addr, void *priv)
{
    return rage128_mmio_read_access((rage128_t *) priv, addr, 1);
}

static uint16_t
rage128_mmio_readw(uint32_t addr, void *priv)
{
    return rage128_mmio_read_access((rage128_t *) priv, addr, 2);
}

static uint32_t
rage128_mmio_readl(uint32_t addr, void *priv)
{
    return rage128_mmio_read_access((rage128_t *) priv, addr, 4);
}

static void
rage128_mmio_writeb(uint32_t addr, uint8_t val, void *priv)
{
    rage128_mmio_write_access((rage128_t *) priv, addr, val, 1);
}

static void
rage128_mmio_writew(uint32_t addr, uint16_t val, void *priv)
{
    rage128_mmio_write_access((rage128_t *) priv, addr, val, 2);
}

static void
rage128_mmio_writel(uint32_t addr, uint32_t val, void *priv)
{
    rage128_mmio_write_access((rage128_t *) priv, addr, val, 4);
}

static void
rage128_reset(void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    memset(dev->regs, 0, sizeof(dev->regs));
    memset(dev->pll_regs, 0, sizeof(dev->pll_regs));
    rage128_pci_reset_profile(dev);

    dev->pll_index = 0;
    dev->pll_regs[R128_PPLL_REF_DIV] = 12;
    dev->pll_regs[R128_PPLL_DIV_3]   = 56 | (4U << 16);

    R128_REG(dev, R128_CNFG_MEMSIZE) = dev->vram_size;
    R128_REG(dev, R128_DAC_CNTL)     = R128_DAC_8BIT_EN;
    R128_REG(dev, R128_CRTC_EXT_CNTL) = 0;
    if (dev->i2c)
        i2c_gpio_set(dev->i2c, 1, 1);
    rage128_gpio_monid_update(dev);

    rage128_pm4_reset(dev, true);
    rage128_update_cursor(dev);
    rage128_update_irq(dev);
    rage128_recalc_mapping(dev);
    svga_recalctimings(&dev->svga);
}

static void *
rage128_init(const device_t *info)
{
    rage128_t *dev = calloc(1, sizeof(rage128_t));
    const char *bios_fn;

    if (!dev)
        return NULL;

    dev->device_id = info->local & 0xffffU;
    dev->subsystem_id = (info->local >> 16) & 0xffffU;
    dev->is_agp = !!(info->flags & DEVICE_AGP);
    dev->rom_size = dev->is_agp ? R128_ROM_SIZE_AGP :
                                  R128_ROM_SIZE_PCI;
    dev->rom_mask = ~(dev->rom_size - 1U);
    dev->vram_size = (uint32_t) device_get_config_int("memory") << 20;
    dev->vram_mask = dev->vram_size - 1;

    bios_fn = device_get_config_string("bios");
    if (!bios_fn || !bios_fn[0])
        bios_fn = rage128_default_bios(dev);
    if (bios_fn && bios_fn[0] && rom_present(bios_fn)) {
        dev->has_bios = 1;
        rom_init(&dev->bios_rom, bios_fn, 0x000c0000, dev->rom_size,
                 dev->rom_size - 1, 0, MEM_MAPPING_EXTERNAL);
        mem_mapping_disable(&dev->bios_rom.mapping);
    }

    dev->i2c = i2c_gpio_init("ddc_ati_rage128");
    dev->ddc = ddc_init(i2c_gpio_get_bus(dev->i2c));

    video_inform(VIDEO_FLAG_TYPE_SPECIAL,
                 dev->is_agp ? &timing_rage128_agp :
                               &timing_rage128_pci);

    svga_init(info, &dev->svga, dev, dev->vram_size,
              rage128_recalctimings,
              rage128_vga_in, rage128_vga_out,
              rage128_hwcursor_draw, NULL);

    dev->svga.miscout          = 1;
    dev->svga.packed_chain4    = 1;
    dev->svga.decode_mask      = dev->vram_mask;
    dev->svga.vram_display_mask = dev->vram_mask;
    dev->svga.vblank_start     = rage128_vblank_start;
    dev->svga.hwcursor.cur_xsize = 64;
    dev->svga.hwcursor.cur_ysize = 64;
    svga_set_ramdac_type(&dev->svga, RAMDAC_8BIT);

    mem_mapping_add(&dev->linear_mapping, 0, 0,
                    rage128_linear_readb, rage128_linear_readw,
                    rage128_linear_readl, rage128_linear_writeb,
                    rage128_linear_writew, rage128_linear_writel,
                    NULL, MEM_MAPPING_EXTERNAL, dev);
    mem_mapping_add(&dev->mmio_mapping, 0, 0,
                    rage128_mmio_readb, rage128_mmio_readw,
                    rage128_mmio_readl, rage128_mmio_writeb,
                    rage128_mmio_writew, rage128_mmio_writel,
                    NULL, MEM_MAPPING_EXTERNAL, dev);
    mem_mapping_disable(&dev->linear_mapping);
    mem_mapping_disable(&dev->mmio_mapping);

    pci_add_card((info->flags & DEVICE_AGP) ? PCI_ADD_AGP : PCI_ADD_NORMAL,
                 rage128_pci_read, rage128_pci_write, dev, &dev->pci_slot);

    dev->pci_regs[PCI_REG_COMMAND_L] = PCI_COMMAND_IO | PCI_COMMAND_MEM;
    dev->pci_regs[PCI_REG_COMMAND_H] = 0;
    dev->int_line = 0xff;

    rage128_reset(dev);
    return dev;
}

static void
rage128_close(void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    rage128_unmap_io(dev);
    pci_clear_irq(dev->pci_slot, PCI_INTA, &dev->irq_state);
    svga_close(&dev->svga);
    if (dev->ddc)
        ddc_close(dev->ddc);
    if (dev->i2c)
        i2c_gpio_close(dev->i2c);
    free(dev);
}

static void
rage128_speed_changed(void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    svga_recalctimings(&dev->svga);
}

static void
rage128_force_redraw(void *priv)
{
    rage128_t *dev = (rage128_t *) priv;

    dev->svga.fullchange = dev->svga.monitor->mon_changeframecount;
}

// clang-format off
static const device_config_t rage128_config[] = {
    {
        .name           = "memory",
        .description    = "Memory size",
        .type           = CONFIG_SELECTION,
        .default_string = NULL,
        .default_int    = 32,
        .file_filter    = NULL,
        .spinner        = { 0 },
        .selection      = {
            { .description =  "8 MB", .value =  8 },
            { .description = "16 MB", .value = 16 },
            { .description = "32 MB", .value = 32 },
            { .description = "64 MB", .value = 64 },
            { .description = ""                   }
        },
        .bios           = { { 0 } }
    },
    {
        .name           = "bios",
        .description    = "Video BIOS",
        .type           = CONFIG_FNAME,
        .default_string = "",
        .default_int    = 0,
        .file_filter    = "ROM images (*.bin *.rom);;All files (*.*)",
        .spinner        = { 0 },
        .selection      = { { 0 } },
        .bios           = { { 0 } }
    },
    { .name = "", .description = "", .type = CONFIG_END }
};
// clang-format on

const device_t ati_rage128_pro_pf_device = {
    .name          = "ATI Rage Fury Pro / Xpert 2000 Pro (PF, AGP 4x)",
    .internal_name = "ati_rage128_pro_pf_agp",
    .flags         = DEVICE_AGP,
    .local         = (R128_PCI_SUBSYSTEM_PF << 16) |
                     R128_PCI_DEVICE_ID_PF,
    .init          = rage128_init,
    .close         = rage128_close,
    .reset         = rage128_reset,
    .available     = NULL,
    .speed_changed = rage128_speed_changed,
    .force_redraw  = rage128_force_redraw,
    .config        = rage128_config
};

const device_t ati_xpert128_re_pci_device = {
    .name          = "ATI Xpert 128 (RE, PCI)",
    .internal_name = "ati_xpert128_re_pci",
    .flags         = DEVICE_PCI,
    .local         = (R128_PCI_SUBSYSTEM_XPERT << 16) |
                     R128_PCI_DEVICE_ID_RE,
    .init          = rage128_init,
    .close         = rage128_close,
    .reset         = rage128_reset,
    .available     = NULL,
    .speed_changed = rage128_speed_changed,
    .force_redraw  = rage128_force_redraw,
    .config        = rage128_config
};

/* The multimedia/capture functions are intentionally not exposed yet. */
const device_t ati_all_in_wonder128_re_pci_device = {
    .name          = "ATI All-in-Wonder 128 (RE, PCI; display function)",
    .internal_name = "ati_all_in_wonder128_re_pci",
    .flags         = DEVICE_PCI,
    .local         = (R128_PCI_SUBSYSTEM_AIW << 16) |
                     R128_PCI_DEVICE_ID_RE,
    .init          = rage128_init,
    .close         = rage128_close,
    .reset         = rage128_reset,
    .available     = NULL,
    .speed_changed = rage128_speed_changed,
    .force_redraw  = rage128_force_redraw,
    .config        = rage128_config
};
