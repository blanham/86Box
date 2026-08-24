/*
 * This file is generated from the validated QEMU Rage 128 reference
 * implementation at commit c7061f0a9fedd6994e7403ec844c978c29f59320.
 * Emulator-boundary adaptations are performed by the 86Box source
 * transform; guest-visible PM4/3D behavior remains shared.
 */

/*
 * QEMU ATI Rage 128 fixed-function 3D reference renderer
 *
 * Copyright (c) 2026 Bryce Lanham
 *
 * This work is licensed under the GNU GPL license version 2 or later.
 */

#include "vid_ati_rage128_qemu_compat.h"
#include <stdlib.h>

#define ATI_3D_REG_BASE 0x1800
#define ATI_3D_REG_END  0x1e00
#define ATI_3D_MAX_VERTICES 65535
#define ATI_3D_MAX_RASTER_PIXELS (16U * 1024U * 1024U)

#define ATI_3D_TEX_Z_ENABLE          BIT(0)
#define ATI_3D_TEX_Z_WRITE_ENABLE    BIT(1)
#define ATI_3D_TEX_STENCIL_ENABLE    BIT(3)
#define ATI_3D_TEXMAP_ENABLE         BIT(4)
#define ATI_3D_SEC_TEXMAP_ENABLE     BIT(5)
#define ATI_3D_FOG_ENABLE            BIT(7)
#define ATI_3D_TEX_DITHER_ENABLE     BIT(8)
#define ATI_3D_ALPHA_ENABLE          BIT(9)
#define ATI_3D_ALPHA_TEST_ENABLE     BIT(10)
#define ATI_3D_SPEC_LIGHT_ENABLE     BIT(11)

#define ATI_3D_Z_PIX_WIDTH_MASK      (3U << 1)
#define ATI_3D_Z_TEST_MASK           (7U << 4)
#define ATI_3D_STENCIL_TEST_SHIFT    12
#define ATI_3D_STENCIL_FAIL_SHIFT    16
#define ATI_3D_STENCIL_ZPASS_SHIFT   20
#define ATI_3D_STENCIL_ZFAIL_SHIFT   24
#define ATI_3D_STENCIL_FIELD_MASK    7U
#define ATI_3D_STENCIL_REF_SHIFT     0
#define ATI_3D_STENCIL_TEST_MASK_SHIFT 16
#define ATI_3D_STENCIL_WRITE_MASK_SHIFT 24
#define ATI_3D_FOG_SOURCE_TABLE      BIT(14)
#define ATI_3D_FOG_TABLE_START_REG   0x1810U
#define ATI_3D_FOG_TABLE_END_REG     0x1814U
#define ATI_3D_FOG_TABLE_DENSITY_REG 0x181cU
#define ATI_3D_FOG_TABLE_INDEX_REG   0x1a14U
#define ATI_3D_FOG_TABLE_DATA_REG    0x1a18U
#define ATI_3D_CLR_CMP_COLOR_REG     0x1a24U
#define ATI_3D_CLR_CMP_MASK_REG      0x1a28U
#define ATI_3D_CLR_CMP_FCN_SHIFT     30
#define ATI_3D_CLR_CMP_FCN_MASK      3U

#define ATI_3D_ALPHA_COMB_SHIFT      12
#define ATI_3D_ALPHA_COMB_MASK       (3U << ATI_3D_ALPHA_COMB_SHIFT)
#define ATI_3D_ALPHA_COMB_ADD_CLAMP  0U
#define ATI_3D_ALPHA_COMB_SUB_SRC_DST_CLAMP 2U
#define ATI_3D_ALPHA_SRC_SHIFT       16
#define ATI_3D_ALPHA_DST_SHIFT       20
#define ATI_3D_ALPHA_TEST_SHIFT      24

#define ATI_3D_FRONT_DIR_CCW         BIT(0)
#define ATI_3D_BACKFACE_MASK         (3U << 1)
#define ATI_3D_FRONTFACE_MASK        (3U << 3)
#define ATI_3D_COLOR_MODE_MASK       (3U << 5)
#define ATI_3D_COLOR_SOLID           (0U << 5)
#define ATI_3D_COLOR_FLAT            (1U << 5)
#define ATI_3D_COLOR_GOURAUD         (2U << 5)
#define ATI_3D_COLOR_GOURAUD2        (3U << 5)
#define ATI_3D_FLAT_SHADE_VERTEX_OGL BIT(14)
#define ATI_3D_FACE_CULL             0U
#define ATI_3D_FACE_POINTS           1U
#define ATI_3D_FACE_LINES            2U
#define ATI_3D_FACE_SOLID            3U

#define ATI_3D_AUX1_ENABLE           BIT(0)
#define ATI_3D_AUX2_ENABLE           BIT(2)
#define ATI_3D_AUX3_ENABLE           BIT(4)

#define ATI_3D_Z_TILE                BIT(16)

#define ATI_3D_VC_PRIM_MASK          0x7U
#define ATI_3D_VC_WALK_MASK          0x30U
#define ATI_3D_VC_COUNT_SHIFT        16

#define ATI_3D_VERTEX_RHW            BIT(0)
#define ATI_3D_VERTEX_DIFFUSE_BGR    BIT(1)
#define ATI_3D_VERTEX_DIFFUSE_A      BIT(2)
#define ATI_3D_VERTEX_DIFFUSE_ARGB   BIT(3)
#define ATI_3D_VERTEX_SPEC_BGR       BIT(4)
#define ATI_3D_VERTEX_SPEC_F         BIT(5)
#define ATI_3D_VERTEX_SPEC_FRGB      BIT(6)
#define ATI_3D_VERTEX_ST             BIT(7)
#define ATI_3D_VERTEX_S2T2           BIT(8)
#define ATI_3D_VERTEX_RHW2           BIT(9)

#define ATI_3D_WALK_IND              0x10U
#define ATI_3D_WALK_LIST             0x20U
#define ATI_3D_WALK_RING             0x30U

#define ATI_3D_PRIM_POINT            1U
#define ATI_3D_PRIM_LINE             2U
#define ATI_3D_PRIM_POLYLINE         3U
#define ATI_3D_PRIM_TRI_LIST         4U
#define ATI_3D_PRIM_TRI_FAN          5U
#define ATI_3D_PRIM_TRI_STRIP        6U
#define ATI_3D_PRIM_TRI_TYPE2        7U

/* Primary fixed-function texture state. */
#define ATI_3D_TEX_MIN_FILTER_SHIFT  1
#define ATI_3D_TEX_MIN_FILTER_MASK   (7U << ATI_3D_TEX_MIN_FILTER_SHIFT)
#define ATI_3D_TEX_MAG_FILTER_SHIFT  4
#define ATI_3D_TEX_MAG_FILTER_MASK   (7U << ATI_3D_TEX_MAG_FILTER_SHIFT)
#define ATI_3D_TEX_MIP_DISABLE       BIT(7)
#define ATI_3D_TEX_CLAMP_S_SHIFT     8
#define ATI_3D_TEX_WRAP_S            BIT(10)
#define ATI_3D_TEX_CLAMP_T_SHIFT     11
#define ATI_3D_TEX_WRAP_T            BIT(13)
#define ATI_3D_TEX_CLAMP_MASK        3U
#define ATI_3D_TEX_PERSPECTIVE_DIS   BIT(14)
#define ATI_3D_TEX_FORMAT_SHIFT      16
#define ATI_3D_TEX_FORMAT_MASK       0xfU
#define ATI_3D_TEX_EXT_FORMAT_MASK   0xfff00000U
#define ATI_3D_TEX_PALETTE_SHIFT     20
#define ATI_3D_TEX_PALETTE_MASK      0xfU
#define ATI_3D_TEX_PSEUDO_DT_SHIFT   24
#define ATI_3D_TEX_PSEUDO_DT_MASK    0x3U
#define ATI_3D_TEX_PALETTED_STATE_MASK \
    ((ATI_3D_TEX_PALETTE_MASK << ATI_3D_TEX_PALETTE_SHIFT) | \
     (ATI_3D_TEX_PSEUDO_DT_MASK << ATI_3D_TEX_PSEUDO_DT_SHIFT))
#define ATI_3D_TEX_RESERVED_EXT_MASK \
    (ATI_3D_TEX_EXT_FORMAT_MASK & ~ATI_3D_TEX_PALETTED_STATE_MASK)

#define ATI_3D_TEX_OFFSET_ADDRESS_MASK 0x03ffffffU
#define ATI_3D_TEX_OFFSET_RESERVED_MASK 0x3c000000U
#define ATI_3D_TEX_OFFSET_TILE_MASK  0xc0000000U
#define ATI_3D_TEX_MAX_LEVELS        11U
#define ATI_3D_TEX_MAX_DIMENSION     1024U
#define ATI_3D_TEX_UNIT_COUNT        2U
#define ATI_3D_SEC_TEX_0_OFFSET_C    0x1d08U
#define ATI_3D_SEC_SELECT_SEC_ST     BIT(0)
#define ATI_3D_TEX_GART_BASE         UINT32_C(0x02000000)
#define ATI_3D_TEX_GART_END          UINT32_C(0x04000000)

#define ATI_3D_TEX_MIN_NEAREST       0U
#define ATI_3D_TEX_MIN_LINEAR        1U
#define ATI_3D_TEX_MIN_NEAREST_MIP_NEAREST 2U
#define ATI_3D_TEX_MIN_NEAREST_MIP_LINEAR  3U
#define ATI_3D_TEX_MIN_LINEAR_MIP_NEAREST  4U
#define ATI_3D_TEX_MIN_LINEAR_MIP_LINEAR   5U

#define ATI_3D_TEX_WRAP              0U
#define ATI_3D_TEX_MIRROR            1U
#define ATI_3D_TEX_CLAMP             2U
#define ATI_3D_TEX_BORDER            3U

#define ATI_3D_TEX_FMT_CI4           1U
#define ATI_3D_TEX_FMT_CI8           2U
#define ATI_3D_TEX_FMT_ARGB1555      3U
#define ATI_3D_TEX_FMT_RGB565        4U
#define ATI_3D_TEX_FMT_RGB888        5U
#define ATI_3D_TEX_FMT_ARGB8888      6U
#define ATI_3D_TEX_FMT_RGB332        7U
#define ATI_3D_TEX_FMT_Y8            8U
#define ATI_3D_TEX_FMT_RGB8          9U
#define ATI_3D_TEX_FMT_CI16          10U
#define ATI_3D_TEX_FMT_YVYU422       11U
#define ATI_3D_TEX_FMT_VYUY422       12U
#define ATI_3D_TEX_FMT_AYUV444       14U
#define ATI_3D_TEX_FMT_ARGB4444      15U

#define ATI_3D_COMB_COLOR_MASK       0xfU
#define ATI_3D_COMB_COLOR_FACTOR_SHIFT 4
#define ATI_3D_COMB_COLOR_FACTOR_MASK  0xfU
#define ATI_3D_COMB_FCN_MSB          BIT(8)
#define ATI_3D_COMB_INPUT_SHIFT      10
#define ATI_3D_COMB_INPUT_MASK       0xfU
#define ATI_3D_COMB_ALPHA_SHIFT      14
#define ATI_3D_COMB_ALPHA_MASK       0xfU
#define ATI_3D_COMB_ALPHA_FACTOR_SHIFT 18
#define ATI_3D_COMB_ALPHA_FACTOR_MASK  0xfU
#define ATI_3D_COMB_ALPHA_INPUT_SHIFT 25
#define ATI_3D_COMB_ALPHA_INPUT_MASK  0x7U

#define ATI_3D_COMB_DISABLE          0U
#define ATI_3D_COMB_COPY             1U
#define ATI_3D_COMB_COPY_INPUT       2U
#define ATI_3D_COMB_MODULATE         3U
#define ATI_3D_COMB_MODULATE2X       4U
#define ATI_3D_COMB_MODULATE4X       5U
#define ATI_3D_COMB_ADD              6U
#define ATI_3D_COMB_ADD_SIGNED       7U
#define ATI_3D_COMB_BLEND_VERTEX     8U
#define ATI_3D_COMB_BLEND_TEXTURE    9U
#define ATI_3D_COMB_BLEND_CONSTANT   10U
#define ATI_3D_COMB_BLEND_PREMULT    11U
#define ATI_3D_COMB_BLEND_PREVIOUS   12U
#define ATI_3D_COMB_BLEND_PREMULT_INV 13U
#define ATI_3D_COMB_ADD_SIGNED2X     14U
#define ATI_3D_COMB_BLEND_CONST_COLOR 15U

#define ATI_3D_SETUP_ST_DIRECT       BIT(9)
#define ATI_3D_TEX_LIGHT_FCN_MSB     BIT(6)
#define ATI_3D_TEX_CHROMA_KEY_ENABLE BIT(12)
#define ATI_3D_ALPHA_IN_TEX_LSB_A    BIT(13)
#define ATI_3D_TEX_LIGHT_FN_SHIFT    14
#define ATI_3D_TEX_LIGHT_FN_MASK     0x0003c000U
#define ATI_3D_ALPHA_LIGHT_FN_SHIFT  18
#define ATI_3D_ALPHA_LIGHT_FN_MASK   0x001c0000U
#define ATI_3D_TEX_ANTI_ALIAS         BIT(21)
#define ATI_3D_TEX_IDCT_ENABLE        BIT(22)
#define ATI_3D_TEX_CACHE_FLUSH        BIT(23)
#define ATI_3D_TEX_LOD_BIAS_SHIFT     24
#define ATI_3D_TEX_LOD_BIAS_MASK      0xff000000U
#define ATI_3D_TEX_LOD_BIAS_ZERO      0x3fU

typedef struct ATI3DSurface {
    uint32_t offset;
    uint32_t stride;
    uint32_t width;
    uint32_t pixel_mask;
    unsigned int datatype;
    unsigned int bpp;
    unsigned int bytes_per_pixel;
    bool tiled;
} ATI3DSurface;

typedef struct ATI3DVertex {
    float x;
    float y;
    float z;
    float rhw;
    float color[4];    /* red, green, blue, alpha */
    float specular[4]; /* red, green, blue, fog */
    float texcoord[2][2];
    float rhw2;        /* parsed for vertex-layout compatibility */
} ATI3DVertex;

typedef struct ATI3DTextureUnit {
    bool enabled;
    bool secondary;
    bool perspective;
    bool mipmapped;
    uint32_t control;
    uint32_t combine;
    uint32_t border;
    uint32_t offsets[ATI_3D_TEX_MAX_LEVELS];
    unsigned int coord_index;
    unsigned int width;
    unsigned int height;
    unsigned int max_level;
    unsigned int format;
    unsigned int bytes_per_texel;
    unsigned int palette_index;
    unsigned int pseudocolor_format;
} ATI3DTextureUnit;

typedef struct ATI3DTextureContext {
    ATI3DTextureUnit units[ATI_3D_TEX_UNIT_COUNT];
    uint32_t constant_color;
    float lod_bias;
} ATI3DTextureContext;

typedef struct ATI3DRect {
    int left;
    int top;
    int right;
    int bottom;
} ATI3DRect;

typedef struct ATI3DFragmentContext {
    rage128_t *s;
    ATI3DSurface *color_surface;
    ATI3DSurface depth_surface;
    uint32_t tex_control;
    uint32_t misc;
    uint32_t z_control;
    uint32_t stencil_ref_mask;
    uint32_t fog_color;
    uint32_t write_mask;
    uint32_t depth_mask;
    uint32_t depth_function;
    uint64_t color_dirty_start;
    uint64_t color_dirty_end;
    uint64_t depth_dirty_start;
    uint64_t depth_dirty_end;
    bool depth_enabled;
    bool stencil_enabled;
    bool fog_enabled;
    ATI3DTextureContext textures;
} ATI3DFragmentContext;

typedef struct ATI3DShadeState {
    unsigned int mode;
    bool flat_last;
    bool specular_enabled;
    float solid[4];
} ATI3DShadeState;

static uint32_t *rage128_3d_reg_ptr(rage128_t *s, uint32_t addr)
{
    if (addr < ATI_3D_REG_BASE || addr >= ATI_3D_REG_END || (addr & 3)) {
        return NULL;
    }
    return &s->accel3d.regs[(addr - ATI_3D_REG_BASE) / 4];
}

static const uint32_t *rage128_3d_reg_cptr(const rage128_t *s, uint32_t addr)
{
    if (addr < ATI_3D_REG_BASE || addr >= ATI_3D_REG_END || (addr & 3)) {
        return NULL;
    }
    return &s->accel3d.regs[(addr - ATI_3D_REG_BASE) / 4];
}

