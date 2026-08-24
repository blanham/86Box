/*
 * QEMU-to-86Box compatibility boundary for the shared Rage 128 PM4/3D core.
 */
#ifndef VID_ATI_RAGE128_QEMU_COMPAT_H
#define VID_ATI_RAGE128_QEMU_COMPAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <86box/86box.h>
#include <86box/mem.h>
#include <86box/pci.h>

#include "vid_ati_rage128.h"
#include "vid_ati_rage128_qemu_regs.h"
#include "vid_ati_rage128_rop3.h"

#ifndef BIT
# define BIT(n) (UINT32_C(1) << (n))
#endif
#ifndef BIT_ULL
# define BIT_ULL(n) (UINT64_C(1) << (n))
#endif
#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define G_N_ELEMENTS(a) (sizeof(a) / sizeof((a)[0]))
#define ATI_PM4_PACKET_MAX_DWORDS R128_PM4_PACKET_MAX_DWORDS
#define g_new0(type, count) ((type *) calloc((count), sizeof(type)))

#ifndef PCI_COMMAND_MASTER
# define PCI_COMMAND_MASTER PCI_COMMAND_L_BM
#endif

static inline uint32_t rage128_extract32(uint32_t value,
                                         unsigned int start,
                                         unsigned int length)
{
    uint32_t mask = length == 32 ? UINT32_MAX :
                    ((UINT32_C(1) << length) - 1U);
    return (value >> start) & mask;
}

static inline uint32_t rage128_deposit32(uint32_t value,
                                         unsigned int start,
                                         unsigned int length,
                                         uint32_t field)
{
    uint32_t mask = length == 32 ? UINT32_MAX :
                    ((UINT32_C(1) << length) - 1U);
    mask <<= start;
    return (value & ~mask) | ((field << start) & mask);
}

#define extract32 rage128_extract32
#define deposit32 rage128_deposit32

static inline uint32_t rage128_bswap32(uint32_t value)
{
    return ((value & UINT32_C(0x000000ff)) << 24) |
           ((value & UINT32_C(0x0000ff00)) << 8) |
           ((value & UINT32_C(0x00ff0000)) >> 8) |
           ((value & UINT32_C(0xff000000)) >> 24);
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define le32_to_cpu(v) rage128_bswap32(v)
# define cpu_to_le32(v) rage128_bswap32(v)
#else
# define le32_to_cpu(v) (v)
# define cpu_to_le32(v) (v)
#endif

static inline void *rage128_memdup(const void *source, size_t length)
{
    void *copy = malloc(length);
    if (copy)
        memcpy(copy, source, length);
    return copy;
}

static inline uint16_t rage128_pci_command(const rage128_t *dev)
{
    return (uint16_t)dev->pci_regs[PCI_REG_COMMAND_L] |
           (uint16_t)dev->pci_regs[PCI_REG_COMMAND_H] << 8;
}

static inline bool rage128_dma_read(rage128_t *dev, uint32_t address,
                                    void *buffer, size_t length)
{
    (void) dev;
    if (length > INT32_MAX || (uint64_t)address + length > UINT64_C(0x100000000))
        return false;
    mem_read_phys(buffer, address, (int)length);
    return true;
}

static inline bool rage128_dma_write(rage128_t *dev, uint32_t address,
                                     const void *buffer, size_t length)
{
    (void) dev;
    if (length > INT32_MAX || (uint64_t)address + length > UINT64_C(0x100000000))
        return false;
    mem_write_phys((void *)buffer, address, (int)length);
    return true;
}

#ifdef RAGE128_STANDALONE_TEST
static inline void
rage128_standalone_log(const char *format, ...)
{
    (void) format;
}
# define rage128_log(...) rage128_standalone_log(__VA_ARGS__)
#else
# define rage128_log(...) pclog(__VA_ARGS__)
#endif



#ifndef R128_GMC_BRUSH_SOLID_COLOR
#define R128_GMC_BRUSH_SOLID_COLOR  0x000000d0U
#define R128_GMC_DST_32BPP          0x00000600U
#define R128_GMC_SRC_DATATYPE_COLOR 0x00003000U
#define R128_ROP3_P                 0x00f00000U
#endif


static inline int32_t
rage128_sextract32(uint32_t value, unsigned int start,
                   unsigned int length)
{
    return (int32_t)(value << (32U - start - length)) >>
           (32U - length);
}
#define sextract32 rage128_sextract32

#endif /* VID_ATI_RAGE128_QEMU_COMPAT_H */
