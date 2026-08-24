/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          ATI Rage 128 internal definitions.
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
#ifndef VID_ATI_RAGE128_H
#define VID_ATI_RAGE128_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <86box/device.h>
#include <86box/mem.h>
#include <86box/rom.h>
#include <86box/timer.h>
#include <86box/video.h>
#include <86box/vid_svga.h>

#define R128_MMIO_SIZE               0x00004000U
#define R128_LINEAR_APERTURE_SIZE    0x04000000U
#define R128_IO_SIZE                 0x00000100U
#define R128_ROM_SIZE_PCI            0x00010000U
#define R128_ROM_SIZE_AGP            0x00020000U

#define R128_PCI_VENDOR_ID           0x1002U
#define R128_PCI_DEVICE_ID_PF        0x5046U
#define R128_PCI_DEVICE_ID_RE        0x5245U
#define R128_PCI_SUBSYSTEM_PF        0x0018U
#define R128_PCI_SUBSYSTEM_XPERT     0x0008U
#define R128_PCI_SUBSYSTEM_AIW       0x0068U
#define R128_PCI_AGP_CAP_OFFSET      0x50U
#define R128_PCI_PM_CAP_OFFSET_PF    0x5cU
#define R128_PCI_PM_CAP_OFFSET_RE    0x50U
#define R128_PCI_CAP_ID_PM           0x01U
#define R128_PCI_CAP_ID_AGP          0x02U
#define R128_PCI_AGP_STATUS          0x1f000207U
#define R128_PCI_AGP_COMMAND_RESET   0x00000200U
#define R128_PCI_AGP_COMMAND_MASK    0x1f000307U
#define R128_PCI_PM_CAPABILITIES     0x0202U
#define R128_PCI_PM_STATE_MASK       0x03U
#define R128_PCI_PM_STATE_D0         0x00U
#define R128_PCI_PM_STATE_D1         0x01U
#define R128_PCI_PM_STATE_D2         0x02U
#define R128_PCI_PM_STATE_D3HOT      0x03U

/* Miscellaneous and display registers. */
#define R128_MM_INDEX                0x0000U
#define R128_MM_DATA                 0x0004U
#define R128_CLOCK_CNTL_INDEX        0x0008U
#define R128_CLOCK_CNTL_DATA         0x000cU
#define R128_BIOS_0_SCRATCH          0x0010U
#define R128_GEN_INT_CNTL            0x0040U
#define R128_GEN_INT_STATUS          0x0044U
#define R128_CRTC_GEN_CNTL           0x0050U
#define R128_CRTC_EXT_CNTL           0x0054U
#define R128_DAC_CNTL                0x0058U
#define R128_GPIO_MONID              0x0068U
#define R128_GPIO_MONID_A_0          (1U << 0)
#define R128_GPIO_MONID_A_1          (1U << 1)
#define R128_GPIO_MONID_A_2          (1U << 2)
#define R128_GPIO_MONID_A_3          (1U << 3)
#define R128_GPIO_MONID_Y_0          (1U << 8)
#define R128_GPIO_MONID_Y_1          (1U << 9)
#define R128_GPIO_MONID_Y_2          (1U << 10)
#define R128_GPIO_MONID_Y_3          (1U << 11)
#define R128_GPIO_MONID_EN_0         (1U << 16)
#define R128_GPIO_MONID_EN_1         (1U << 17)
#define R128_GPIO_MONID_EN_2         (1U << 18)
#define R128_GPIO_MONID_EN_3         (1U << 19)
#define R128_GPIO_MONID_MASK_0       (1U << 24)
#define R128_GPIO_MONID_MASK_1       (1U << 25)
#define R128_GPIO_MONID_MASK_2       (1U << 26)
#define R128_GPIO_MONID_MASK_3       (1U << 27)
#define R128_GPIO_MONID_A_MASK       0x0000000fU
#define R128_GPIO_MONID_Y_MASK       0x00000f00U
#define R128_GPIO_MONID_EN_MASK      0x000f0000U
#define R128_GPIO_MONID_MASK_MASK    0x0f000000U
#define R128_PALETTE_INDEX           0x00b0U
#define R128_PALETTE_DATA            0x00b4U
#define R128_CNFG_CNTL               0x00e0U
#define R128_GEN_RESET_CNTL          0x00f0U
#define R128_CNFG_MEMSIZE            0x00f8U
#define R128_CONFIG_APER_0_BASE      0x0100U
#define R128_CONFIG_APER_1_BASE      0x0104U
#define R128_CONFIG_APER_SIZE        0x0108U
#define R128_CONFIG_REG_1_BASE       0x010cU
#define R128_CONFIG_REG_APER_SIZE    0x0110U
#define R128_HOST_PATH_CNTL          0x0130U
#define R128_MC_STATUS               0x0150U
#define R128_MEM_SDRAM_MODE_REG      0x0158U
#define R128_PC_NGUI_CTLSTAT         0x0184U