static uint32_t rage128_3d_reg(const rage128_t *s, uint32_t addr)
{
    const uint32_t *reg = rage128_3d_reg_cptr(s, addr);

    return reg ? *reg : 0;
}

static uint32_t rage128_3d_read_lane(uint32_t value, uint32_t addr,
                                 unsigned int size)
{
    unsigned int offset = addr & 3;

    if (!offset && size == 4) {
        return value;
    }
    return extract32(value, offset * 8, size * 8);
}

static void rage128_3d_write_lane(uint32_t *value, uint32_t addr, uint64_t data,
                              unsigned int size)
{
    unsigned int offset = addr & 3;

    if (!offset && size == 4) {
        *value = data;
    } else {
        *value = deposit32(*value, offset * 8, size * 8, data);
    }
}

static bool rage128_3d_aux_reg(rage128_t *s, uint32_t addr,
                           uint32_t **value)
{
    if (addr == AUX_SC_CNTL) {
        *value = &s->accel3d.aux_sc_cntl;
        return true;
    }
    if (addr >= AUX1_SC_LEFT && addr <= AUX3_SC_BOTTOM && !(addr & 3)) {
        unsigned int window = (addr - AUX1_SC_LEFT) / 0x10;
        unsigned int field = ((addr - AUX1_SC_LEFT) & 0xf) / 4;

        if (window < 3 && field < 4) {
            *value = &s->accel3d.aux_sc[window][field];
            return true;
        }
    }
    return false;
}

void rage128_3d_reset(rage128_t *s)
{
    memset(&s->accel3d, 0, sizeof(s->accel3d));
    s->accel3d.regs[(SC_BOTTOM_RIGHT_C - ATI_3D_REG_BASE) / 4] =
        UINT32_C(0x1fff1fff);
    s->accel3d.regs[(PLANE_3D_MASK_C - ATI_3D_REG_BASE) / 4] = UINT32_MAX;
    s->accel3d.regs[(RE_WIDTH_HEIGHT - ATI_3D_REG_BASE) / 4] =
        UINT32_C(0x1fff1fff);
}

bool rage128_3d_load_palette(rage128_t *s, uint32_t datatype,
                         const uint32_t *colors, unsigned int count)
{
    unsigned int format = datatype & 0xfU;
    unsigned int palette = (datatype >> 4) & 0xfU;

    if ((datatype & ~UINT32_C(0xff)) || !colors) {
        return false;
    }

    switch (format) {
    case ATI_3D_TEX_FMT_CI4:
        if (count != 16) {
            return false;
        }
        memcpy(s->accel3d.texture_palette4[palette], colors,
               count * sizeof(*colors));
        s->accel3d.texture_palette4_valid |= BIT(palette);
        return true;

    case ATI_3D_TEX_FMT_CI8:
        if (count != 256 || palette > 2) {
            return false;
        }
        if (palette == 0) {
            /*
             * The manual defines zero as "either of 2 palettes". Mirror the
             * upload into both banks, avoiding host-dependent arbitration
             * while preserving the documented freedom.
             */
            for (unsigned int bank = 0; bank < 2; bank++) {
                memcpy(s->accel3d.texture_palette8[bank], colors,
                       count * sizeof(*colors));
            }
            s->accel3d.texture_palette8_valid = 3;
        } else {
            unsigned int bank = palette - 1;

            memcpy(s->accel3d.texture_palette8[bank], colors,
                   count * sizeof(*colors));
            s->accel3d.texture_palette8_valid |= BIT(bank);
        }
        return true;

    default:
        return false;
    }
}

bool rage128_3d_mm_read(rage128_t *s, uint32_t addr, unsigned int size,
                    uint32_t *value)
{
    uint32_t *reg;
    uint32_t aligned = addr & ~3ULL;

    if (size != 1 && size != 2 && size != 4) {
        return false;
    }
    if (aligned == ATI_3D_FOG_TABLE_INDEX_REG) {
        *value = rage128_3d_read_lane(s->accel3d.fog_table_index, addr, size);
        return true;
    }
    if (aligned == ATI_3D_FOG_TABLE_DATA_REG) {
        *value = rage128_3d_read_lane(
            s->accel3d.fog_table[s->accel3d.fog_table_index], addr, size);
        return true;
    }
    if (aligned >= ATI_3D_REG_BASE && aligned < ATI_3D_REG_END) {
        reg = rage128_3d_reg_ptr(s, aligned);
        *value = rage128_3d_read_lane(*reg, addr, size);
        return true;
    }
    if (aligned >= GUI_SCRATCH_REG0 && aligned <= GUI_SCRATCH_REG5) {
        reg = &s->accel3d.gui_scratch[(aligned - GUI_SCRATCH_REG0) / 4];
        *value = rage128_3d_read_lane(*reg, addr, size);
        return true;
    }
    if (rage128_3d_aux_reg(s, aligned, &reg)) {
        *value = rage128_3d_read_lane(*reg, addr, size);
        return true;
    }
    if (aligned == RE_TOP_LEFT) {
        *value = rage128_3d_read_lane(s->accel3d.re_top_left, addr, size);
        return true;
    }
    return false;
}

bool rage128_3d_mm_write(rage128_t *s, uint32_t addr, uint64_t data,
                     unsigned int size)
{
    uint32_t *reg;
    uint32_t lane;
    uint32_t aligned = addr & ~3ULL;

    if (size != 1 && size != 2 && size != 4) {
        return false;
    }
    if (aligned == ATI_3D_FOG_TABLE_INDEX_REG) {
        lane = s->accel3d.fog_table_index;
        rage128_3d_write_lane(&lane, addr, data, size);
        s->accel3d.fog_table_index = lane;
        return true;
    }
    if (aligned == ATI_3D_FOG_TABLE_DATA_REG) {
        uint8_t index = s->accel3d.fog_table_index;

        lane = s->accel3d.fog_table[index];
        rage128_3d_write_lane(&lane, addr, data, size);
        s->accel3d.fog_table[index] = lane;
        s->accel3d.fog_table_index++;
        return true;
    }
    if (aligned >= ATI_3D_REG_BASE && aligned < ATI_3D_REG_END) {
        reg = rage128_3d_reg_ptr(s, aligned);
        rage128_3d_write_lane(reg, addr, data, size);
        return true;
    }
    if (aligned >= GUI_SCRATCH_REG0 && aligned <= GUI_SCRATCH_REG5) {
        reg = &s->accel3d.gui_scratch[(aligned - GUI_SCRATCH_REG0) / 4];
        rage128_3d_write_lane(reg, addr, data, size);
        return true;
    }
    if (rage128_3d_aux_reg(s, aligned, &reg)) {
        rage128_3d_write_lane(reg, addr, data, size);
        return true;
    }
    if (aligned == RE_TOP_LEFT) {
        rage128_3d_write_lane(&s->accel3d.re_top_left, addr, data, size);
        return true;
    }
    return false;
}

static bool rage128_3d_decode_surface(uint32_t master, uint32_t pitch_offset,
                                  ATI3DSurface *surface)
{
    uint32_t datatype = (master >> 8) & 0xf;
    uint32_t pitch_groups = (pitch_offset >> 21) & 0x3ff;

    memset(surface, 0, sizeof(*surface));
    surface->datatype = datatype;
    switch (datatype) {
    case DST_8BPP:
    case ATI_3D_TEX_FMT_Y8:
        surface->bpp = 8;
        surface->bytes_per_pixel = 1;
        surface->pixel_mask = 0xff;
        break;
    case DST_15BPP:
        surface->bpp = 15;
        surface->bytes_per_pixel = 2;
        surface->pixel_mask = 0x7fff;
        break;
    case DST_16BPP:
        surface->bpp = 16;
        surface->bytes_per_pixel = 2;
        surface->pixel_mask = 0xffff;
        break;
    case DST_24BPP:
        surface->bpp = 24;
        surface->bytes_per_pixel = 3;
        surface->pixel_mask = 0x00ffffff;
        break;
    case DST_32BPP:
        surface->bpp = 32;
        surface->bytes_per_pixel = 4;
        surface->pixel_mask = UINT32_MAX;
        break;
    default:
        return false;
    }

    surface->offset = (pitch_offset & 0x001fffffU) << 5;
    surface->width = pitch_groups * 8;
    surface->stride = surface->width * surface->bytes_per_pixel;
    surface->tiled = pitch_offset & R128_DST_PITCH_TILE;
    return surface->stride != 0;
}

static uint32_t rage128_3d_load_pixel(const rage128_t *s,
                                  const ATI3DSurface *surface,
                                  uint64_t address)
{
    const uint8_t *src = s->svga.vram + address;
    uint32_t value = 0;

    for (unsigned int byte = 0; byte < surface->bytes_per_pixel; byte++) {
        unsigned int shift = false ?
            (surface->bytes_per_pixel - 1 - byte) * 8 : byte * 8;
        value |= (uint32_t)src[byte] << shift;
    }
    return value & surface->pixel_mask;
}

static void rage128_3d_store_pixel(rage128_t *s, const ATI3DSurface *surface,
                               uint64_t address, uint32_t value,
                               uint32_t write_mask)
{
    uint8_t *dst = s->svga.vram + address;
    uint32_t old = rage128_3d_load_pixel(s, surface, address);
    uint32_t mask = write_mask & surface->pixel_mask;
    uint32_t merged = (value & mask) | (old & ~mask);

    for (unsigned int byte = 0; byte < surface->bytes_per_pixel; byte++) {
        unsigned int shift = false ?
            (surface->bytes_per_pixel - 1 - byte) * 8 : byte * 8;
        dst[byte] = merged >> shift;
    }
}

static void rage128_3d_unpack_surface_color(const ATI3DSurface *surface,
                                        uint32_t value, float color[4])
{
    switch (surface->bpp) {
    case 8:
        color[0] = color[1] = color[2] = value & 0xff;
        color[3] = 255;
        break;
    case 15:
        color[0] = ((value >> 10) & 0x1f) * (255.0f / 31.0f);
        color[1] = ((value >> 5) & 0x1f) * (255.0f / 31.0f);
        color[2] = (value & 0x1f) * (255.0f / 31.0f);
        color[3] = 255;
        break;
    case 16:
        color[0] = ((value >> 11) & 0x1f) * (255.0f / 31.0f);
        color[1] = ((value >> 5) & 0x3f) * (255.0f / 63.0f);
        color[2] = (value & 0x1f) * (255.0f / 31.0f);
        color[3] = 255;
        break;
    case 24:
        color[0] = (value >> 16) & 0xff;
        color[1] = (value >> 8) & 0xff;
        color[2] = value & 0xff;
        color[3] = 255;
        break;
    case 32:
        color[0] = (value >> 16) & 0xff;
        color[1] = (value >> 8) & 0xff;
        color[2] = value & 0xff;
        color[3] = (value >> 24) & 0xff;
        break;
    default:
        memset(color, 0, 4 * sizeof(*color));
        break;
    }
}

static uint8_t rage128_3d_clamp_channel(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 255.0f) {
        return 255;
    }
    return value + 0.5f;
}

static uint32_t rage128_3d_pack_argb8888(const float color[4])
{
    uint32_t red = rage128_3d_clamp_channel(color[0]);
    uint32_t green = rage128_3d_clamp_channel(color[1]);
    uint32_t blue = rage128_3d_clamp_channel(color[2]);
    uint32_t alpha = rage128_3d_clamp_channel(color[3]);

    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static bool rage128_3d_texture_chroma_accept(const ATI3DFragmentContext *ctx,
                                         const float texture[4])
{
    uint32_t texel = rage128_3d_pack_argb8888(texture);
    uint32_t compare = rage128_3d_reg(ctx->s, ATI_3D_CLR_CMP_COLOR_REG);
    uint32_t mask = rage128_3d_reg(ctx->s, ATI_3D_CLR_CMP_MASK_REG);
    bool equal = !((texel ^ compare) & mask);

    switch ((ctx->misc >> ATI_3D_CLR_CMP_FCN_SHIFT) &
            ATI_3D_CLR_CMP_FCN_MASK) {
    case 0: /* always write */
        return true;
    case 1: /* never write */
        return false;
    case 2: /* write equal texels */
        return equal;
    case 3: /* write unequal texels */
        return !equal;
    default:
        return false;
    }
}

static float rage128_3d_dither_offset(int x, int y, unsigned int bits)
{
    static const uint8_t matrix[4][4] = {
        { 0, 8, 2, 10 },
        { 12, 4, 14, 6 },
        { 3, 11, 1, 9 },
        { 15, 7, 13, 5 },
    };
    float threshold = ((float)matrix[y & 3][x & 3] + 0.5f) / 16.0f -
                      0.5f;

    return threshold * (255.0f / ((1U << bits) - 1U));
}

static uint32_t rage128_3d_pack_surface_color(const ATI3DSurface *surface,
                                          uint32_t tex_control,
                                          int x, int y,
                                          const float color[4])
{
    float packed[4];
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;

    memcpy(packed, color, sizeof(packed));
    if ((tex_control & ATI_3D_TEX_DITHER_ENABLE) &&
        (surface->bpp == 15 || surface->bpp == 16)) {
        packed[0] += rage128_3d_dither_offset(x, y, 5);
        packed[1] += rage128_3d_dither_offset(x, y,
                                          surface->bpp == 16 ? 6 : 5);
        packed[2] += rage128_3d_dither_offset(x, y, 5);
    }
    r = rage128_3d_clamp_channel(packed[0]);
    g = rage128_3d_clamp_channel(packed[1]);
    b = rage128_3d_clamp_channel(packed[2]);
    a = rage128_3d_clamp_channel(packed[3]);

    switch (surface->bpp) {
    case 8:
        return (r * 77 + g * 150 + b * 29) >> 8;
    case 15:
        return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case 16:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case 24:
        return (r << 16) | (g << 8) | b;
    case 32:
        return (a << 24) | (r << 16) | (g << 8) | b;
    default:
        return 0;
    }
}

static bool rage128_3d_surface_address(const rage128_t *s,
                                   const ATI3DSurface *surface,
                                   int x, int y, uint64_t *address)
{
    uint64_t offset;

    if (x < 0 || y < 0 || (uint32_t)x >= surface->width) {
        return false;
    }
    offset = (uint64_t)surface->offset +
             (uint64_t)(unsigned int)y * surface->stride +
             (uint64_t)(unsigned int)x * surface->bytes_per_pixel;
    if (offset + surface->bytes_per_pixel > s->vram_size) {
        return false;
    }
    *address = offset;
    return true;
}

static ATI3DRect rage128_3d_main_scissor(const rage128_t *s)
{
    uint32_t top_left = rage128_3d_reg(s, SC_TOP_LEFT_C);
    uint32_t bottom_right = rage128_3d_reg(s, SC_BOTTOM_RIGHT_C);
    ATI3DRect rect = {
        .left = top_left & 0x1fff,
        .top = (top_left >> 16) & 0x1fff,
        .right = bottom_right & 0x1fff,
        .bottom = (bottom_right >> 16) & 0x1fff,
    };
    uint32_t re_top_left = s->accel3d.re_top_left;
    uint32_t re_width_height = rage128_3d_reg(s, RE_WIDTH_HEIGHT);

    if (re_width_height) {
        int left = re_top_left & 0xffff;
        int top = (re_top_left >> 16) & 0xffff;
        int right = left + (re_width_height & 0xffff) - 1;
        int bottom = top + ((re_width_height >> 16) & 0xffff) - 1;

        rect.left = MAX(rect.left, left);
        rect.top = MAX(rect.top, top);
        rect.right = MIN(rect.right, right);
        rect.bottom = MIN(rect.bottom, bottom);
    }
    return rect;
}

static bool rage128_3d_aux_scissor_pass(const rage128_t *s, int x, int y)
{
    uint32_t control = s->accel3d.aux_sc_cntl;
    bool any_enabled = control &
        (ATI_3D_AUX1_ENABLE | ATI_3D_AUX2_ENABLE | ATI_3D_AUX3_ENABLE);

    if (!any_enabled) {
        return true;
    }
    for (unsigned int i = 0; i < 3; i++) {
        if (!(control & BIT(i * 2))) {
            continue;
        }
        if (x >= (int)(s->accel3d.aux_sc[i][0] & 0x1fff) &&
            x <= (int)(s->accel3d.aux_sc[i][1] & 0x1fff) &&
            y >= (int)(s->accel3d.aux_sc[i][2] & 0x1fff) &&
            y <= (int)(s->accel3d.aux_sc[i][3] & 0x1fff)) {
            return true;
        }
    }
    return false;
}

bool rage128_3d_surface_fill(rage128_t *s, uint32_t master,
                         uint32_t pitch_offset, uint32_t color,
                         uint32_t dst_xy, uint32_t width_height)
{
    ATI3DSurface surface;
    unsigned int x = dst_xy >> 16;
    unsigned int y = dst_xy & 0xffff;
    unsigned int width = width_height >> 16;
    unsigned int height = width_height & 0xffff;
    uint8_t rop = (master >> 16) & 0xff;
    uint32_t write_mask = master & R128_GMC_WR_MSK_DIS ?
                          UINT32_MAX : R128_REG(s, R128_DP_WRITE_MASK);
    uint64_t dirty_start = UINT64_MAX;
    uint64_t dirty_end = 0;

    if (!rage128_3d_decode_surface(master, pitch_offset, &surface)) {
        return false;
    }
    /*
     * Rage 128 tiled depth is canonicalized to a linear software backing.
     * Every software 3D access and PM4 depth clear uses the same layout, so
     * this is a coherent device model rather than a one-off clear hack.
     */
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int col = 0; col < width; col++) {
            uint64_t address;
            uint32_t old;
            uint32_t result;

            if (!rage128_3d_surface_address(s, &surface, x + col, y + row,
                                        &address)) {
                continue;
            }
            old = rage128_3d_load_pixel(s, &surface, address);
            result = ati_rop3_eval(rop, color, 0, old);
            rage128_3d_store_pixel(s, &surface, address, result, write_mask);
            dirty_start = MIN(dirty_start, address);
            dirty_end = MAX(dirty_end,
                            address + surface.bytes_per_pixel);
        }
    }
    if (dirty_start < dirty_end) {
        rage128_mark_vram_dirty(s, dirty_start,
                                dirty_end - dirty_start);
    }
    return true;
}

