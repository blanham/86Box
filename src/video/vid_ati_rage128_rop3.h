/*
 * This file is generated from the validated QEMU Rage 128 reference
 * implementation at commit c7061f0a9fedd6994e7403ec844c978c29f59320.
 * Emulator-boundary adaptations are performed by the 86Box source
 * transform; guest-visible PM4/3D behavior remains shared.
 */

/*
 * ATI 2D engine ROP3 helpers
 *
 * Copyright (c) 2026 Bryce Lanham
 *
 * This work is licensed under the GNU GPL license version 2 or later.
 */

#ifndef VID_ATI_RAGE128_ROP3_H
#define VID_ATI_RAGE128_ROP3_H

#include <stdbool.h>
#include <stdint.h>

/*
 * A Windows/ATI ROP3 byte is an eight-entry truth table indexed as PSD:
 * pattern is the high bit, source the middle bit, and destination the low bit.
 * Evaluate all bits of the operands in parallel.
 */
static inline uint32_t ati_rop3_eval(uint8_t rop, uint32_t pat,
                                    uint32_t src, uint32_t dst)
{
    uint32_t out = 0;

    for (unsigned int i = 0; i < 8; i++) {
        uint32_t term = (i & 4) ? pat : ~pat;

        term &= (i & 2) ? src : ~src;
        term &= (i & 1) ? dst : ~dst;
        out |= term & -(uint32_t)((rop >> i) & 1);
    }
    return out;
}

static inline uint32_t ati_rop3_apply_mask(uint8_t rop, uint32_t pat,
                                           uint32_t src, uint32_t dst,
                                           uint32_t write_mask)
{
    uint32_t result = ati_rop3_eval(rop, pat, src, dst);

    return (result & write_mask) | (dst & ~write_mask);
}

static inline bool ati_rop3_uses_pattern(uint8_t rop)
{
    return ((rop ^ (rop >> 4)) & 0x0f) != 0;
}

static inline bool ati_rop3_uses_source(uint8_t rop)
{
    return ((rop ^ (rop >> 2)) & 0x33) != 0;
}

static inline bool ati_rop3_uses_destination(uint8_t rop)
{
    return ((rop ^ (rop >> 1)) & 0x55) != 0;
}

#endif /* VID_ATI_RAGE128_ROP3_H */