#define R128_CRTC_H_TOTAL_DISP       0x0200U
#define R128_CRTC_H_SYNC_STRT_WID    0x0204U
#define R128_CRTC_V_TOTAL_DISP       0x0208U
#define R128_CRTC_V_SYNC_STRT_WID    0x020cU
#define R128_CRTC_OFFSET             0x0224U
#define R128_CRTC_OFFSET_CNTL        0x0228U
#define R128_CRTC_PITCH              0x022cU
#define R128_CUR_OFFSET              0x0260U
#define R128_CUR_HORZ_VERT_POSN      0x0264U
#define R128_CUR_HORZ_VERT_OFF       0x0268U
#define R128_CUR_CLR0                0x026cU
#define R128_CUR_CLR1                0x0270U

#define R128_RBBM_STATUS             0x0e40U

/* GUI engine registers. */
#define R128_DST_OFFSET              0x1404U
#define R128_DST_PITCH               0x1408U
#define R128_DST_WIDTH               0x140cU
#define R128_DST_HEIGHT              0x1410U
#define R128_SRC_X                   0x1414U
#define R128_SRC_Y                   0x1418U
#define R128_DST_X                   0x141cU
#define R128_DST_Y                   0x1420U
#define R128_SRC_PITCH_OFFSET        0x1428U
#define R128_DST_PITCH_OFFSET        0x142cU
#define R128_SRC_Y_X                 0x1434U
#define R128_DST_Y_X                 0x1438U
#define R128_DST_HEIGHT_WIDTH        0x143cU
#define R128_DP_GUI_MASTER_CNTL      0x146cU
#define R128_BRUSH_Y_X               0x1474U
#define R128_DP_BRUSH_BKGD_CLR       0x1478U
#define R128_BRUSH_DATA0             0x1480U
#define R128_BRUSH_DATA1             0x1484U
#define R128_BRUSH_DATA63            0x157cU
#define R128_DP_BRUSH_FRGD_CLR       0x147cU
#define R128_DST_WIDTH_X             0x1588U
#define R128_DST_HEIGHT_WIDTH_8      0x158cU
#define R128_SRC_X_Y                 0x1590U
#define R128_DST_X_Y                 0x1594U
#define R128_DST_WIDTH_HEIGHT        0x1598U
#define R128_DST_HEIGHT_Y            0x15a0U
#define R128_SRC_OFFSET              0x15acU
#define R128_SRC_PITCH               0x15b0U
#define R128_DST_HEIGHT_WIDTH_BW     0x15b4U
#define R128_CLR_CMP_CNTL            0x15c0U
#define R128_CLR_CMP_CLR_SRC         0x15c4U
#define R128_CLR_CMP_CLR_DST         0x15c8U
#define R128_CLR_CMP_MASK            0x15ccU
#define R128_DP_SRC_FRGD_CLR         0x15d8U
#define R128_DP_SRC_BKGD_CLR         0x15dcU
#define R128_DST_BRES_ERR            0x1628U
#define R128_DST_BRES_INC            0x162cU
#define R128_DST_BRES_DEC            0x1630U
#define R128_DST_BRES_LNTH           0x1634U
#define R128_SC_LEFT                 0x1640U
#define R128_SC_RIGHT                0x1644U
#define R128_SC_TOP                  0x1648U
#define R128_SC_BOTTOM               0x164cU
#define R128_SRC_SC_RIGHT            0x1654U
#define R128_SRC_SC_BOTTOM           0x165cU
#define R128_DP_CNTL                 0x16c0U
#define R128_DP_DATATYPE             0x16c4U
#define R128_DP_MIX                  0x16c8U
#define R128_DP_WRITE_MASK           0x16ccU
#define R128_DP_CNTL_XDIR_YDIR_YMAJOR 0x16d0U
#define R128_DEFAULT_OFFSET          0x16e0U
#define R128_DEFAULT_PITCH           0x16e4U
#define R128_DEFAULT_SC_BOTTOM_RIGHT 0x16e8U
#define R128_SC_TOP_LEFT             0x16ecU
#define R128_SC_BOTTOM_RIGHT         0x16f0U
#define R128_SRC_SC_BOTTOM_RIGHT     0x16f4U
#define R128_DST_TILE                0x1700U
#define R128_GUI_STAT                0x1740U
#define R128_PC_GUI_CTLSTAT          0x1748U
#define R128_GUI_FIFOCNT_MASK        0x00000fffU
#define R128_GUI_ACTIVE              0x80000000U
#define R128_PC_FLUSH_ALL            0x000000ffU
#define R128_PC_BUSY                 0x80000000U
#define R128_HOST_DATA0              0x17c0U
#define R128_HOST_DATA7              0x17dcU
#define R128_HOST_DATA_LAST          0x17e0U