static unsigned int rage128_3d_vertex_stride(uint32_t format)
{
    unsigned int dwords = 3;

    if (format & ATI_3D_VERTEX_RHW) {
        dwords++;
    }
    if (format & (ATI_3D_VERTEX_DIFFUSE_BGR |
                  ATI_3D_VERTEX_DIFFUSE_A |
                  ATI_3D_VERTEX_DIFFUSE_ARGB)) {
        dwords++;
    }
    if (format & (ATI_3D_VERTEX_SPEC_BGR |
                  ATI_3D_VERTEX_SPEC_F |
                  ATI_3D_VERTEX_SPEC_FRGB)) {
        dwords++;
    }
    if (format & ATI_3D_VERTEX_ST) {
        dwords += 2;
    }
    if (format & ATI_3D_VERTEX_S2T2) {
        dwords += 2;
    }
    if (format & ATI_3D_VERTEX_RHW2) {
        dwords++;
    }
    return dwords;
}

static float rage128_3d_u32_to_float(uint32_t value)
{
    float result;

    memcpy(&result, &value, sizeof(result));
    return result;
}

static void rage128_3d_unpack_vertex_color(uint32_t packed, float color[4])
{
    /* Mesa's little-endian r128 vertices store bytes R,G,B,A/F. */
    color[0] = packed & 0xff;
    color[1] = (packed >> 8) & 0xff;
    color[2] = (packed >> 16) & 0xff;
    color[3] = (packed >> 24) & 0xff;
}

static bool rage128_3d_decode_vertex(const uint32_t *words,
                                 uint32_t format, unsigned int stride,
                                 ATI3DVertex *vertex)
{
    unsigned int index = 3;
    uint32_t diffuse = UINT32_C(0xffffffff);
    uint32_t specular = 0;

    if (stride > 12) {
        return false;
    }
    vertex->x = rage128_3d_u32_to_float(words[0]);
    vertex->y = rage128_3d_u32_to_float(words[1]);
    vertex->z = rage128_3d_u32_to_float(words[2]);
    vertex->rhw = 1.0f;
    if (format & ATI_3D_VERTEX_RHW) {
        vertex->rhw = rage128_3d_u32_to_float(words[index++]);
    }
    if (format & (ATI_3D_VERTEX_DIFFUSE_BGR |
                  ATI_3D_VERTEX_DIFFUSE_A |
                  ATI_3D_VERTEX_DIFFUSE_ARGB)) {
        diffuse = words[index++];
    }
    rage128_3d_unpack_vertex_color(diffuse, vertex->color);

    if (format & (ATI_3D_VERTEX_SPEC_BGR |
                  ATI_3D_VERTEX_SPEC_F |
                  ATI_3D_VERTEX_SPEC_FRGB)) {
        specular = words[index++];
    }
    rage128_3d_unpack_vertex_color(specular, vertex->specular);
    memset(vertex->texcoord, 0, sizeof(vertex->texcoord));
    vertex->rhw2 = vertex->rhw;
    if (format & ATI_3D_VERTEX_ST) {
        vertex->texcoord[0][0] = rage128_3d_u32_to_float(words[index++]);
        vertex->texcoord[0][1] = rage128_3d_u32_to_float(words[index++]);
    }
    if (format & ATI_3D_VERTEX_S2T2) {
        vertex->texcoord[1][0] = rage128_3d_u32_to_float(words[index++]);
        vertex->texcoord[1][1] = rage128_3d_u32_to_float(words[index++]);
    }
    if (format & ATI_3D_VERTEX_RHW2) {
        vertex->rhw2 = rage128_3d_u32_to_float(words[index++]);
    }
    return index == stride &&
           isfinite(vertex->x) && isfinite(vertex->y) &&
           isfinite(vertex->z) && isfinite(vertex->rhw) &&
           isfinite(vertex->rhw2) &&
           isfinite(vertex->texcoord[0][0]) &&
           isfinite(vertex->texcoord[0][1]) &&
           isfinite(vertex->texcoord[1][0]) &&
           isfinite(vertex->texcoord[1][1]);
}

static bool rage128_3d_read_vertex(rage128_t *s, uint32_t address,
                               uint32_t format, unsigned int stride,
                               ATI3DVertex *vertex)
{
    uint32_t words[12];

    if (stride > G_N_ELEMENTS(words) ||
        !rage128_pm4_read_guest(s, address, words, stride * sizeof(uint32_t))) {
        return false;
    }
    for (unsigned int i = 0; i < stride; i++) {
        words[i] = le32_to_cpu(words[i]);
    }
    return rage128_3d_decode_vertex(words, format, stride, vertex);
}

static float rage128_3d_edge(const ATI3DVertex *a, const ATI3DVertex *b,
                         float x, float y)
{
    return (x - a->x) * (b->y - a->y) -
           (y - a->y) * (b->x - a->x);
}

static bool rage128_3d_alpha_compare(unsigned int function,
                                 uint8_t source, uint8_t reference)
{
    switch (function) {
    case 0:
        return false;
    case 1:
        return source < reference;
    case 2:
        return source <= reference;
    case 3:
        return source == reference;
    case 4:
        return source >= reference;
    case 5:
        return source > reference;
    case 6:
        return source != reference;
    case 7:
        return true;
    default:
        return false;
    }
}

static float rage128_3d_blend_factor(unsigned int factor, unsigned int channel,
                                 const float src[4], const float dst[4])
{
    switch (factor) {
    case 0:
        return 0.0f;
    case 1:
        return 1.0f;
    case 2:
        return src[channel] / 255.0f;
    case 3:
        return 1.0f - src[channel] / 255.0f;
    case 4:
        return src[3] / 255.0f;
    case 5:
        return 1.0f - src[3] / 255.0f;
    case 6:
        return dst[3] / 255.0f;
    case 7:
        return 1.0f - dst[3] / 255.0f;
    case 8:
        return dst[channel] / 255.0f;
    case 9:
        return 1.0f - dst[channel] / 255.0f;
    case 10:
        return channel == 3 ? 1.0f :
               MIN(src[3], 255.0f - dst[3]) / 255.0f;
    default:
        return 0.0f;
    }
}

static bool rage128_3d_blend_equation_supported(uint32_t misc)
{
    unsigned int equation =
        (misc & ATI_3D_ALPHA_COMB_MASK) >> ATI_3D_ALPHA_COMB_SHIFT;

    return equation == ATI_3D_ALPHA_COMB_ADD_CLAMP ||
           equation == ATI_3D_ALPHA_COMB_SUB_SRC_DST_CLAMP;
}

static bool rage128_3d_blend(uint32_t misc, const float src[4],
                         const float dst[4], float result[4])
{
    unsigned int equation =
        (misc & ATI_3D_ALPHA_COMB_MASK) >> ATI_3D_ALPHA_COMB_SHIFT;
    unsigned int src_factor = (misc >> ATI_3D_ALPHA_SRC_SHIFT) & 0xf;
    unsigned int dst_factor = (misc >> ATI_3D_ALPHA_DST_SHIFT) & 0xf;

    if (src_factor > 10 || dst_factor > 10 ||
        !rage128_3d_blend_equation_supported(misc)) {
        return false;
    }
    for (unsigned int channel = 0; channel < 4; channel++) {
        float sf = rage128_3d_blend_factor(src_factor, channel, src, dst);
        float df = rage128_3d_blend_factor(dst_factor, channel, src, dst);
        float source = src[channel] * sf;
        float destination = dst[channel] * df;

        result[channel] =
            equation == ATI_3D_ALPHA_COMB_SUB_SRC_DST_CLAMP ?
            source - destination : source + destination;
    }
    return true;
}

static bool rage128_3d_depth_compare(uint32_t function,
                                 uint32_t source, uint32_t destination)
{
    switch (function) {
    case 0:
        return false;
    case 1:
        return source < destination;
    case 2:
        return source <= destination;
    case 3:
        return source == destination;
    case 4:
        return source >= destination;
    case 5:
        return source > destination;
    case 6:
        return source != destination;
    case 7:
        return true;
    default:
        return false;
    }
}

static bool rage128_3d_depth_surface(const rage128_t *s,
                                 ATI3DSurface *surface,
                                 uint32_t *depth_mask,
                                 uint32_t *depth_function)
{
    uint32_t z_offset = rage128_3d_reg(s, Z_OFFSET_C);
    uint32_t z_pitch = rage128_3d_reg(s, Z_PITCH_C);
    uint32_t z_control = rage128_3d_reg(s, Z_STEN_CNTL_C);
    unsigned int bytes;

    memset(surface, 0, sizeof(*surface));
    switch (z_control & ATI_3D_Z_PIX_WIDTH_MASK) {
    case 0U << 1:
        surface->bpp = 16;
        bytes = 2;
        *depth_mask = 0xffff;
        break;
    case 1U << 1:
        surface->bpp = 24;
        bytes = 4;
        *depth_mask = 0x00ffffff;
        break;
    case 2U << 1:
        surface->bpp = 32;
        bytes = 4;
        *depth_mask = UINT32_MAX;
        break;
    default:
        return false;
    }
    surface->bytes_per_pixel = bytes;
    /*
     * Z24 shares its fourth byte with stencil. Keep all 32 bits visible
     * to the load/store helpers while depth comparisons mask to 24 bits.
     */
    surface->pixel_mask = bytes == 4 ? UINT32_MAX : *depth_mask;
    surface->offset = z_offset & 0x07ffffff;
    surface->width = (z_pitch & 0xffff) * 8;
    surface->stride = surface->width * bytes;
    surface->tiled = z_pitch & ATI_3D_Z_TILE;
    *depth_function = (z_control & ATI_3D_Z_TEST_MASK) >> 4;
    return surface->stride != 0;
}

static uint32_t rage128_3d_float_depth(float value, uint32_t mask)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= mask) {
        return mask;
    }
    return value + 0.5f;
}

static unsigned int rage128_3d_face_mode(const rage128_t *s,
                                     float signed_area)
{
    uint32_t setup = s->pm4.vc_fpu_setup;
    bool ccw_front = setup & ATI_3D_FRONT_DIR_CCW;
    bool ccw = signed_area < 0.0f;
    bool front = ccw == ccw_front;

    return front ? (setup & ATI_3D_FRONTFACE_MASK) >> 3 :
                   (setup & ATI_3D_BACKFACE_MASK) >> 1;
}

static void rage128_3d_shade_state_init(const rage128_t *s,
                                    ATI3DShadeState *shade)
{
    shade->mode = s->pm4.vc_fpu_setup & ATI_3D_COLOR_MODE_MASK;
    shade->flat_last = s->pm4.vc_fpu_setup &
                       ATI_3D_FLAT_SHADE_VERTEX_OGL;
    shade->specular_enabled = rage128_3d_reg(s, TEX_CNTL_C) &
                              ATI_3D_SPEC_LIGHT_ENABLE;
    rage128_3d_unpack_vertex_color(rage128_3d_reg(s, SOLID_COLOR), shade->solid);
}

static void rage128_3d_shade_sample(const ATI3DShadeState *shade,
                                const ATI3DVertex *vertices,
                                const float *weights, unsigned int count,
                                float result[4], float specular[4])
{
    memset(specular, 0, 4 * sizeof(*specular));
    if (shade->mode == ATI_3D_COLOR_SOLID) {
        memcpy(result, shade->solid, sizeof(shade->solid));
        /* Fog is carried in the specular alpha byte independently of
         * the selected diffuse shading mode. */
        for (unsigned int i = 0; i < count; i++) {
            for (unsigned int channel = 0; channel < 4; channel++) {
                specular[channel] +=
                    vertices[i].specular[channel] * weights[i];
            }
        }
    } else if (shade->mode == ATI_3D_COLOR_FLAT) {
        unsigned int index = shade->flat_last ? count - 1 : 0;

        memcpy(result, vertices[index].color, sizeof(vertices[index].color));
        memcpy(specular, vertices[index].specular,
               sizeof(vertices[index].specular));
    } else {
        memset(result, 0, 4 * sizeof(*result));
        for (unsigned int i = 0; i < count; i++) {
            for (unsigned int channel = 0; channel < 4; channel++) {
                result[channel] += vertices[i].color[channel] * weights[i];
                specular[channel] +=
                    vertices[i].specular[channel] * weights[i];
            }
        }
    }
}

static void rage128_3d_shade_add_specular(const ATI3DShadeState *shade,
                                      const float specular[4],
                                      float result[4])
{
    if (!shade->specular_enabled || shade->mode == ATI_3D_COLOR_SOLID) {
        return;
    }
    for (unsigned int channel = 0; channel < 3; channel++) {
        result[channel] = MIN(result[channel] + specular[channel], 255.0f);
    }
}

static void rage128_3d_unpack_argb(uint32_t value, float color[4])
{
    color[0] = (value >> 16) & 0xff;
    color[1] = (value >> 8) & 0xff;
    color[2] = value & 0xff;
    color[3] = (value >> 24) & 0xff;
}

static uint8_t rage128_3d_stencil_apply(unsigned int operation,
                                    uint8_t current, uint8_t reference)
{
    switch (operation) {
    case 0: /* keep */
        return current;
    case 1: /* zero */
        return 0;
    case 2: /* replace */
        return reference;
    case 3: /* saturating increment */
        return current == UINT8_MAX ? current : current + 1;
    case 4: /* saturating decrement */
        return current == 0 ? 0 : current - 1;
    case 5: /* invert */
        return (uint8_t)~current;
    case 6: /* wrapping increment */
        return (uint8_t)(current + 1);
    case 7: /* wrapping decrement */
        return (uint8_t)(current - 1);
    default:
        return current;
    }
}

static void rage128_3d_stencil_write(ATI3DFragmentContext *ctx,
                                 uint64_t address, uint32_t pixel,
                                 unsigned int operation)
{
    uint8_t reference =
        (ctx->stencil_ref_mask >> ATI_3D_STENCIL_REF_SHIFT) & 0xff;
    uint8_t write_mask =
        (ctx->stencil_ref_mask >> ATI_3D_STENCIL_WRITE_MASK_SHIFT) &
        0xff;
    uint8_t current = pixel >> 24;
    uint8_t result;

    if (!write_mask) {
        return;
    }
    result = rage128_3d_stencil_apply(operation, current, reference);
    rage128_3d_store_pixel(ctx->s, &ctx->depth_surface, address,
                       (uint32_t)result << 24,
                       (uint32_t)write_mask << 24);
    ctx->depth_dirty_start = MIN(ctx->depth_dirty_start, address);
    ctx->depth_dirty_end = MAX(ctx->depth_dirty_end,
                               address +
                               ctx->depth_surface.bytes_per_pixel);
}

static uint8_t rage128_3d_table_fog_factor(const ATI3DFragmentContext *ctx,
                                       float z)
{
    unsigned int index;

    /*
     * The software renderer consumes the post-viewport z supplied by the
     * command stream.  Direct table mode addresses the 256-entry table with
     * the nearest clamped integer z.  Parameterized start/end/density mapping
     * is rejected during context validation until its fixed-point encoding is
     * independently documented.
     */
    if (!isfinite(z) || z <= 0.0f) {
        index = 0;
    } else if (z >= 255.0f) {
        index = 255;
    } else {
        index = z + 0.5f;
    }
    return ctx->s->accel3d.fog_table[index];
}