/* PLL indices and fields. */
#define R128_PLL_WR_EN               0x00000080U
#define R128_PPLL_REF_DIV            0x03U
#define R128_PPLL_DIV_3              0x07U
#define R128_PPLL_REF_DIV_MASK       0x000003ffU
#define R128_PPLL_FB3_DIV_MASK       0x000007ffU
#define R128_PPLL_POST3_DIV_MASK     0x00070000U

/* Interrupts. */
#define R128_CRTC_VBLANK_INT         0x00000001U

/* Configuration and CRTC controls. */
#define R128_CFG_VGA_IO_DIS          0x00000400U
#define R128_CRTC_DBL_SCAN_EN        0x00000001U
#define R128_CRTC_INTERLACE_EN       0x00000002U
#define R128_CRTC_CUR_EN             0x00010000U
#define R128_CRTC_EXT_DISP_EN        0x01000000U
#define R128_CRTC_EN                 0x02000000U
#define R128_CRTC_PIX_WIDTH_MASK     0x00000700U
#define R128_CRTC_PIX_WIDTH_4BPP     0x00000100U
#define R128_CRTC_PIX_WIDTH_8BPP     0x00000200U
#define R128_CRTC_PIX_WIDTH_15BPP    0x00000300U
#define R128_CRTC_PIX_WIDTH_16BPP    0x00000400U
#define R128_CRTC_PIX_WIDTH_24BPP    0x00000500U
#define R128_CRTC_PIX_WIDTH_32BPP    0x00000600U
#define R128_CRTC_DISPLAY_DIS        0x00000400U
#define R128_DAC_8BIT_EN             0x00000100U

/* 2D engine controls. */
#define R128_DST_X_LEFT_TO_RIGHT     0x00000001U
#define R128_DST_Y_TOP_TO_BOTTOM     0x00000002U
#define R128_DST_X_TILE_EN           0x00000008U
#define R128_DST_Y_TILE_EN           0x00000010U

/* DP_CNTL_XDIR_YDIR_YMAJOR controls. */
#define R128_DST_LINE_Y_MAJOR        0x00000004U
#define R128_DST_LINE_Y_TOP_TO_BOTTOM 0x00008000U
#define R128_DST_LINE_X_LEFT_TO_RIGHT 0x80000000U

#define R128_DST_8BPP                0x00000002U
#define R128_DST_15BPP               0x00000003U
#define R128_DST_16BPP               0x00000004U
#define R128_DST_24BPP               0x00000005U
#define R128_DST_32BPP               0x00000006U
#define R128_DP_DST_DATATYPE         0x0000000fU
#define R128_DP_BRUSH_DATATYPE       0x00000f00U
#define R128_BRUSH_8X8_MONO_FG_BG   0x00000000U
#define R128_BRUSH_8X8_MONO_FG_LA   0x00000100U
#define R128_BRUSH_32X1_MONO_FG_BG  0x00000600U
#define R128_BRUSH_32X1_MONO_FG_LA  0x00000700U
#define R128_BRUSH_SOLIDCOLOR        0x00000d00U
#define R128_BRUSH_NONE              0x00000f00U
#define R128_SRC_MONO_FRGD_BKGD      0x00000000U
#define R128_SRC_MONO_FRGD           0x00010000U
#define R128_SRC_COLOR               0x00030000U
#define R128_DP_SRC_DATATYPE         0x00030000U
#define R128_DP_BYTE_PIX_ORDER       0x40000000U