static void rage128_3d_apply_fog(const ATI3DFragmentContext *ctx,
                             const float specular[4], float z,
                             float color[4])
{
    float fog[4];
    float factor;

    if (!ctx->fog_enabled) {
        return;
    }
    if (ctx->misc & ATI_3D_FOG_SOURCE_TABLE) {
        factor = rage128_3d_table_fog_factor(ctx, z) / 255.0f;
    } else {
        factor = MIN(MAX(specular[3] / 255.0f, 0.0f), 1.0f);
    }
    rage128_3d_unpack_argb(ctx->fog_color, fog);
    for (unsigned int channel = 0; channel < 3; channel++) {
        color[channel] = color[channel] * factor +
                         fog[channel] * (1.0f - factor);
    }
}

static bool rage128_3d_texture_read(rage128_t *s, uint32_t address,
                                void *buffer, size_t size)
{
    uint64_t end = (uint64_t)address + size;

    if (!size) {
        return true;
    }

    /*
     * Rage 128 AGP textures use the same 32 MiB virtual aperture as the
     * PM4 engine.  The aperture numerically overlaps the upper half of a
     * 64 MiB framebuffer BAR, so test it before treating an offset as VRAM.
     */
    if (address >= ATI_3D_TEX_GART_BASE) {
        if (end > ATI_3D_TEX_GART_END) {
            return false;
        }
        return rage128_pm4_read_guest(s, address, buffer, size);
    }
    if (end > s->vram_size) {
        return false;
    }
    memcpy(buffer, s->svga.vram + address, size);
    return true;
}

static unsigned int rage128_3d_texture_bytes(unsigned int format)
{
    switch (format) {
    case ATI_3D_TEX_FMT_CI4:
    case ATI_3D_TEX_FMT_CI8:
    case ATI_3D_TEX_FMT_RGB332:
    case ATI_3D_TEX_FMT_Y8:
    case ATI_3D_TEX_FMT_RGB8:
        return 1;
    case ATI_3D_TEX_FMT_CI16:
    case ATI_3D_TEX_FMT_ARGB1555:
    case ATI_3D_TEX_FMT_RGB565:
    case ATI_3D_TEX_FMT_YVYU422:
    case ATI_3D_TEX_FMT_VYUY422:
    case ATI_3D_TEX_FMT_ARGB4444:
        return 2;
    case ATI_3D_TEX_FMT_RGB888:
        return 3;
    case ATI_3D_TEX_FMT_ARGB8888:
    case ATI_3D_TEX_FMT_AYUV444:
        return 4;
    default:
        return 0;
    }
}

static bool rage128_3d_texture_color_factor_valid(unsigned int factor,
                                              bool secondary)
{
    switch (factor) {
    case 0: /* constant color */
    case 1: /* inverse constant color */
    case 4: /* texture color */
    case 5: /* inverse texture color */
    case 6: /* texture alpha */
    case 7: /* inverse texture alpha */
        return true;
    case 8: /* previous color */
        return secondary;
    default:
        return false;
    }
}

static bool rage128_3d_texture_color_input_valid(unsigned int input,
                                             bool secondary)
{
    if (input >= 2 && input <= 5) {
        return true;
    }
    return secondary && (input == 8 || input == 9);
}

static bool rage128_3d_texture_alpha_factor_valid(unsigned int factor)
{
    return factor == 6 || factor == 7;
}

static bool rage128_3d_texture_alpha_input_valid(unsigned int input,
                                             bool secondary)
{
    return input == 1 || input == 2 || (secondary && input == 4);
}

static bool rage128_3d_texture_blend_color_pro(uint32_t combine)
{
    unsigned int color_op = combine & ATI_3D_COMB_COLOR_MASK;
    unsigned int color_factor =
        (combine >> ATI_3D_COMB_COLOR_FACTOR_SHIFT) &
        ATI_3D_COMB_COLOR_FACTOR_MASK;

    /*
     * Rage 128 Pro/M3 extends MODULATE2X with FCN_MSB into the GL_BLEND
     * equation emitted by Mesa: Cf * (1 - Ct) + Cc * Ct.
     */
    return (combine & ATI_3D_COMB_FCN_MSB) &&
           color_op == ATI_3D_COMB_MODULATE2X &&
           color_factor == 0;
}

static bool rage128_3d_texture_combine_validate(uint32_t combine,
                                            bool secondary)
{
    unsigned int color_op = combine & ATI_3D_COMB_COLOR_MASK;
    unsigned int color_factor =
        (combine >> ATI_3D_COMB_COLOR_FACTOR_SHIFT) &
        ATI_3D_COMB_COLOR_FACTOR_MASK;
    unsigned int color_input =
        (combine >> ATI_3D_COMB_INPUT_SHIFT) & ATI_3D_COMB_INPUT_MASK;
    unsigned int alpha_op =
        (combine >> ATI_3D_COMB_ALPHA_SHIFT) & ATI_3D_COMB_ALPHA_MASK;
    unsigned int alpha_factor =
        (combine >> ATI_3D_COMB_ALPHA_FACTOR_SHIFT) &
        ATI_3D_COMB_ALPHA_FACTOR_MASK;
    unsigned int alpha_input =
        (combine >> ATI_3D_COMB_ALPHA_INPUT_SHIFT) &
        ATI_3D_COMB_ALPHA_INPUT_MASK;

    if ((combine & ATI_3D_COMB_FCN_MSB) &&
        !rage128_3d_texture_blend_color_pro(combine)) {
        return false;
    }
    switch (color_op) {
    case ATI_3D_COMB_DISABLE:
        if (color_factor != 4) {
            return false;
        }
        break;
    case ATI_3D_COMB_COPY:
        if (!rage128_3d_texture_color_factor_valid(color_factor, secondary)) {
            return false;
        }
        break;
    case ATI_3D_COMB_COPY_INPUT:
        if (!rage128_3d_texture_color_input_valid(color_input, secondary)) {
            return false;
        }
        break;
    default:
        if (!rage128_3d_texture_color_factor_valid(color_factor, secondary) ||
            !rage128_3d_texture_color_input_valid(color_input, secondary)) {
            return false;
        }
        break;
    }
    switch (alpha_op) {
    case ATI_3D_COMB_DISABLE:
        if (alpha_factor != 6) {
            return false;
        }
        break;
    case ATI_3D_COMB_COPY:
        if (!rage128_3d_texture_alpha_factor_valid(alpha_factor)) {
            return false;
        }
        break;
    case ATI_3D_COMB_COPY_INPUT:
        if (!rage128_3d_texture_alpha_input_valid(alpha_input, secondary)) {
            return false;
        }
        break;
    case ATI_3D_COMB_MODULATE:
    case ATI_3D_COMB_MODULATE2X:
    case ATI_3D_COMB_MODULATE4X:
    case ATI_3D_COMB_ADD:
    case ATI_3D_COMB_ADD_SIGNED:
    case ATI_3D_COMB_ADD_SIGNED2X:
        if (!rage128_3d_texture_alpha_factor_valid(alpha_factor) ||
            !rage128_3d_texture_alpha_input_valid(alpha_input, secondary)) {
            return false;
        }
        break;
    default:
        return false;
    }
    return true;
}

static bool rage128_3d_texture_unit_init(ATI3DTextureUnit *unit,
                                     const rage128_t *s,
                                     unsigned int unit_index)
{
    uint32_t packed = rage128_3d_reg(s, TEX_SIZE_PITCH_C);
    uint32_t control_reg;
    uint32_t combine_reg;
    uint32_t border_reg;
    uint32_t offset_reg;
    uint32_t enable_bit;
    const char *name;
    unsigned int pitch_log2;
    unsigned int size_log2;
    unsigned int height_log2;
    unsigned int min_log2;
    unsigned int min_filter;
    unsigned int mag_filter;
    bool paletted;

    if (unit_index >= ATI_3D_TEX_UNIT_COUNT) {
        return false;
    }
    memset(unit, 0, sizeof(*unit));
    unit->secondary = unit_index == 1;
    if (unit->secondary) {
        packed >>= 16;
        control_reg = SEC_TEX_CNTL_C;
        combine_reg = SEC_TEX_COMBINE_CNTL_C;
        border_reg = SEC_TEXTURE_BORDER_COLOR_C;
        offset_reg = ATI_3D_SEC_TEX_0_OFFSET_C;
        enable_bit = ATI_3D_SEC_TEXMAP_ENABLE;
        name = "secondary";
    } else {
        packed &= 0xffff;
        control_reg = PRIM_TEX_CNTL_C;
        combine_reg = PRIM_TEX_COMBINE_CNTL_C;
        border_reg = PRIM_TEXTURE_BORDER_COLOR_C;
        offset_reg = PRIM_TEX_0_OFFSET_C;
        enable_bit = ATI_3D_TEXMAP_ENABLE;
        name = "primary";
    }
    unit->enabled = rage128_3d_reg(s, TEX_CNTL_C) & enable_bit;
    if (!unit->enabled) {
        return true;
    }
    unit->control = rage128_3d_reg(s, control_reg);
    unit->combine = rage128_3d_reg(s, combine_reg);
    unit->border = rage128_3d_reg(s, border_reg);
    unit->format = (unit->control >> ATI_3D_TEX_FORMAT_SHIFT) &
                   ATI_3D_TEX_FORMAT_MASK;
    unit->palette_index =
        (unit->control >> ATI_3D_TEX_PALETTE_SHIFT) &
        ATI_3D_TEX_PALETTE_MASK;
    unit->pseudocolor_format =
        (unit->control >> ATI_3D_TEX_PSEUDO_DT_SHIFT) &
        ATI_3D_TEX_PSEUDO_DT_MASK;
    paletted = unit->format == ATI_3D_TEX_FMT_CI4 ||
               unit->format == ATI_3D_TEX_FMT_CI8 ||
               unit->format == ATI_3D_TEX_FMT_CI16;
    unit->coord_index = unit->secondary &&
                        (unit->control & ATI_3D_SEC_SELECT_SEC_ST) ? 1 : 0;
    unit->perspective = !(unit->control & ATI_3D_TEX_PERSPECTIVE_DIS) &&
                        !(rage128_3d_reg(s, SETUP_CNTL) &
                          ATI_3D_SETUP_ST_DIRECT);
    if (unit->control & (ATI_3D_TEX_WRAP_S | ATI_3D_TEX_WRAP_T |
                         ATI_3D_TEX_RESERVED_EXT_MASK)) {
        rage128_log(
                      "ATI Rage 128 cylindrical or compressed %s texture is not implemented\n",
                      name);
        return false;
    }
    if (!paletted &&
        (unit->control & ATI_3D_TEX_PALETTED_STATE_MASK)) {
        rage128_log(
                      "ATI Rage 128 non-paletted %s texture has palette state\n",
                      name);
        return false;
    }
    if (paletted && unit->pseudocolor_format > 2) {
        rage128_log(
                      "ATI Rage 128 %s pseudocolor datatype is reserved\n",
                      name);
        return false;
    }
    if (unit->format == ATI_3D_TEX_FMT_CI4) {
        if (!(s->accel3d.texture_palette4_valid &
              BIT(unit->palette_index))) {
            rage128_log(
                          "ATI Rage 128 %s CI4 palette is not loaded\n",
                          name);
            return false;
        }
    } else if (unit->format == ATI_3D_TEX_FMT_CI8 ||
               unit->format == ATI_3D_TEX_FMT_CI16) {
        unsigned int selector = unit->palette_index;

        if (selector > 2) {
            rage128_log(
                          "ATI Rage 128 %s CI8 palette selection is reserved\n",
                          name);
            return false;
        }
        if (selector == 0) {
            if (s->accel3d.texture_palette8_valid & BIT(0)) {
                unit->palette_index = 0;
            } else if (s->accel3d.texture_palette8_valid & BIT(1)) {
                unit->palette_index = 1;
            } else {
                rage128_log(
                              "ATI Rage 128 %s CI8 palette is not loaded\n",
                              name);
                return false;
            }
        } else {
            unit->palette_index = selector - 1;
            if (!(s->accel3d.texture_palette8_valid &
                  BIT(unit->palette_index))) {
                rage128_log(
                              "ATI Rage 128 selected %s CI8 palette is not loaded\n",
                              name);
                return false;
            }
        }
    }
    min_filter = (unit->control & ATI_3D_TEX_MIN_FILTER_MASK) >>
                 ATI_3D_TEX_MIN_FILTER_SHIFT;
    mag_filter = (unit->control & ATI_3D_TEX_MAG_FILTER_MASK) >>
                 ATI_3D_TEX_MAG_FILTER_SHIFT;
    if (min_filter > ATI_3D_TEX_MIN_LINEAR_MIP_LINEAR ||
        mag_filter > 1) {
        rage128_log(
                      "ATI Rage 128 %s texture filter mode is not implemented\n",
                      name);
        return false;
    }
    pitch_log2 = packed & 0xf;
    size_log2 = (packed >> 4) & 0xf;
    height_log2 = (packed >> 8) & 0xf;
    min_log2 = (packed >> 12) & 0xf;
    unit->bytes_per_texel = rage128_3d_texture_bytes(unit->format);
    if (!unit->bytes_per_texel ||
        pitch_log2 > 10 || height_log2 > 10 || size_log2 > 10 ||
        size_log2 != MAX(pitch_log2, height_log2) ||
        min_log2 > size_log2) {
        rage128_log(
                      "ATI Rage 128 unsupported %s texture format or dimensions\n",
                      name);
        return false;
    }
    unit->width = 1U << pitch_log2;
    unit->height = 1U << height_log2;
    if (unit->width > ATI_3D_TEX_MAX_DIMENSION ||
        unit->height > ATI_3D_TEX_MAX_DIMENSION) {
        return false;
    }
    unit->mipmapped = !(unit->control & ATI_3D_TEX_MIP_DISABLE);
    if ((unit->format == ATI_3D_TEX_FMT_YVYU422 ||
         unit->format == ATI_3D_TEX_FMT_VYUY422) &&
        (unit->mipmapped || (unit->width & 1U))) {
        rage128_log(
                      "ATI Rage 128 packed YUV textures require an "
                      "even, non-mipmapped width\n");
        return false;
    }
    unit->max_level = unit->mipmapped ? size_log2 - min_log2 : 0;
    if (unit->max_level >= ATI_3D_TEX_MAX_LEVELS ||
        !rage128_3d_texture_combine_validate(unit->combine,
                                         unit->secondary)) {
        rage128_log(
                      "ATI Rage 128 %s texture mip or combine state is not implemented\n",
                      name);
        return false;
    }
    for (unsigned int index = 0; index <= unit->max_level; index++) {
        uint32_t raw = rage128_3d_reg(s, offset_reg + index * 4);

        if (raw & ATI_3D_TEX_OFFSET_TILE_MASK) {
            rage128_log(
                          "ATI Rage 128 tiled %s textures are not implemented\n",
                          name);
            return false;
        }
        if (raw & ATI_3D_TEX_OFFSET_RESERVED_MASK) {
            rage128_log(
                          "ATI Rage 128 %s texture offset has reserved bits set\n",
                          name);
            return false;
        }
        unit->offsets[index] = raw & ATI_3D_TEX_OFFSET_ADDRESS_MASK;
    }
    return true;
}

static bool rage128_3d_texture_lighting_validate(uint32_t tex_control)
{
    unsigned int color_function =
        (tex_control & ATI_3D_TEX_LIGHT_FN_MASK) >>
        ATI_3D_TEX_LIGHT_FN_SHIFT;
    unsigned int alpha_function =
        (tex_control & ATI_3D_ALPHA_LIGHT_FN_MASK) >>
        ATI_3D_ALPHA_LIGHT_FN_SHIFT;

    /*
     * TEX_LIGHT_FCN_MSB is a separate Pro-generation extension rather than
     * part of the documented four-bit TEX_LIGHT_FN table. Keep that extension
     * fail-closed until its altered equations have independent vectors.
     */
    if (tex_control & ATI_3D_TEX_LIGHT_FCN_MSB) {
        return false;
    }

    switch (color_function) {
    case ATI_3D_COMB_DISABLE:
    case ATI_3D_COMB_COPY:
    case ATI_3D_COMB_COPY_INPUT:
    case ATI_3D_COMB_MODULATE:
    case ATI_3D_COMB_MODULATE2X:
    case ATI_3D_COMB_MODULATE4X:
    case ATI_3D_COMB_ADD:
    case ATI_3D_COMB_ADD_SIGNED:
    case ATI_3D_COMB_BLEND_VERTEX:
    case ATI_3D_COMB_BLEND_TEXTURE:
    case ATI_3D_COMB_BLEND_CONSTANT:
    case ATI_3D_COMB_BLEND_PREVIOUS:
    case ATI_3D_COMB_ADD_SIGNED2X:
    case ATI_3D_COMB_BLEND_CONST_COLOR:
        break;
    default:
        return false;
    }

    return alpha_function <= ATI_3D_COMB_ADD_SIGNED;
}

static bool rage128_3d_texture_context_init(ATI3DTextureContext *textures,
                                        const rage128_t *s)
{
    uint32_t tex_control = rage128_3d_reg(s, TEX_CNTL_C);

    memset(textures, 0, sizeof(*textures));
    textures->constant_color = rage128_3d_reg(s, CONSTANT_COLOR_C);
    if ((tex_control & ATI_3D_SEC_TEXMAP_ENABLE) &&
        !(tex_control & ATI_3D_TEXMAP_ENABLE)) {
        rage128_log(
                      "ATI Rage 128 secondary texture requires the primary texture unit\n");
        return false;
    }
    if ((tex_control & ATI_3D_TEX_CHROMA_KEY_ENABLE) &&
        !(tex_control & ATI_3D_TEXMAP_ENABLE)) {
        rage128_log(
                      "ATI Rage 128 texture chroma key requires the primary texture unit\n");
        return false;
    }
    if ((tex_control & ATI_3D_TEX_CHROMA_KEY_ENABLE) &&
        (tex_control & ATI_3D_SEC_TEXMAP_ENABLE)) {
        rage128_log(
                      "ATI Rage 128 legacy texture chroma key does not support multitexture\n");
        return false;
    }
    if (tex_control & (ATI_3D_ALPHA_IN_TEX_LSB_A |
                       ATI_3D_TEX_ANTI_ALIAS |
                       ATI_3D_TEX_IDCT_ENABLE)) {
        rage128_log(
                      "ATI Rage 128 texture alpha-source, antialias, or IDCT state is not implemented\n");
        return false;
    }
    if (!rage128_3d_texture_lighting_validate(tex_control)) {
        rage128_log(
                      "ATI Rage 128 post-texture lighting function is not implemented\n");
        return false;
    }
    /*
     * Mesa programs the signed byte sequence 0x7f, 0x3f, 0x00,
     * 0xc0, and 0x80 for progressively more positive GL LOD bias.
     * Interpreting the 0x3f hardware zero around a 1/256-LOD step
     * reproduces those historical quarter-level buckets, including
     * the deliberately limited positive range noted by the driver.
     */
    textures->lod_bias =
        ((float)ATI_3D_TEX_LOD_BIAS_ZERO -
         (float)(int8_t)(tex_control >> ATI_3D_TEX_LOD_BIAS_SHIFT)) /
        256.0f;
    for (unsigned int index = 0; index < ATI_3D_TEX_UNIT_COUNT; index++) {
        if (!rage128_3d_texture_unit_init(&textures->units[index], s, index)) {
            return false;
        }
    }
    return true;
}

static int rage128_3d_texture_wrap_index(int value, int size,
                                     unsigned int mode, bool *border)
{
    int period;

    *border = false;
    switch (mode) {
    case ATI_3D_TEX_WRAP:
        value %= size;
        if (value < 0) {
            value += size;
        }
        return value;
    case ATI_3D_TEX_MIRROR:
        period = size * 2;
        value %= period;
        if (value < 0) {
            value += period;
        }
        return value < size ? value : period - 1 - value;
    case ATI_3D_TEX_CLAMP:
        return MIN(MAX(value, 0), size - 1);
    case ATI_3D_TEX_BORDER:
        if (value < 0 || value >= size) {
            *border = true;
            return 0;
        }
        return value;
    default:
        *border = true;
        return 0;
    }
}

static float rage128_3d_texture_quantize(float value, unsigned int bits)
{
    unsigned int maximum = (1U << bits) - 1U;
    float clamped = MIN(MAX(value, 0.0f), 255.0f);
    unsigned int quantized = clamped * maximum / 255.0f + 0.5f;

    return quantized * (255.0f / maximum);
}

static void rage128_3d_texture_palette_unpack(
    const rage128_t *s, const ATI3DTextureUnit *unit,
    unsigned int index, float color[4])
{
    uint32_t entry;

    if (unit->format == ATI_3D_TEX_FMT_CI4) {
        entry = s->accel3d.texture_palette4[unit->palette_index][index & 0xf];
    } else {
        entry = s->accel3d.texture_palette8[unit->palette_index][index & 0xff];
    }
    rage128_3d_unpack_argb(entry, color);

    switch (unit->pseudocolor_format) {
    case 0: /* RGB565 */
        color[0] = rage128_3d_texture_quantize(color[0], 5);
        color[1] = rage128_3d_texture_quantize(color[1], 6);
        color[2] = rage128_3d_texture_quantize(color[2], 5);
        color[3] = 255.0f;
        break;
    case 1: /* ARGB1555 */
        color[0] = rage128_3d_texture_quantize(color[0], 5);
        color[1] = rage128_3d_texture_quantize(color[1], 5);
        color[2] = rage128_3d_texture_quantize(color[2], 5);
        color[3] = color[3] >= 128.0f ? 255.0f : 0.0f;
        break;
    case 2: /* ARGB4444 */
        for (unsigned int channel = 0; channel < 4; channel++) {
            color[channel] = rage128_3d_texture_quantize(color[channel], 4);
        }
        break;
    default:
        memset(color, 0, 4 * sizeof(*color));
        break;
    }
}

static bool rage128_3d_texture_yuv422_texel(
    rage128_t *s, const ATI3DTextureUnit *unit,
    unsigned int level, int x, int y, float color[4])
{
    unsigned int width = MAX(unit->width >> level, 1U);
    unsigned int offset_index = unit->max_level - level;
    unsigned int pair_x = (unsigned int)x & ~1U;
    uint8_t bytes[4];
    uint16_t first;
    uint16_t second;
    uint8_t y0;
    uint8_t y1;
    uint8_t cb;
    uint8_t cr;
    uint8_t luma;
    uint64_t address;
    float red;
    float green;
    float blue;

    /*
     * The Rage 128 formats match Mesa's historical YCbCr uploads as two
     * endian-aware 16-bit words sharing Cb/Cr across each horizontal pair.
     * YVYU422 carries Y0:Cb then Y1:Cr; VYUY422 carries Cr:Y0 then Cb:Y1.
     */
    if (width < 2 || (width & 1U)) {
        return false;
    }
    address = (uint64_t)unit->offsets[offset_index] +
              ((uint64_t)(unsigned int)y * width + pair_x) * 2U;
    if (address > UINT32_MAX ||
        !rage128_3d_texture_read(s, address, bytes, sizeof(bytes))) {
        return false;
    }
    if (false) {
        first = ((uint16_t)bytes[0] << 8) | bytes[1];
        second = ((uint16_t)bytes[2] << 8) | bytes[3];
    } else {
        first = bytes[0] | ((uint16_t)bytes[1] << 8);
        second = bytes[2] | ((uint16_t)bytes[3] << 8);
    }

    if (unit->format == ATI_3D_TEX_FMT_YVYU422) {
        y0 = first >> 8;
        cb = first & 0xff;
        y1 = second >> 8;
        cr = second & 0xff;
    } else {
        y0 = first & 0xff;
        cr = first >> 8;
        y1 = second & 0xff;
        cb = second >> 8;
    }
    luma = x & 1 ? y1 : y0;

    /* ITU-R BT.601 limited-range conversion used by Mesa's r128 path. */
    red = 1.164f * ((float)luma - 16.0f) +
          1.596f * ((float)cr - 128.0f);
    green = 1.164f * ((float)luma - 16.0f) -
            0.813f * ((float)cr - 128.0f) -
            0.391f * ((float)cb - 128.0f);
    blue = 1.164f * ((float)luma - 16.0f) +
           2.018f * ((float)cb - 128.0f);
    color[0] = MIN(MAX(red, 0.0f), 255.0f);
    color[1] = MIN(MAX(green, 0.0f), 255.0f);
    color[2] = MIN(MAX(blue, 0.0f), 255.0f);
    color[3] = 255.0f;
    return true;
}

static bool rage128_3d_texture_texel(rage128_t *s,
                                 const ATI3DTextureUnit *unit,
                                 unsigned int level, int x, int y,
                                 float color[4])
{
    unsigned int width = MAX(unit->width >> level, 1U);
    unsigned int height = MAX(unit->height >> level, 1U);
    unsigned int offset_index = unit->max_level - level;
    unsigned int wrap_s = (unit->control >> ATI_3D_TEX_CLAMP_S_SHIFT) &
                          ATI_3D_TEX_CLAMP_MASK;
    unsigned int wrap_t = (unit->control >> ATI_3D_TEX_CLAMP_T_SHIFT) &
                          ATI_3D_TEX_CLAMP_MASK;
    uint8_t bytes[4] = { 0 };
    bool border_s;
    bool border_t;
    uint64_t address;
    uint32_t value = 0;

    x = rage128_3d_texture_wrap_index(x, width, wrap_s, &border_s);
    y = rage128_3d_texture_wrap_index(y, height, wrap_t, &border_t);
    if (border_s || border_t) {
        rage128_3d_unpack_argb(unit->border, color);
        return true;
    }
    if (unit->format == ATI_3D_TEX_FMT_YVYU422 ||
        unit->format == ATI_3D_TEX_FMT_VYUY422) {
        return rage128_3d_texture_yuv422_texel(s, unit, level,
                                            x, y, color);
    }
    address = (uint64_t)unit->offsets[offset_index] +
              ((uint64_t)y * width + x) * unit->bytes_per_texel;
    if (address > UINT32_MAX ||
        !rage128_3d_texture_read(s, address, bytes, unit->bytes_per_texel)) {
        return false;
    }
    for (unsigned int byte = 0; byte < unit->bytes_per_texel; byte++) {
        unsigned int shift = false ?
            (unit->bytes_per_texel - 1 - byte) * 8 : byte * 8;

        value |= (uint32_t)bytes[byte] << shift;
    }
    switch (unit->format) {
    case ATI_3D_TEX_FMT_CI4:
        rage128_3d_texture_palette_unpack(s, unit, value & 0xf, color);
        break;
    case ATI_3D_TEX_FMT_CI8:
        rage128_3d_texture_palette_unpack(s, unit, value & 0xff, color);
        break;
    case ATI_3D_TEX_FMT_CI16:
        rage128_3d_texture_palette_unpack(s, unit, value & 0xff, color);
        color[3] = (value >> 8) & 0xff;
        break;
    case ATI_3D_TEX_FMT_ARGB1555:
        color[0] = ((value >> 10) & 0x1f) * (255.0f / 31.0f);
        color[1] = ((value >> 5) & 0x1f) * (255.0f / 31.0f);
        color[2] = (value & 0x1f) * (255.0f / 31.0f);
        color[3] = value & 0x8000 ? 255.0f : 0.0f;
        break;
    case ATI_3D_TEX_FMT_RGB565:
        color[0] = ((value >> 11) & 0x1f) * (255.0f / 31.0f);
        color[1] = ((value >> 5) & 0x3f) * (255.0f / 63.0f);
        color[2] = (value & 0x1f) * (255.0f / 31.0f);
        color[3] = 255.0f;
        break;
    case ATI_3D_TEX_FMT_RGB888:
        color[0] = (value >> 16) & 0xff;
        color[1] = (value >> 8) & 0xff;
        color[2] = value & 0xff;
        color[3] = 255.0f;
        break;
    case ATI_3D_TEX_FMT_ARGB8888:
        rage128_3d_unpack_argb(value, color);
        break;
    case ATI_3D_TEX_FMT_RGB332:
        color[0] = ((value >> 5) & 7) * (255.0f / 7.0f);
        color[1] = ((value >> 2) & 7) * (255.0f / 7.0f);
        color[2] = (value & 3) * (255.0f / 3.0f);
        color[3] = 255.0f;
        break;
    case ATI_3D_TEX_FMT_Y8:
        color[0] = color[1] = color[2] = value & 0xff;
        color[3] = 255.0f;
        break;
    case ATI_3D_TEX_FMT_RGB8:
        /*
         * Mesa uses RGB8 for packed RGB332, while X Render uses the same
         * datatype for A8 masks.  Expose both interpretations: RGB from
         * the 3:3:2 fields and alpha from the full source byte.
         */
        color[0] = ((value >> 5) & 7) * (255.0f / 7.0f);
        color[1] = ((value >> 2) & 7) * (255.0f / 7.0f);
        color[2] = (value & 3) * (255.0f / 3.0f);
        color[3] = value & 0xff;
        break;
    case ATI_3D_TEX_FMT_AYUV444:
    {
        uint8_t luma = (value >> 16) & 0xff;
        uint8_t cb = (value >> 8) & 0xff;
        uint8_t cr = value & 0xff;
        float red;
        float green;
        float blue;

        /*
         * AYUV444 is laid out as A:Y:Cb:Cr.  Use the same BT.601
         * limited-range conversion as the packed YVYU/VYUY path, while
         * retaining the independently stored alpha byte.
         */
        red = 1.164f * ((float)luma - 16.0f) +
              1.596f * ((float)cr - 128.0f);
        green = 1.164f * ((float)luma - 16.0f) -
                0.813f * ((float)cr - 128.0f) -
                0.391f * ((float)cb - 128.0f);
        blue = 1.164f * ((float)luma - 16.0f) +
               2.018f * ((float)cb - 128.0f);
        color[0] = MIN(MAX(red, 0.0f), 255.0f);
        color[1] = MIN(MAX(green, 0.0f), 255.0f);
        color[2] = MIN(MAX(blue, 0.0f), 255.0f);
        color[3] = (value >> 24) & 0xff;
        break;
    }
    case ATI_3D_TEX_FMT_ARGB4444:
        color[0] = ((value >> 8) & 0xf) * 17.0f;
        color[1] = ((value >> 4) & 0xf) * 17.0f;
        color[2] = (value & 0xf) * 17.0f;
        color[3] = ((value >> 12) & 0xf) * 17.0f;
        break;
    default:
        return false;
    }
    return true;
}

static void rage128_3d_color_lerp(const float a[4], const float b[4],
                              float t, float result[4])
{
    for (unsigned int channel = 0; channel < 4; channel++) {
        result[channel] = a[channel] + (b[channel] - a[channel]) * t;
    }
}

static float rage128_3d_texture_reduce_coordinate(float coordinate,
                                               unsigned int mode)
{
    switch (mode) {
    case ATI_3D_TEX_WRAP:
        coordinate = fmodf(coordinate, 1.0f);
        return coordinate < 0.0f ? coordinate + 1.0f : coordinate;
    case ATI_3D_TEX_MIRROR:
        coordinate = fmodf(coordinate, 2.0f);
        return coordinate < 0.0f ? coordinate + 2.0f : coordinate;
    case ATI_3D_TEX_CLAMP:
        return MIN(MAX(coordinate, 0.0f), 1.0f);
    case ATI_3D_TEX_BORDER:
        /* Keep a one-texture span so linear filtering can reach the border. */
        return MIN(MAX(coordinate, -1.0f), 2.0f);
    default:
        return 0.0f;
    }
}

static bool rage128_3d_texture_level_sample(rage128_t *s,
                                        const ATI3DTextureUnit *unit,
                                        unsigned int level,
                                        float u, float v, bool linear,
                                        float color[4])
{
    unsigned int width = MAX(unit->width >> level, 1U);
    unsigned int height = MAX(unit->height >> level, 1U);
    unsigned int wrap_s = (unit->control >> ATI_3D_TEX_CLAMP_S_SHIFT) &
                          ATI_3D_TEX_CLAMP_MASK;
    unsigned int wrap_t = (unit->control >> ATI_3D_TEX_CLAMP_T_SHIFT) &
                          ATI_3D_TEX_CLAMP_MASK;

    /* Bound guest coordinates before converting the texel location to int. */
    u = rage128_3d_texture_reduce_coordinate(u, wrap_s);
    v = rage128_3d_texture_reduce_coordinate(v, wrap_t);
    if (!linear) {
        int x = floorf(u * width);
        int y = floorf(v * height);

        return rage128_3d_texture_texel(s, unit, level, x, y, color);
    }
    {
        float fx = u * width - 0.5f;
        float fy = v * height - 0.5f;
        int x0 = floorf(fx);
        int y0 = floorf(fy);
        float tx = fx - x0;
        float ty = fy - y0;
        float c00[4], c10[4], c01[4], c11[4], top[4], bottom[4];

        if (!rage128_3d_texture_texel(s, unit, level, x0, y0, c00) ||
            !rage128_3d_texture_texel(s, unit, level, x0 + 1, y0, c10) ||
            !rage128_3d_texture_texel(s, unit, level, x0, y0 + 1, c01) ||
            !rage128_3d_texture_texel(s, unit, level, x0 + 1, y0 + 1, c11)) {
            return false;
        }
        rage128_3d_color_lerp(c00, c10, tx, top);
        rage128_3d_color_lerp(c01, c11, tx, bottom);
        rage128_3d_color_lerp(top, bottom, ty, color);
        return true;
    }
}