#define R128_GMC_SRC_PITCH_OFFSET_CNTL 0x00000001U
#define R128_GMC_DST_PITCH_OFFSET_CNTL 0x00000002U
#define R128_GMC_SRC_CLIPPING          0x00000004U
#define R128_GMC_DST_CLIPPING          0x00000008U
#define R128_GMC_ROP3_MASK             0x00ff0000U

#define R128_DP_SRC_RECT             0x00000200U
#define R128_DP_SRC_HOST             0x00000300U
#define R128_DP_SRC_HOST_BYTEALIGN   0x00000400U
#define R128_DP_SRC_SOURCE           0x00000700U
#define R128_DP_ROP3                 0x00ff0000U

#define R128_ROP3_BLACKNESS          0x00U
#define R128_ROP3_SRCCOPY            0xccU
#define R128_ROP3_PATCOPY            0xf0U
#define R128_ROP3_WHITENESS          0xffU

/* Color-comparison controls used by transparent copies and image writes. */
#define R128_CLR_CMP_FN_MASK         0x00000007U
#define R128_SRC_CMP_EQ_COLOR        0x00000004U
#define R128_SRC_CMP_NEQ_COLOR       0x00000005U
#define R128_CLR_CMP_SRC_SOURCE      0x01000000U

#define R128_REG_INDEX(reg)          ((reg) >> 2)
#define R128_REG(dev, reg)           ((dev)->regs[R128_REG_INDEX(reg)])

typedef struct rage128_host_data_t {
    uint8_t  active;
    uint8_t  complete;
    uint8_t  mono;
    uint8_t  transparent;
    uint8_t  lsb_first;
    uint8_t  bytes_per_pixel;
    uint8_t  pixel_bytes_used;
    uint8_t  rop;

    uint32_t pixel_mask;
    uint32_t pattern;
    uint32_t write_mask;
    uint32_t foreground;
    uint32_t background;

    uint32_t dst_offset;
    uint32_t dst_stride;
    int32_t  dst_x;
    int32_t  dst_y;
    uint32_t width;
    uint32_t height;

    int32_t clip_left;
    int32_t clip_top;
    int32_t clip_right;
    int32_t clip_bottom;

    uint32_t row;
    uint32_t col;
    uint32_t padding_remaining;
    uint32_t pixel_accumulator;
} rage128_host_data_t;

typedef struct rage128_t {
    mem_mapping_t linear_mapping;
    mem_mapping_t mmio_mapping;
    rom_t         bios_rom;
    svga_t        svga;
    void         *i2c;
    void         *ddc;

    uint8_t pci_slot;
    uint8_t irq_state;
    uint8_t int_line;
    uint8_t pci_regs[256];

    uint32_t linear_base;
    uint32_t mmio_base;
    uint32_t io_base;
    uint32_t rom_base;
    uint32_t vram_size;
    uint32_t vram_mask;
    uint32_t rom_size;
    uint32_t rom_mask;
    uint16_t device_id;
    uint16_t subsystem_id;
    uint16_t mapped_io_base;

    uint8_t io_bar_mapped;
    uint8_t vga_io_mapped;
    uint8_t rom_enabled;
    uint8_t has_bios;
    uint8_t is_agp;
    uint8_t power_state;

    uint8_t  pll_index;
    uint32_t pll_regs[64];
    uint32_t regs[R128_MMIO_SIZE / sizeof(uint32_t)];

    rage128_host_data_t host_data;

    uint8_t warned_brush;
    uint8_t warned_source;
    uint8_t warned_tiling;
    uint8_t warned_host_data;
    uint8_t warned_host_direction;
    uint8_t warned_host_early_end;
    uint8_t warned_color_compare;
} rage128_t;

void rage128_accel_reset(rage128_t *dev);
void rage128_accel_reg_written(rage128_t *dev, uint32_t reg);

#endif /* VID_ATI_RAGE128_H */