static bool rage128_3d_texture_filter_linear(unsigned int filter)
{
    return filter == ATI_3D_TEX_MIN_LINEAR ||
           filter == ATI_3D_TEX_MIN_LINEAR_MIP_NEAREST ||
           filter == ATI_3D_TEX_MIN_LINEAR_MIP_LINEAR;
}

static bool rage128_3d_texture_sample(rage128_t *s,
                                  const ATI3DTextureUnit *unit,
                                  float u, float v, float lod,
                                  float color[4])
{
    unsigned int min_filter = (unit->control & ATI_3D_TEX_MIN_FILTER_MASK) >>
                              ATI_3D_TEX_MIN_FILTER_SHIFT;
    unsigned int mag_filter = (unit->control & ATI_3D_TEX_MAG_FILTER_MASK) >>
                              ATI_3D_TEX_MAG_FILTER_SHIFT;
    bool magnify = lod <= 0.0f;
    unsigned int level0;

    if (magnify) {
        return rage128_3d_texture_level_sample(s, unit, 0, u, v,
                                           mag_filter & 1, color);
    }
    if (!unit->mipmapped || min_filter <= ATI_3D_TEX_MIN_LINEAR) {
        return rage128_3d_texture_level_sample(
            s, unit, 0, u, v,
            rage128_3d_texture_filter_linear(min_filter), color);
    }
    lod = MIN(MAX(lod, 0.0f), (float)unit->max_level);
    if (min_filter == ATI_3D_TEX_MIN_NEAREST_MIP_LINEAR ||
        min_filter == ATI_3D_TEX_MIN_LINEAR_MIP_LINEAR) {
        float floor_lod = floorf(lod);
        unsigned int level1;
        bool linear = min_filter == ATI_3D_TEX_MIN_LINEAR_MIP_LINEAR;
        float first[4];
        float second[4];

        level0 = floor_lod;
        level1 = MIN(level0 + 1, unit->max_level);
        if (!rage128_3d_texture_level_sample(s, unit, level0, u, v,
                                         linear, first) ||
            !rage128_3d_texture_level_sample(s, unit, level1, u, v,
                                         linear, second)) {
            return false;
        }
        rage128_3d_color_lerp(first, second, lod - floor_lod, color);
        return true;
    }
    level0 = MIN((unsigned int)floorf(lod + 0.5f), unit->max_level);
    return rage128_3d_texture_level_sample(
        s, unit, level0, u, v,
        min_filter == ATI_3D_TEX_MIN_LINEAR_MIP_NEAREST, color);
}

static bool rage128_3d_texture_coordinates(const ATI3DTextureUnit *unit,
                                       const ATI3DVertex *vertices,
                                       const float *weights,
                                       unsigned int count,
                                       float *u, float *v)
{
    *u = 0.0f;
    *v = 0.0f;
    if (unit->perspective) {
        float denominator = 0.0f;

        for (unsigned int i = 0; i < count; i++) {
            float rhw = unit->coord_index ? vertices[i].rhw2 :
                                            vertices[i].rhw;

            denominator += weights[i] * rhw;
            *u += weights[i] * vertices[i].texcoord[unit->coord_index][0] *
                  rhw;
            *v += weights[i] * vertices[i].texcoord[unit->coord_index][1] *
                  rhw;
        }
        if (!isfinite(denominator) || fabsf(denominator) < 1.0e-20f) {
            return false;
        }
        *u /= denominator;
        *v /= denominator;
    } else {
        for (unsigned int i = 0; i < count; i++) {
            *u += weights[i] * vertices[i].texcoord[unit->coord_index][0];
            *v += weights[i] * vertices[i].texcoord[unit->coord_index][1];
        }
    }
    return isfinite(*u) && isfinite(*v);
}

static float rage128_3d_texture_lod(const ATI3DTextureUnit *unit,
                                const ATI3DVertex *vertices,
                                const float *weights,
                                const float *weights_dx,
                                const float *weights_dy,
                                unsigned int count)
{
    float u, v;
    float rho = 0.0f;
    float neighbor[3];

    if (count > G_N_ELEMENTS(neighbor) ||
        !rage128_3d_texture_coordinates(unit, vertices, weights, count,
                                    &u, &v)) {
        return 0.0f;
    }
    if (weights_dx) {
        float ux, vx;

        for (unsigned int i = 0; i < count; i++) {
            neighbor[i] = weights[i] + weights_dx[i];
        }
        if (rage128_3d_texture_coordinates(unit, vertices, neighbor, count,
                                       &ux, &vx)) {
            rho = MAX(rho, hypotf((ux - u) * unit->width,
                                  (vx - v) * unit->height));
        }
    }
    if (weights_dy) {
        float uy, vy;

        for (unsigned int i = 0; i < count; i++) {
            neighbor[i] = weights[i] + weights_dy[i];
        }
        if (rage128_3d_texture_coordinates(unit, vertices, neighbor, count,
                                       &uy, &vy)) {
            rho = MAX(rho, hypotf((uy - u) * unit->width,
                                  (vy - v) * unit->height));
        }
    }
    return rho > 1.0e-20f && isfinite(rho) ? log2f(rho) : 0.0f;
}

static bool rage128_3d_texture_color_factor(unsigned int selection,
                                       const ATI3DTextureUnit *unit,
                                       const float texture[4],
                                       const float constant[4],
                                       const float previous[4],
                                       float result[3])
{
    switch (selection) {
    case 0:
        memcpy(result, constant, 3 * sizeof(*result));
        return true;
    case 1:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = 255.0f - constant[channel];
        }
        return true;
    case 4:
        memcpy(result, texture, 3 * sizeof(*result));
        return true;
    case 5:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = 255.0f - texture[channel];
        }
        return true;
    case 6:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = texture[3];
        }
        return true;
    case 7:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = 255.0f - texture[3];
        }
        return true;
    case 8:
        if (!unit->secondary) {
            return false;
        }
        memcpy(result, previous, 3 * sizeof(*result));
        return true;
    default:
        return false;
    }
}

static bool rage128_3d_texture_color_input(unsigned int selection,
                                       const ATI3DTextureUnit *unit,
                                       const float interpolated[4],
                                       const float constant[4],
                                       const float previous[4],
                                       float result[3])
{
    switch (selection) {
    case 2:
        memcpy(result, constant, 3 * sizeof(*result));
        return true;
    case 3:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = constant[3];
        }
        return true;
    case 4:
        memcpy(result, interpolated, 3 * sizeof(*result));
        return true;
    case 5:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = interpolated[3];
        }
        return true;
    case 8:
        if (!unit->secondary) {
            return false;
        }
        memcpy(result, previous, 3 * sizeof(*result));
        return true;
    case 9:
        if (!unit->secondary) {
            return false;
        }
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = previous[3];
        }
        return true;
    default:
        return false;
    }
}

static bool rage128_3d_texture_alpha_factor(unsigned int selection,
                                        const float texture[4],
                                        float *result)
{
    switch (selection) {
    case 6:
        *result = texture[3];
        return true;
    case 7:
        *result = 255.0f - texture[3];
        return true;
    default:
        return false;
    }
}

static bool rage128_3d_texture_alpha_input(unsigned int selection,
                                       const ATI3DTextureUnit *unit,
                                       const float interpolated[4],
                                       const float constant[4],
                                       const float previous[4],
                                       float *result)
{
    switch (selection) {
    case 1:
        *result = constant[3];
        return true;
    case 2:
        *result = interpolated[3];
        return true;
    case 4:
        if (!unit->secondary) {
            return false;
        }
        *result = previous[3];
        return true;
    default:
        return false;
    }
}

static float rage128_3d_texture_arithmetic(unsigned int operation,
                                       float factor, float input)
{
    switch (operation) {
    case ATI_3D_COMB_COPY:
        return factor;
    case ATI_3D_COMB_COPY_INPUT:
        return input;
    case ATI_3D_COMB_MODULATE:
        return factor * input / 255.0f;
    case ATI_3D_COMB_MODULATE2X:
        return factor * input * (2.0f / 255.0f);
    case ATI_3D_COMB_MODULATE4X:
        return factor * input * (4.0f / 255.0f);
    case ATI_3D_COMB_ADD:
        return factor + input;
    case ATI_3D_COMB_ADD_SIGNED:
        return factor + input - 128.0f;
    case ATI_3D_COMB_ADD_SIGNED2X:
        return (factor + input - 128.0f) * 2.0f;
    default:
        return 0.0f;
    }
}

static bool rage128_3d_texture_combine(const ATI3DTextureContext *textures,
                                   const ATI3DTextureUnit *unit,
                                   const float interpolated[4],
                                   const float previous[4],
                                   const float texture[4],
                                   float result[4])
{
    float constant[4];
    float factor[3] = { 0 };
    float input[3] = { 0 };
    float alpha_factor = 0.0f;
    float alpha_input = 0.0f;
    unsigned int color_op = unit->combine & ATI_3D_COMB_COLOR_MASK;
    unsigned int color_factor =
        (unit->combine >> ATI_3D_COMB_COLOR_FACTOR_SHIFT) &
        ATI_3D_COMB_COLOR_FACTOR_MASK;
    unsigned int color_input =
        (unit->combine >> ATI_3D_COMB_INPUT_SHIFT) &
        ATI_3D_COMB_INPUT_MASK;
    unsigned int alpha_op =
        (unit->combine >> ATI_3D_COMB_ALPHA_SHIFT) &
        ATI_3D_COMB_ALPHA_MASK;
    unsigned int alpha_factor_selection =
        (unit->combine >> ATI_3D_COMB_ALPHA_FACTOR_SHIFT) &
        ATI_3D_COMB_ALPHA_FACTOR_MASK;
    unsigned int alpha_input_selection =
        (unit->combine >> ATI_3D_COMB_ALPHA_INPUT_SHIFT) &
        ATI_3D_COMB_ALPHA_INPUT_MASK;

    rage128_3d_unpack_argb(textures->constant_color, constant);
    if (color_op == ATI_3D_COMB_DISABLE) {
        memcpy(result, texture, 3 * sizeof(*result));
    } else {
        if (color_op != ATI_3D_COMB_COPY_INPUT &&
            !rage128_3d_texture_color_factor(color_factor, unit, texture,
                                         constant, previous, factor)) {
            return false;
        }
        if (color_op != ATI_3D_COMB_COPY &&
            !rage128_3d_texture_color_input(color_input, unit, interpolated,
                                        constant, previous, input)) {
            return false;
        }
        for (unsigned int channel = 0; channel < 3; channel++) {
            float blend;

            if (rage128_3d_texture_blend_color_pro(unit->combine)) {
                blend = texture[channel] / 255.0f;
                result[channel] = input[channel] * (1.0f - blend) +
                                  factor[channel] * blend;
                continue;
            }
            switch (color_op) {
            case ATI_3D_COMB_COPY:
            case ATI_3D_COMB_COPY_INPUT:
            case ATI_3D_COMB_MODULATE:
            case ATI_3D_COMB_MODULATE2X:
            case ATI_3D_COMB_MODULATE4X:
            case ATI_3D_COMB_ADD:
            case ATI_3D_COMB_ADD_SIGNED:
            case ATI_3D_COMB_ADD_SIGNED2X:
                result[channel] = rage128_3d_texture_arithmetic(
                    color_op, factor[channel], input[channel]);
                break;
            case ATI_3D_COMB_BLEND_VERTEX:
                blend = interpolated[3] / 255.0f;
                result[channel] = factor[channel] * blend +
                                  input[channel] * (1.0f - blend);
                break;
            case ATI_3D_COMB_BLEND_TEXTURE:
                blend = texture[3] / 255.0f;
                result[channel] = factor[channel] * blend +
                                  input[channel] * (1.0f - blend);
                break;
            case ATI_3D_COMB_BLEND_CONSTANT:
                blend = constant[3] / 255.0f;
                result[channel] = factor[channel] * blend +
                                  input[channel] * (1.0f - blend);
                break;
            case ATI_3D_COMB_BLEND_PREMULT:
                blend = texture[3] / 255.0f;
                result[channel] = factor[channel] +
                                  input[channel] * (1.0f - blend);
                break;
            case ATI_3D_COMB_BLEND_PREVIOUS:
                blend = previous[3] / 255.0f;
                result[channel] = factor[channel] * blend +
                                  input[channel] * (1.0f - blend);
                break;
            case ATI_3D_COMB_BLEND_PREMULT_INV:
                blend = texture[3] / 255.0f;
                result[channel] = factor[channel] +
                                  input[channel] * blend;
                break;
            case ATI_3D_COMB_BLEND_CONST_COLOR:
                blend = constant[channel] / 255.0f;
                result[channel] = factor[channel] * blend +
                                  input[channel] * (1.0f - blend);
                break;
            default:
                return false;
            }
        }
    }

    if (alpha_op == ATI_3D_COMB_DISABLE) {
        result[3] = texture[3];
    } else {
        if (alpha_op != ATI_3D_COMB_COPY_INPUT &&
            !rage128_3d_texture_alpha_factor(alpha_factor_selection,
                                         texture, &alpha_factor)) {
            return false;
        }
        if (alpha_op != ATI_3D_COMB_COPY &&
            !rage128_3d_texture_alpha_input(alpha_input_selection, unit,
                                        interpolated, constant, previous,
                                        &alpha_input)) {
            return false;
        }
        result[3] = rage128_3d_texture_arithmetic(alpha_op,
                                               alpha_factor, alpha_input);
    }
    for (unsigned int channel = 0; channel < 4; channel++) {
        result[channel] = MIN(MAX(result[channel], 0.0f), 255.0f);
    }
    return true;
}

static bool rage128_3d_texture_lighting_apply(
    const ATI3DTextureContext *textures, uint32_t tex_control,
    const float interpolated[4], const float texture_output[4],
    float primary_texture_alpha, float result[4])
{
    float constant[4];
    unsigned int color_function =
        (tex_control & ATI_3D_TEX_LIGHT_FN_MASK) >>
        ATI_3D_TEX_LIGHT_FN_SHIFT;
    unsigned int alpha_function =
        (tex_control & ATI_3D_ALPHA_LIGHT_FN_MASK) >>
        ATI_3D_ALPHA_LIGHT_FN_SHIFT;

    rage128_3d_unpack_argb(textures->constant_color, constant);

    switch (color_function) {
    case ATI_3D_COMB_DISABLE:
    case ATI_3D_COMB_COPY:
        memcpy(result, texture_output, 3 * sizeof(*result));
        break;
    case ATI_3D_COMB_COPY_INPUT:
        memcpy(result, interpolated, 3 * sizeof(*result));
        break;
    case ATI_3D_COMB_MODULATE:
    case ATI_3D_COMB_MODULATE2X:
    case ATI_3D_COMB_MODULATE4X:
    case ATI_3D_COMB_ADD:
    case ATI_3D_COMB_ADD_SIGNED:
    case ATI_3D_COMB_ADD_SIGNED2X:
        for (unsigned int channel = 0; channel < 3; channel++) {
            result[channel] = rage128_3d_texture_arithmetic(
                color_function, texture_output[channel],
                interpolated[channel]);
        }
        break;
    case ATI_3D_COMB_BLEND_VERTEX:
    case ATI_3D_COMB_BLEND_TEXTURE:
    case ATI_3D_COMB_BLEND_CONSTANT:
    case ATI_3D_COMB_BLEND_PREVIOUS:
    case ATI_3D_COMB_BLEND_CONST_COLOR:
        for (unsigned int channel = 0; channel < 3; channel++) {
            float blend;

            switch (color_function) {
            case ATI_3D_COMB_BLEND_VERTEX:
                blend = interpolated[3] / 255.0f;
                break;
            case ATI_3D_COMB_BLEND_TEXTURE:
                blend = primary_texture_alpha / 255.0f;
                break;
            case ATI_3D_COMB_BLEND_CONSTANT:
                blend = constant[3] / 255.0f;
                break;
            case ATI_3D_COMB_BLEND_PREVIOUS:
                blend = texture_output[3] / 255.0f;
                break;
            case ATI_3D_COMB_BLEND_CONST_COLOR:
                blend = constant[channel] / 255.0f;
                break;
            default:
                return false;
            }
            result[channel] =
                texture_output[channel] * blend +
                interpolated[channel] * (1.0f - blend);
        }
        break;
    default:
        return false;
    }

    switch (alpha_function) {
    case ATI_3D_COMB_DISABLE:
    case ATI_3D_COMB_COPY:
        result[3] = texture_output[3];
        break;
    case ATI_3D_COMB_COPY_INPUT:
        result[3] = interpolated[3];
        break;
    case ATI_3D_COMB_MODULATE:
    case ATI_3D_COMB_MODULATE2X:
    case ATI_3D_COMB_MODULATE4X:
    case ATI_3D_COMB_ADD:
    case ATI_3D_COMB_ADD_SIGNED:
        result[3] = rage128_3d_texture_arithmetic(
            alpha_function, texture_output[3], interpolated[3]);
        break;
    default:
        return false;
    }

    for (unsigned int channel = 0; channel < 4; channel++) {
        result[channel] = MIN(MAX(result[channel], 0.0f), 255.0f);
    }
    return true;
}

static bool rage128_3d_texture_apply(ATI3DFragmentContext *ctx,
                                 const ATI3DVertex *vertices,
                                 const float *weights,
                                 const float *weights_dx,
                                 const float *weights_dy,
                                 unsigned int count,
                                 float color[4], bool *discard)
{
    float interpolated[4];
    float previous[4];
    float primary_texture_alpha;

    *discard = false;
    memcpy(interpolated, color, sizeof(interpolated));
    memcpy(previous, color, sizeof(previous));
    primary_texture_alpha = interpolated[3];
    for (unsigned int index = 0; index < ATI_3D_TEX_UNIT_COUNT; index++) {
        ATI3DTextureUnit *unit = &ctx->textures.units[index];
        float texture[4];
        float result[4];
        float u;
        float v;
        float lod;

        if (!unit->enabled) {
            continue;
        }
        if (!rage128_3d_texture_coordinates(unit, vertices, weights, count,
                                        &u, &v)) {
            return false;
        }
        lod = rage128_3d_texture_lod(unit, vertices, weights,
                                 weights_dx, weights_dy, count) +
              ctx->textures.lod_bias;
        if (!rage128_3d_texture_sample(ctx->s, unit, u, v, lod, texture)) {
            return false;
        }
        if (index == 0) {
            primary_texture_alpha = texture[3];
        }
        if (index == 0 &&
            (ctx->tex_control & ATI_3D_TEX_CHROMA_KEY_ENABLE) &&
            !rage128_3d_texture_chroma_accept(ctx, texture)) {
            *discard = true;
            return true;
        }
        if (!rage128_3d_texture_combine(&ctx->textures, unit, interpolated,
                                    previous, texture, result)) {
            return false;
        }
        memcpy(previous, result, sizeof(previous));
    }
    return rage128_3d_texture_lighting_apply(
        &ctx->textures, ctx->tex_control, interpolated, previous,
        primary_texture_alpha, color);
}

static bool rage128_3d_edge_is_top_left(const ATI3DVertex *a,
                                    const ATI3DVertex *b)
{
    float dx = b->x - a->x;
    float dy = b->y - a->y;

    /* Screen-space Y grows downward and inside edges evaluate positive. */
    return dy > 0.0f || (dy == 0.0f && dx < 0.0f);
}

static bool rage128_3d_edge_accept(float value, bool top_left)
{
    return value > 0.0f || (value == 0.0f && top_left);
}

static bool rage128_3d_fragment_context_init(ATI3DFragmentContext *ctx,
                                         rage128_t *s,
                                         ATI3DSurface *surface)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->s = s;
    ctx->color_surface = surface;
    ctx->tex_control = rage128_3d_reg(s, TEX_CNTL_C);
    ctx->misc = rage128_3d_reg(s, MISC_3D_STATE_CNTL_REG);
    ctx->z_control = rage128_3d_reg(s, Z_STEN_CNTL_C);
    ctx->stencil_ref_mask = rage128_3d_reg(s, STEN_REF_MASK_C);
    ctx->fog_color = rage128_3d_reg(s, FOG_COLOR_C);
    ctx->write_mask = rage128_3d_reg(s, PLANE_3D_MASK_C) &
                      R128_REG(s, R128_DP_WRITE_MASK);
    ctx->color_dirty_start = UINT64_MAX;
    ctx->depth_dirty_start = UINT64_MAX;
    ctx->depth_enabled = ctx->tex_control & ATI_3D_TEX_Z_ENABLE;
    ctx->stencil_enabled = ctx->tex_control & ATI_3D_TEX_STENCIL_ENABLE;
    ctx->fog_enabled = ctx->tex_control & ATI_3D_FOG_ENABLE;

    if (ctx->tex_control & ATI_3D_ALPHA_ENABLE) {
        unsigned int src_factor =
            (ctx->misc >> ATI_3D_ALPHA_SRC_SHIFT) & 0xf;
        unsigned int dst_factor =
            (ctx->misc >> ATI_3D_ALPHA_DST_SHIFT) & 0xf;

        if (src_factor > 10 || dst_factor > 10 ||
            !rage128_3d_blend_equation_supported(ctx->misc)) {
            rage128_log(
                          "ATI Rage 128 3D unsupported blend equation/factor\n");
            return false;
        }
    }
    if ((ctx->depth_enabled || ctx->stencil_enabled) &&
        !rage128_3d_depth_surface(s, &ctx->depth_surface, &ctx->depth_mask,
                              &ctx->depth_function)) {
        rage128_log(
                      "ATI Rage 128 3D: invalid depth/stencil surface\n");
        return false;
    }
    if (ctx->stencil_enabled && ctx->depth_mask != 0x00ffffffU) {
        rage128_log(
                      "ATI Rage 128 stencil requires a Z24/S8 surface\n");
        return false;
    }
    if (ctx->fog_enabled && (ctx->misc & ATI_3D_FOG_SOURCE_TABLE) &&
        (rage128_3d_reg(s, ATI_3D_FOG_TABLE_START_REG) ||
         rage128_3d_reg(s, ATI_3D_FOG_TABLE_END_REG) ||
         rage128_3d_reg(s, ATI_3D_FOG_TABLE_DENSITY_REG))) {
        rage128_log(
                      "ATI Rage 128 parameterized table fog is not implemented\n");
        return false;
    }
    return rage128_3d_texture_context_init(&ctx->textures, s);
}

static bool rage128_3d_fragment(ATI3DFragmentContext *ctx, int x, int y,
                            const float src[4], float z)
{
    rage128_t *s = ctx->s;
    uint64_t color_address;
    uint64_t depth_address = 0;
    uint32_t color_pixel;
    uint32_t depth_pixel = 0;

    if (!rage128_3d_surface_address(s, ctx->color_surface, x, y,
                                &color_address)) {
        return true;
    }
    if ((ctx->tex_control & ATI_3D_ALPHA_TEST_ENABLE) &&
        !rage128_3d_alpha_compare((ctx->misc >> ATI_3D_ALPHA_TEST_SHIFT) & 7,
                              rage128_3d_clamp_channel(src[3]),
                              ctx->misc & 0xff)) {
        return true;
    }

    if (ctx->depth_enabled || ctx->stencil_enabled) {
        if (!rage128_3d_surface_address(s, &ctx->depth_surface, x, y,
                                    &depth_address)) {
            return true;
        }
        depth_pixel = rage128_3d_load_pixel(s, &ctx->depth_surface,
                                        depth_address);
    }

    if (ctx->stencil_enabled) {
        uint8_t reference =
            (ctx->stencil_ref_mask >> ATI_3D_STENCIL_REF_SHIFT) & 0xff;
        uint8_t test_mask =
            (ctx->stencil_ref_mask >> ATI_3D_STENCIL_TEST_MASK_SHIFT) &
            0xff;
        uint8_t current = depth_pixel >> 24;
        unsigned int function =
            (ctx->z_control >> ATI_3D_STENCIL_TEST_SHIFT) &
            ATI_3D_STENCIL_FIELD_MASK;

        if (!rage128_3d_alpha_compare(function, reference & test_mask,
                                  current & test_mask)) {
            rage128_3d_stencil_write(
                ctx, depth_address, depth_pixel,
                (ctx->z_control >> ATI_3D_STENCIL_FAIL_SHIFT) &
                ATI_3D_STENCIL_FIELD_MASK);
            return true;
        }
    }

    if (ctx->depth_enabled) {
        uint32_t source_depth = rage128_3d_float_depth(z, ctx->depth_mask);
        uint32_t destination_depth = depth_pixel & ctx->depth_mask;

        if (!rage128_3d_depth_compare(ctx->depth_function, source_depth,
                                  destination_depth)) {
            if (ctx->stencil_enabled) {
                rage128_3d_stencil_write(
                    ctx, depth_address, depth_pixel,
                    (ctx->z_control >> ATI_3D_STENCIL_ZFAIL_SHIFT) &
                    ATI_3D_STENCIL_FIELD_MASK);
            }
            return true;
        }
        if (ctx->tex_control & ATI_3D_TEX_Z_WRITE_ENABLE) {
            rage128_3d_store_pixel(s, &ctx->depth_surface, depth_address,
                               source_depth, ctx->depth_mask);
            ctx->depth_dirty_start = MIN(ctx->depth_dirty_start,
                                         depth_address);
            ctx->depth_dirty_end = MAX(ctx->depth_dirty_end,
                                       depth_address +
                                       ctx->depth_surface.bytes_per_pixel);
        }
    }

    if (ctx->stencil_enabled) {
        rage128_3d_stencil_write(
            ctx, depth_address, depth_pixel,
            (ctx->z_control >> ATI_3D_STENCIL_ZPASS_SHIFT) &
            ATI_3D_STENCIL_FIELD_MASK);
    }

    color_pixel = rage128_3d_load_pixel(s, ctx->color_surface, color_address);
    if (ctx->tex_control & ATI_3D_ALPHA_ENABLE) {
        float dst[4];
        float blended[4];

        rage128_3d_unpack_surface_color(ctx->color_surface, color_pixel, dst);
        if (!rage128_3d_blend(ctx->misc, src, dst, blended)) {
            rage128_log(
                          "ATI Rage 128 3D blend equation/factor is not implemented\n");
            return false;
        }
        color_pixel = rage128_3d_pack_surface_color(ctx->color_surface,
                                                ctx->tex_control, x, y,
                                                blended);
    } else {
        color_pixel = rage128_3d_pack_surface_color(ctx->color_surface,
                                                ctx->tex_control, x, y,
                                                src);
    }
    rage128_3d_store_pixel(s, ctx->color_surface, color_address, color_pixel,
                       ctx->write_mask);
    ctx->color_dirty_start = MIN(ctx->color_dirty_start, color_address);
    ctx->color_dirty_end = MAX(ctx->color_dirty_end,
                               color_address +
                               ctx->color_surface->bytes_per_pixel);
    return true;
}

static void rage128_3d_fragment_context_finish(ATI3DFragmentContext *ctx)
{
    if (ctx->color_dirty_start < ctx->color_dirty_end) {
        rage128_mark_vram_dirty(ctx->s, ctx->color_dirty_start,
                                ctx->color_dirty_end -
                                ctx->color_dirty_start);
    }
    if (ctx->depth_dirty_start < ctx->depth_dirty_end) {
        rage128_mark_vram_dirty(ctx->s, ctx->depth_dirty_start,
                                ctx->depth_dirty_end -
                                ctx->depth_dirty_start);
    }
}

static int rage128_3d_clamp_floor_to_int(float value, int lower, int upper)
{
    if (value <= lower) {
        return lower;
    }
    if (value >= upper) {
        return upper;
    }
    return floorf(value);
}

static int rage128_3d_clamp_ceil_to_int(float value, int lower, int upper)
{
    if (value <= lower) {
        return lower;
    }
    if (value >= upper) {
        return upper;
    }
    return ceilf(value);
}

static bool rage128_3d_clip_test(float p, float q, float *first, float *last)
{
    float ratio;

    if (p == 0.0f) {
        return q >= 0.0f;
    }
    ratio = q / p;
    if (p < 0.0f) {
        if (ratio > *last) {
            return false;
        }
        *first = MAX(*first, ratio);
    } else {
        if (ratio < *first) {
            return false;
        }
        *last = MIN(*last, ratio);
    }
    return true;
}

static bool rage128_3d_clip_line(const ATI3DRect *rect,
                             const ATI3DVertex *a,
                             const ATI3DVertex *b,
                             float *first, float *last)
{
    float dx = b->x - a->x;
    float dy = b->y - a->y;

    *first = 0.0f;
    *last = 1.0f;
    return rage128_3d_clip_test(-dx, a->x - rect->left, first, last) &&
           rage128_3d_clip_test(dx, rect->right - a->x, first, last) &&
           rage128_3d_clip_test(-dy, a->y - rect->top, first, last) &&
           rage128_3d_clip_test(dy, rect->bottom - a->y, first, last);
}

static bool rage128_3d_draw_line(rage128_t *s, ATI3DSurface *surface,
                             const ATI3DVertex *a, const ATI3DVertex *b);
static bool rage128_3d_draw_point(rage128_t *s, ATI3DSurface *surface,
                              const ATI3DVertex *vertex);

static bool rage128_3d_draw_triangle(rage128_t *s, ATI3DSurface *surface,
                                 ATI3DVertex v0, ATI3DVertex v1,
                                 ATI3DVertex v2)
{
    ATI3DRect scissor = rage128_3d_main_scissor(s);
    ATI3DFragmentContext fragments;
    ATI3DShadeState shade;
    ATI3DVertex original_vertices[3] = { v0, v1, v2 };
    ATI3DVertex vertices[3];
    bool result = false;
    float area = rage128_3d_edge(&v0, &v1, v2.x, v2.y);
    float min_vertex_x;
    float min_vertex_y;
    float max_vertex_x;
    float max_vertex_y;
    int min_x;
    int min_y;
    int max_x;
    int max_y;

    if (!isfinite(area)) {
        rage128_log(
                      "ATI Rage 128 3D triangle area overflowed\n");
        return false;
    }
    if (fabsf(area) < 1.0e-8f) {
        return true;
    }
    rage128_3d_shade_state_init(s, &shade);
    if (shade.mode == ATI_3D_COLOR_FLAT) {
        const ATI3DVertex *provoking =
            &original_vertices[shade.flat_last ? 2 : 0];

        for (unsigned int i = 0; i < 3; i++) {
            memcpy(original_vertices[i].color, provoking->color,
                   sizeof(provoking->color));
            memcpy(original_vertices[i].specular, provoking->specular,
                   sizeof(provoking->specular));
        }
        v0 = original_vertices[0];
        v1 = original_vertices[1];
        v2 = original_vertices[2];
    }
    switch (rage128_3d_face_mode(s, area)) {
    case ATI_3D_FACE_CULL:
        return true;
    case ATI_3D_FACE_POINTS:
        return rage128_3d_draw_point(s, surface, &v0) &&
               rage128_3d_draw_point(s, surface, &v1) &&
               rage128_3d_draw_point(s, surface, &v2);
    case ATI_3D_FACE_LINES:
        return rage128_3d_draw_line(s, surface, &v0, &v1) &&
               rage128_3d_draw_line(s, surface, &v1, &v2) &&
               rage128_3d_draw_line(s, surface, &v2, &v0);
    case ATI_3D_FACE_SOLID:
        break;
    default:
        return false;
    }
    if (area < 0.0f) {
        ATI3DVertex tmp = v1;
        v1 = v2;
        v2 = tmp;
        area = -area;
    }
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;

    min_vertex_x = MIN(v0.x, MIN(v1.x, v2.x));
    min_vertex_y = MIN(v0.y, MIN(v1.y, v2.y));
    max_vertex_x = MAX(v0.x, MAX(v1.x, v2.x));
    max_vertex_y = MAX(v0.y, MAX(v1.y, v2.y));
    if (max_vertex_x < scissor.left || max_vertex_y < scissor.top ||
        min_vertex_x > scissor.right || min_vertex_y > scissor.bottom) {
        return true;
    }
    min_x = rage128_3d_clamp_floor_to_int(min_vertex_x,
                                      scissor.left, scissor.right);
    min_y = rage128_3d_clamp_floor_to_int(min_vertex_y,
                                      scissor.top, scissor.bottom);
    max_x = rage128_3d_clamp_ceil_to_int(max_vertex_x,
                                     scissor.left, scissor.right);
    max_y = rage128_3d_clamp_ceil_to_int(max_vertex_y,
                                     scissor.top, scissor.bottom);
    if ((uint64_t)(max_x - min_x + 1) *
        (uint64_t)(max_y - min_y + 1) > ATI_3D_MAX_RASTER_PIXELS) {
        rage128_log(
                      "ATI Rage 128 3D triangle raster area is too large\n");
        return false;
    }

    if (!rage128_3d_fragment_context_init(&fragments, s, surface)) {
        return false;
    }
    bool edge0_top_left = rage128_3d_edge_is_top_left(&v1, &v2);
    bool edge1_top_left = rage128_3d_edge_is_top_left(&v2, &v0);
    bool edge2_top_left = rage128_3d_edge_is_top_left(&v0, &v1);
    const float weights_dx[3] = {
        (v2.y - v1.y) / area,
        (v0.y - v2.y) / area,
        (v1.y - v0.y) / area,
    };
    const float weights_dy[3] = {
        (v1.x - v2.x) / area,
        (v2.x - v0.x) / area,
        (v0.x - v1.x) / area,
    };

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float sample_x = x + 0.5f;
            float sample_y = y + 0.5f;
            float w0 = rage128_3d_edge(&v1, &v2, sample_x, sample_y);
            float w1 = rage128_3d_edge(&v2, &v0, sample_x, sample_y);
            float w2 = rage128_3d_edge(&v0, &v1, sample_x, sample_y);
            float src[4];
            float specular[4];
            float z;

            if (!isfinite(w0) || !isfinite(w1) || !isfinite(w2)) {
                rage128_log(
                              "ATI Rage 128 3D barycentric overflow\n");
                goto out;
            }
            if (!rage128_3d_edge_accept(w0, edge0_top_left) ||
                !rage128_3d_edge_accept(w1, edge1_top_left) ||
                !rage128_3d_edge_accept(w2, edge2_top_left) ||
                !rage128_3d_aux_scissor_pass(s, x, y)) {
                continue;
            }
            w0 /= area;
            w1 /= area;
            w2 /= area;
            {
                const float weights[3] = { w0, w1, w2 };
                bool discard;

                rage128_3d_shade_sample(&shade,
                                    shade.mode == ATI_3D_COLOR_FLAT ?
                                    original_vertices : vertices,
                                    weights, 3, src, specular);
                if (!rage128_3d_texture_apply(&fragments, vertices, weights,
                                          weights_dx, weights_dy, 3, src,
                                          &discard)) {
                    goto out;
                }
                if (discard) {
                    continue;
                }
                rage128_3d_shade_add_specular(&shade, specular, src);
                rage128_3d_apply_fog(&fragments, specular,
                                 v0.z * w0 + v1.z * w1 + v2.z * w2,
                                 src);
            }
            z = v0.z * w0 + v1.z * w1 + v2.z * w2;
            if (!rage128_3d_fragment(&fragments, x, y, src, z)) {
                goto out;
            }
        }
    }
    result = true;

out:
    rage128_3d_fragment_context_finish(&fragments);
    if (result) {
        s->pm4.primitives_drawn++;
    }
    return result;
}

static bool rage128_3d_draw_line(rage128_t *s, ATI3DSurface *surface,
                             const ATI3DVertex *a, const ATI3DVertex *b)
{
    ATI3DRect scissor = rage128_3d_main_scissor(s);
    ATI3DFragmentContext fragments;
    ATI3DShadeState shade;
    ATI3DVertex vertices[2] = { *a, *b };
    bool result = false;
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float first;
    float last;
    float clipped_dx;
    float clipped_dy;
    float span;
    unsigned int steps;

    if (!isfinite(dx) || !isfinite(dy)) {
        rage128_log(
                      "ATI Rage 128 3D line delta overflowed\n");
        return false;
    }
    if (!rage128_3d_clip_line(&scissor, a, b, &first, &last)) {
        return true;
    }
    clipped_dx = dx * (last - first);
    clipped_dy = dy * (last - first);
    span = MAX(fabsf(clipped_dx), fabsf(clipped_dy));
    if (!isfinite(span) ||
        span > (float)ATI_3D_MAX_RASTER_PIXELS) {
        rage128_log(
                      "ATI Rage 128 3D line is too large\n");
        return false;
    }
    steps = MAX((unsigned int)ceilf(span), 1U);
    if (!rage128_3d_fragment_context_init(&fragments, s, surface)) {
        return false;
    }
    rage128_3d_shade_state_init(s, &shade);
    for (unsigned int i = 0; i <= steps; i++) {
        float local = (float)i / steps;
        float t = first + (last - first) * local;
        float sample_x = a->x + dx * t;
        float sample_y = a->y + dy * t;
        int x = sample_x + 0.5f;
        int y = sample_y + 0.5f;
        float color[4];
        float specular[4];
        float z;

        if (!rage128_3d_aux_scissor_pass(s, x, y)) {
            continue;
        }
        {
            const float delta = (last - first) / steps;
            const float weights[2] = { 1.0f - t, t };
            const float weights_dx[2] = { -delta, delta };
            bool discard;

            rage128_3d_shade_sample(&shade, vertices, weights, 2,
                                color, specular);
            if (!rage128_3d_texture_apply(&fragments, vertices, weights,
                                      weights_dx, NULL, 2, color,
                                      &discard)) {
                goto out;
            }
            if (discard) {
                continue;
            }
            rage128_3d_shade_add_specular(&shade, specular, color);
            rage128_3d_apply_fog(&fragments, specular,
                             a->z * (1.0f - t) + b->z * t, color);
        }
        z = a->z * (1.0f - t) + b->z * t;
        if (!rage128_3d_fragment(&fragments, x, y, color, z)) {
            goto out;
        }
    }
    result = true;

out:
    rage128_3d_fragment_context_finish(&fragments);
    if (result) {
        s->pm4.primitives_drawn++;
    }
    return result;
}

static bool rage128_3d_draw_point(rage128_t *s, ATI3DSurface *surface,
                              const ATI3DVertex *vertex)
{
    ATI3DRect scissor = rage128_3d_main_scissor(s);
    ATI3DFragmentContext fragments;
    ATI3DShadeState shade;
    const float weight = 1.0f;
    float color[4];
    float specular[4];
    bool discard;
    bool result;
    int x;
    int y;

    if (vertex->x < scissor.left || vertex->x > scissor.right ||
        vertex->y < scissor.top || vertex->y > scissor.bottom) {
        return true;
    }
    x = vertex->x + 0.5f;
    y = vertex->y + 0.5f;
    if (!rage128_3d_aux_scissor_pass(s, x, y)) {
        return true;
    }
    if (!rage128_3d_fragment_context_init(&fragments, s, surface)) {
        return false;
    }
    rage128_3d_shade_state_init(s, &shade);
    rage128_3d_shade_sample(&shade, vertex, &weight, 1, color, specular);
    if (!rage128_3d_texture_apply(&fragments, vertex, &weight,
                              NULL, NULL, 1, color, &discard)) {
        result = false;
    } else if (discard) {
        result = true;
    } else {
        rage128_3d_shade_add_specular(&shade, specular, color);
        rage128_3d_apply_fog(&fragments, specular, vertex->z, color);
        result = rage128_3d_fragment(&fragments, x, y, color, vertex->z);
    }
    rage128_3d_fragment_context_finish(&fragments);
    if (result) {
        s->pm4.primitives_drawn++;
    }
    return result;
}

static bool rage128_3d_prepare_draw(rage128_t *s, uint32_t format,
                                uint32_t vc_cntl, ATI3DSurface *surface,
                                unsigned int *primitive,
                                unsigned int *count,
                                unsigned int *stride)
{
    uint32_t tex_control = rage128_3d_reg(s, TEX_CNTL_C);
    uint32_t master = rage128_3d_reg(s, DP_GUI_MASTER_CNTL_C);
    uint32_t pitch_offset = rage128_3d_reg(s, DST_PITCH_OFFSET_C);

    *primitive = vc_cntl & ATI_3D_VC_PRIM_MASK;
    *count = vc_cntl >> ATI_3D_VC_COUNT_SHIFT;
    *stride = rage128_3d_vertex_stride(format);
    if (!*count || *count > ATI_3D_MAX_VERTICES || !*stride) {
        rage128_log(
                      "ATI Rage 128 3D invalid vertex count/format\n");
        return false;
    }
    if ((tex_control & ATI_3D_FOG_ENABLE) &&
        !(rage128_3d_reg(s, MISC_3D_STATE_CNTL_REG) &
          ATI_3D_FOG_SOURCE_TABLE) &&
        !(format & (ATI_3D_VERTEX_SPEC_F |
                    ATI_3D_VERTEX_SPEC_FRGB))) {
        rage128_log(
                      "ATI Rage 128 vertex fog requires SPEC_F vertices\n");
        return false;
    }
    if (tex_control & (ATI_3D_TEXMAP_ENABLE |
                       ATI_3D_SEC_TEXMAP_ENABLE)) {
        ATI3DTextureContext validation;

        if (!rage128_3d_texture_context_init(&validation, s)) {
            return false;
        }
        for (unsigned int index = 0;
             index < ATI_3D_TEX_UNIT_COUNT; index++) {
            ATI3DTextureUnit *unit = &validation.units[index];
            uint32_t required_format = unit->coord_index ?
                                       ATI_3D_VERTEX_S2T2 :
                                       ATI_3D_VERTEX_ST;

            if (unit->enabled && !(format & required_format)) {
                rage128_log(
                              "ATI Rage 128 texture unit %u is missing "
                              "S/T vertices\n", index);
                return false;
            }
        }
    }
    if (!rage128_3d_decode_surface(master, pitch_offset, surface)) {
        rage128_log(
                      "ATI Rage 128 3D invalid destination surface\n");
        return false;
    }
    if (surface->datatype == DST_8BPP) {
        rage128_log(
                      "ATI Rage 128 8-bpp color-indexed 3D is not "
                      "implemented\n");
        return false;
    }
    if ((tex_control & ATI_3D_ALPHA_ENABLE) &&
        (!rage128_3d_blend_equation_supported(
             rage128_3d_reg(s, MISC_3D_STATE_CNTL_REG)) ||
         ((rage128_3d_reg(s, MISC_3D_STATE_CNTL_REG) >>
           ATI_3D_ALPHA_SRC_SHIFT) & 0xf) > 10 ||
         ((rage128_3d_reg(s, MISC_3D_STATE_CNTL_REG) >>
           ATI_3D_ALPHA_DST_SHIFT) & 0xf) > 10)) {
        rage128_log(
                      "ATI Rage 128 3D blend state is not implemented\n");
        return false;
    }
    return true;
}

static void rage128_3d_vertex_apply_window(const rage128_t *s,
                                       ATI3DVertex *vertex)
{
    uint32_t window_offset = rage128_3d_reg(s, WINDOW_XY_OFFSET);

    vertex->x += sextract32(window_offset, 20, 12);
    vertex->y += sextract32(window_offset, 4, 12);
}

static bool rage128_3d_render_vertices(rage128_t *s, ATI3DSurface *surface,
                                   unsigned int primitive,
                                   ATI3DVertex *vertices,
                                   unsigned int count)
{
    switch (primitive) {
    case ATI_3D_PRIM_POINT:
        for (unsigned int i = 0; i < count; i++) {
            if (!rage128_3d_draw_point(s, surface, &vertices[i])) {
                return false;
            }
        }
        return true;
    case ATI_3D_PRIM_LINE:
    case ATI_3D_PRIM_POLYLINE:
        for (unsigned int i = 1; i < count; i +=
             primitive == ATI_3D_PRIM_LINE ? 2 : 1) {
            if (!rage128_3d_draw_line(s, surface, &vertices[i - 1],
                                  &vertices[i])) {
                return false;
            }
        }
        return true;
    case ATI_3D_PRIM_TRI_LIST:
    case ATI_3D_PRIM_TRI_TYPE2:
        for (unsigned int i = 2; i < count; i += 3) {
            if (!rage128_3d_draw_triangle(s, surface, vertices[i - 2],
                                      vertices[i - 1], vertices[i])) {
                return false;
            }
        }
        return true;
    case ATI_3D_PRIM_TRI_FAN:
        for (unsigned int i = 2; i < count; i++) {
            if (!rage128_3d_draw_triangle(s, surface, vertices[0],
                                      vertices[i - 1], vertices[i])) {
                return false;
            }
        }
        return true;
    case ATI_3D_PRIM_TRI_STRIP:
        for (unsigned int i = 2; i < count; i++) {
            ATI3DVertex a = vertices[i - 2];
            ATI3DVertex b = vertices[i - 1];

            if (i & 1) {
                ATI3DVertex tmp = a;
                a = b;
                b = tmp;
            }
            if (!rage128_3d_draw_triangle(s, surface, a, b, vertices[i])) {
                return false;
            }
        }
        return true;
    default:
        rage128_log(
                      "ATI Rage 128 3D primitive type %u is not implemented\n",
                      primitive);
        return false;
    }
}

bool rage128_3d_draw_indexed(rage128_t *s, uint32_t address,
                         uint32_t size, uint32_t format,
                         uint32_t vc_cntl,
                         const uint32_t *index_words,
                         unsigned int index_dwords)
{
    unsigned int primitive;
    unsigned int count;
    unsigned int stride;
    unsigned int walk = vc_cntl & ATI_3D_VC_WALK_MASK;
    ATI3DSurface surface;
    ATI3DVertex *vertices = NULL;
    bool result = false;

    if (walk != ATI_3D_WALK_LIST && walk != ATI_3D_WALK_IND) {
        rage128_log(
                      "ATI Rage 128 indexed 3D vertex walk mode 0x%x is "
                      "not implemented\n", walk);
        return false;
    }
    if (!rage128_3d_prepare_draw(s, format, vc_cntl, &surface, &primitive,
                             &count, &stride)) {
        return false;
    }
    if (walk == ATI_3D_WALK_LIST && size && size < count) {
        /* The historical list packet carries the vertex count twice. */
        rage128_log(
                      "ATI Rage 128 3D vertex packet count is inconsistent\n");
        return false;
    }
    if (walk == ATI_3D_WALK_IND &&
        (!index_words || index_dwords < (count + 1U) / 2U)) {
        rage128_log(
                      "ATI Rage 128 3D indexed packet is truncated\n");
        return false;
    }

    vertices = calloc(count, sizeof(ATI3DVertex));
    for (unsigned int i = 0; i < count; i++) {
        unsigned int vertex_index = i;

        if (walk == ATI_3D_WALK_IND) {
            uint32_t packed = index_words[i / 2];

            vertex_index = (i & 1) ? packed >> 16 : packed & 0xffff;
            if (size && vertex_index >= size) {
                rage128_log(
                              "ATI Rage 128 3D vertex index %u exceeds %u\n",
                              vertex_index, size);
                goto out;
            }
        }
        if (!rage128_3d_read_vertex(s,
                                address + (uint32_t)vertex_index * stride * 4,
                                format, stride, &vertices[i])) {
            goto out;
        }
        rage128_3d_vertex_apply_window(s, &vertices[i]);
    }
    result = rage128_3d_render_vertices(s, &surface, primitive, vertices, count);

out:
    free(vertices);
    return result;
}

bool rage128_3d_draw_inline(rage128_t *s, uint32_t format,
                        uint32_t vc_cntl, const uint32_t *vertex_words,
                        unsigned int vertex_dwords)
{
    unsigned int primitive;
    unsigned int count;
    unsigned int stride;
    unsigned int walk = vc_cntl & ATI_3D_VC_WALK_MASK;
    ATI3DSurface surface;
    ATI3DVertex *vertices;
    bool result = false;

    if (walk != ATI_3D_WALK_RING) {
        rage128_log(
                      "ATI Rage 128 inline 3D vertex walk mode 0x%x is "
                      "not implemented\n", walk);
        return false;
    }
    if (!rage128_3d_prepare_draw(s, format, vc_cntl, &surface, &primitive,
                             &count, &stride)) {
        return false;
    }
    if (!vertex_words || vertex_dwords != (uint64_t)count * stride) {
        rage128_log(
                      "ATI Rage 128 inline 3D vertex payload is malformed\n");
        return false;
    }

    vertices = calloc(count, sizeof(ATI3DVertex));
    for (unsigned int i = 0; i < count; i++) {
        if (!rage128_3d_decode_vertex(&vertex_words[i * stride], format,
                                  stride, &vertices[i])) {
            goto out;
        }
        rage128_3d_vertex_apply_window(s, &vertices[i]);
    }
    result = rage128_3d_render_vertices(s, &surface, primitive, vertices, count);

out:
    free(vertices);
    return result;
}
