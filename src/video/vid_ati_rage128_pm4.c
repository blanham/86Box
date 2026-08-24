/*
 * This file is generated from the validated QEMU Rage 128 reference
 * implementation at commit c7061f0a9fedd6994e7403ec844c978c29f59320.
 * Emulator-boundary adaptations are performed by the 86Box source
 * transform; guest-visible PM4/3D behavior remains shared.
 */

/*
 * QEMU ATI Rage 128 PM4/CCE emulation
 *
 * Copyright (c) 2026 Bryce Lanham
 *
 * This work is licensed under the GNU GPL license version 2 or later.
 */

#include "vid_ati_rage128_qemu_compat.h"

#define ATI_PM4_MAX_EXEC_DWORDS (1U << 20)
#define ATI_PM4_MAX_SURFACE_PIXELS (16U * 1024U * 1024U)
#define ATI_PM4_MAX_INDIRECT_DEPTH 4
#define ATI_PM4_AGP_VIRT_BASE UINT32_C(0x02000000)
#define ATI_PM4_AGP_VIRT_END  UINT32_C(0x04000000)
#define ATI_PM4_NATIVE_AGP_SENTINEL UINT32_C(0x00000001)
#define ATI_PM4_GART_PAGE_STORE_MASK UINT32_C(0xfffff001)
#define ATI_PM4_BM_CHUNK_0_VAL UINT32_C(0x00000a18)
#define ATI_PM4_BM_FORCE_TO_PCI_MASK \
    (BIT(21) | BIT(22) | BIT(23))
#define ATI_PM4_AGP_APER_SIZE_MASK UINT32_C(0x0000003f)

static bool rage128_pm4_bus_master_mode(uint32_t mode)
{
    switch (mode) {
    case 2: /* 192 bus-master */
    case 4: /* 128 bus-master + 64 indirect */
    case 6: /* 64 bus-master + 128 indirect */
    case 8: /* 64 bus-master + 64 VC + 64 indirect */
        return true;
    default:
        return false;
    }
}

static uint32_t rage128_pm4_fifo_size(uint32_t buffer_cntl)
{
    switch ((buffer_cntl >> 28) & 0xf) {
    case 1: /* 192 PIO */
    case 2: /* 192 bus-master */
        return 192;
    case 3: /* 128 PIO + 64 indirect */
    case 4: /* 128 bus-master + 64 indirect */
        return 128;
    case 5: /* 64 PIO + 128 indirect */
    case 6: /* 64 bus-master + 128 indirect */
    case 7: /* 64 PIO + 64 VC + 64 indirect */
    case 8: /* 64 bus-master + 64 VC + 64 indirect */
    case 15:
        return 64;
    default:
        return 0;
    }
}

static bool rage128_pm4_bus_master_enabled(const rage128_t *s)
{
    uint16_t command = rage128_pci_command(s);

    return (command & PCI_COMMAND_MASTER) &&
           !(s->pm4.bus_cntl & BUS_MASTER_DIS);
}

static void rage128_pm4_fault(rage128_t *s, const char *message)
{
    (void) message;
    if (!s->pm4.fault) {
        rage128_log( "ATI Rage 128 PM4: %s\n", message);
    }
    s->pm4.fault = true;
}

static bool rage128_pm4_dma_read_direct(rage128_t *s,
                                            uint32_t address,
                                            void *buffer, size_t length)
{
    bool result;

    result = rage128_dma_read(s, address, buffer, length);
    if (result != true) {
        rage128_pm4_fault(s, "DMA read failed");
    }
    return result;
}

static bool rage128_pm4_dma_write_direct(rage128_t *s,
                                             uint32_t address,
                                             const void *buffer,
                                             size_t length)
{
    bool result;

    result = rage128_dma_write(s, address, buffer, length);
    if (result != true) {
        rage128_pm4_fault(s, "DMA write failed");
    }
    return result;
}

/*
 * The Rage 128 command stream reserves 0x02000000..0x03ffffff for host
 * memory.  PCI cards translate it through PCI_GART_PAGE.  The AGP Xorg path
 * instead writes PCI_GART_PAGE=1, clears the BM force-to-PCI bits, and
 * programs AGP_BASE/AGP_CNTL; 86Box's chipset then exposes the translated AGP
 * aperture as an ordinary physical-memory mapping.  Preserve both paths at
 * the emulator boundary instead of forcing AGP boards through a fake PCI
 * page table.
 */
static uint32_t rage128_pm4_native_agp_size(const rage128_t *s)
{
    switch (R128_REG(s, AGP_CNTL) & ATI_PM4_AGP_APER_SIZE_MASK) {
    case 0x00:
        return UINT32_C(256) * 1024 * 1024;
    case 0x20:
        return UINT32_C(128) * 1024 * 1024;
    case 0x30:
        return UINT32_C(64) * 1024 * 1024;
    case 0x38:
        return UINT32_C(32) * 1024 * 1024;
    case 0x3c:
        return UINT32_C(16) * 1024 * 1024;
    case 0x3e:
        return UINT32_C(8) * 1024 * 1024;
    case 0x3f:
        return UINT32_C(4) * 1024 * 1024;
    default:
        return 0;
    }
}

static bool rage128_pm4_native_agp_mode(const rage128_t *s)
{
    return s->is_agp &&
           s->pm4.pci_gart_page == ATI_PM4_NATIVE_AGP_SENTINEL &&
           !(R128_REG(s, ATI_PM4_BM_CHUNK_0_VAL) &
             ATI_PM4_BM_FORCE_TO_PCI_MASK);
}

static bool rage128_pm4_native_agp_translate(rage128_t *s,
                                              uint32_t address,
                                              uint32_t *physical,
                                              size_t *chunk)
{
    uint32_t aperture_size = rage128_pm4_native_agp_size(s);
    uint32_t aperture_base;
    uint32_t window_offset = address - ATI_PM4_AGP_VIRT_BASE;
    uint64_t translated;
    size_t in_page;

    if (!aperture_size) {
        rage128_pm4_fault(s, "invalid native AGP aperture size");
        return false;
    }
    if (R128_REG(s, AGP_APER_OFFSET)) {
        rage128_pm4_fault(s,
                          "nonzero native AGP aperture offset is unsupported");
        return false;
    }
    if (window_offset >= aperture_size) {
        rage128_pm4_fault(s, "native AGP access exceeds the aperture");
        return false;
    }

    aperture_base = R128_REG(s, AGP_BASE) & ~(aperture_size - 1U);
    if (!aperture_base) {
        rage128_pm4_fault(s, "native AGP access without an aperture base");
        return false;
    }
    translated = (uint64_t)aperture_base + window_offset;
    if (translated > UINT32_MAX) {
        rage128_pm4_fault(s, "native AGP address overflows 32 bits");
        return false;
    }

    *physical = translated;
    in_page = 0x1000 - (address & 0xfff);
    *chunk = MIN(*chunk, in_page);
    *chunk = MIN(*chunk, (size_t)(aperture_size - window_offset));
    return true;
}

static bool rage128_pm4_dma_rw(rage128_t *s, uint32_t address,
                                  void *buffer, size_t length, bool write)
{
    uint8_t *bytes = buffer;

    if (!rage128_pm4_bus_master_enabled(s)) {
        rage128_pm4_fault(s, "bus-master access while PCI mastering is disabled");
        return false;
    }

    while (length) {
        uint32_t physical = address;
        size_t chunk = length;

        if (address >= ATI_PM4_AGP_VIRT_BASE &&
            address < ATI_PM4_AGP_VIRT_END) {
            uint32_t page_index;
            uint32_t page_entry;
            uint32_t page_table_address;
            uint32_t page_table_base;
            size_t in_page;

            if (s->pm4.pci_gart_page == ATI_PM4_NATIVE_AGP_SENTINEL) {
                if (!rage128_pm4_native_agp_mode(s)) {
                    rage128_pm4_fault(s,
                                      "native AGP DMA requested on an unavailable path");
                    return false;
                }
                if (!rage128_pm4_native_agp_translate(s, address,
                                                       &physical, &chunk)) {
                    return false;
                }
            } else {
                page_table_base = s->pm4.pci_gart_page &
                                  UINT32_C(0xfffff000);
                if (!page_table_base) {
                    rage128_pm4_fault(s,
                                      "AGP/PCI-GART access without a page table");
                    return false;
                }
                page_index = (address - ATI_PM4_AGP_VIRT_BASE) >> 12;
                page_table_address = page_table_base + page_index * 4;
                if (rage128_pm4_dma_read_direct(s, page_table_address,
                                                &page_entry,
                                                sizeof(page_entry)) != true) {
                    return false;
                }
                page_entry = le32_to_cpu(page_entry);
                physical = (page_entry & UINT32_C(0xfffff000)) |
                           (address & UINT32_C(0x00000fff));
                if (!(page_entry & UINT32_C(0xfffff000))) {
                    rage128_pm4_fault(s, "unmapped PCI-GART page");
                    return false;
                }
                in_page = 0x1000 - (address & 0xfff);
                chunk = MIN(chunk, in_page);
            }
        }

        if (write) {
            if (rage128_pm4_dma_write_direct(s, physical, bytes, chunk) !=
                true) {
                return false;
            }
        } else if (rage128_pm4_dma_read_direct(s, physical, bytes, chunk) !=
                   true) {
            return false;
        }

        address += chunk;
        bytes += chunk;
        length -= chunk;
    }
    return true;
}

static bool rage128_pm4_dma_read(rage128_t *s, uint32_t address,
                                    void *buffer, size_t length)
{
    return rage128_pm4_dma_rw(s, address, buffer, length, false);
}

static bool rage128_pm4_dma_write(rage128_t *s, uint32_t address,
                                     const void *buffer, size_t length)
{
    return rage128_pm4_dma_rw(s, address, (void *)buffer, length, true);
}

bool rage128_pm4_read_guest(rage128_t *s, uint32_t address,
                         void *buffer, size_t length)
{
    return rage128_pm4_dma_read(s, address, buffer, length) == true;
}

static bool rage128_pm4_read_dword(rage128_t *s, uint32_t address,
                               uint32_t *value)
{
    uint32_t raw;

    if (rage128_pm4_dma_read(s, address, &raw, sizeof(raw)) != true) {
        return false;
    }
    *value = le32_to_cpu(raw);
    return true;
}

static bool rage128_pm4_write_dword(rage128_t *s, uint32_t address,
                                uint32_t value)
{
    uint32_t raw = cpu_to_le32(value);

    return rage128_pm4_dma_write(s, address, &raw, sizeof(raw)) == true;
}

static bool rage128_pm4_packet3(rage128_t *s, uint32_t opcode,
                            const uint32_t *payload, unsigned int count)
{
    switch (opcode) {
    case R128_PM4_PACKET3_NOP:
        return true;

    case R128_PM4_CNTL_PAINT_MULTI:
        if (count < 5) {
            rage128_pm4_fault(s, "short PAINT_MULTI packet");
            return false;
        }
        if ((uint64_t)(payload[4] >> 16) *
            (payload[4] & UINT32_C(0xffff)) >
            ATI_PM4_MAX_SURFACE_PIXELS) {
            rage128_pm4_fault(s,
                          "PAINT_MULTI rectangle exceeds pixel work limit");
            return false;
        }
        if (payload[0] & R128_GMC_WR_MSK_DIS) {
            rage128_mmio_write_reg(s, DP_WRITE_MASK, UINT32_MAX);
        }
        if (payload[0] & R128_GMC_AUX_CLIP_DIS) {
            rage128_mmio_write_reg(s, AUX_SC_CNTL, 0);
        }
        /*
         * PAINT_MULTI carries an explicit Rage 128 pitch/offset surface.
         * Route every surface through the shared software surface helper;
         * the legacy 2D register path uses the implicit/default surface and
         * therefore loses this packet-local destination on non-tiled clears.
         */
        if (rage128_3d_surface_fill(s, payload[0], payload[1], payload[2],
                                payload[3], payload[4])) {
            return true;
        }
        rage128_mmio_write_reg(s, DP_GUI_MASTER_CNTL, payload[0]);
        rage128_mmio_write_reg(s, DST_PITCH_OFFSET, payload[1]);
        rage128_mmio_write_reg(s, DP_BRUSH_FRGD_CLR, payload[2]);
        rage128_mmio_write_reg(s, DST_X_Y, payload[3]);
        rage128_mmio_write_reg(s, DST_WIDTH_HEIGHT, payload[4]);
        return true;

    case R128_PM4_CNTL_BITBLT_MULTI:
        if (count < 6) {
            rage128_pm4_fault(s, "short BITBLT_MULTI packet");
            return false;
        }
        if ((uint64_t)(payload[5] >> 16) *
            (payload[5] & UINT32_C(0xffff)) >
            ATI_PM4_MAX_SURFACE_PIXELS) {
            rage128_pm4_fault(s,
                          "BITBLT_MULTI rectangle exceeds pixel work limit");
            return false;
        }
        if (payload[0] & R128_GMC_WR_MSK_DIS) {
            rage128_mmio_write_reg(s, DP_WRITE_MASK, UINT32_MAX);
        }
        if (payload[0] & R128_GMC_AUX_CLIP_DIS) {
            rage128_mmio_write_reg(s, AUX_SC_CNTL, 0);
        }
        rage128_mmio_write_reg(s, DP_GUI_MASTER_CNTL, payload[0]);
        rage128_mmio_write_reg(s, SRC_PITCH_OFFSET, payload[1]);
        rage128_mmio_write_reg(s, DST_PITCH_OFFSET, payload[2]);
        rage128_mmio_write_reg(s, SRC_X_Y, payload[3]);
        rage128_mmio_write_reg(s, DST_X_Y, payload[4]);
        rage128_mmio_write_reg(s, DST_WIDTH_HEIGHT, payload[5]);
        return true;

    case R128_PM4_CNTL_HOSTDATA_BLT:
    {
        unsigned int data_words;

        if (count < 7) {
            rage128_pm4_fault(s, "short HOSTDATA_BLT packet");
            return false;
        }
        if (payload[6] != count - 7) {
            rage128_pm4_fault(s, "HOSTDATA_BLT payload length is inconsistent");
            return false;
        }
        data_words = payload[6];
        if (payload[0] & R128_GMC_WR_MSK_DIS) {
            rage128_mmio_write_reg(s, DP_WRITE_MASK, UINT32_MAX);
        }
        if (payload[0] & R128_GMC_AUX_CLIP_DIS) {
            rage128_mmio_write_reg(s, AUX_SC_CNTL, 0);
        }
        rage128_mmio_write_reg(s, DP_GUI_MASTER_CNTL, payload[0]);
        rage128_mmio_write_reg(s, DST_PITCH_OFFSET, payload[1]);
        /* HOSTDATA_BLT uses Y:X and HEIGHT:WIDTH field ordering. */
        rage128_mmio_write_reg(s, DST_Y_X, payload[4]);
        rage128_mmio_write_reg(s, DST_HEIGHT_WIDTH, payload[5]);
        if (!s->host_data.active && data_words) {
            rage128_pm4_fault(s, "HOSTDATA_BLT did not start a host transfer");
            return false;
        }
        for (unsigned int i = 0; i < data_words; i++) {
            uint32_t reg = i + 1 == data_words ? HOST_DATA_LAST :
                         HOST_DATA0 + (i & 3) * 4;
            rage128_mmio_write_reg(s, reg, payload[7 + i]);
        }
        if (s->host_data.active) {
            rage128_host_data_finish(s);
        }
        return true;
    }

    case R128_PM4_CNTL_LOAD_PALETTE:
        if (count < 1 ||
            !rage128_3d_load_palette(s, payload[0], &payload[1], count - 1)) {
            rage128_pm4_fault(s, "invalid LOAD_PALETTE packet");
            return false;
        }
        return true;

    case R128_PM4_3D_RNDR_GEN_INDX_PRIM:
        if (count < 4) {
            rage128_pm4_fault(s, "short indexed 3D primitive packet");
            return false;
        }
        if (!rage128_3d_draw_indexed(s, payload[0], payload[1],
                                 payload[2], payload[3],
                                 count > 4 ? &payload[4] : NULL,
                                 count > 4 ? count - 4 : 0)) {
            rage128_pm4_fault(s, "indexed 3D primitive command failed");
            return false;
        }
        return true;

    case R128_PM4_3D_RNDR_GEN_PRIM:
        if (count < 2) {
            rage128_pm4_fault(s, "short inline 3D primitive packet");
            return false;
        }
        if (!rage128_3d_draw_inline(s, payload[0], payload[1],
                                count > 2 ? &payload[2] : NULL,
                                count > 2 ? count - 2 : 0)) {
            rage128_pm4_fault(s, "inline 3D primitive command failed");
            return false;
        }
        return true;

    default:
        rage128_log(
                      "ATI Rage 128 PM4 packet3 opcode 0x%04x is not implemented\n",
                      opcode);
        rage128_pm4_fault(s, "unsupported packet3 opcode");
        return false;
    }
}

static bool rage128_pm4_dispatch_packet(rage128_t *s,
                                    const uint32_t *packet,
                                    unsigned int dwords)
{
    uint32_t header = packet[0];
    uint32_t type = header & R128_PM4_PACKET_TYPE_MASK;
    unsigned int payload_count = dwords - 1;

    s->pm4.packets_executed++;
    switch (type) {
    case R128_PM4_PACKET0:
    {
        uint32_t reg = (header & R128_PM4_PACKET0_REG_MASK) << 2;
        bool one_reg = header & R128_PM4_PACKET0_ONE_REG_WR;

        for (unsigned int i = 0; i < payload_count; i++) {
            rage128_mmio_write_reg(s, one_reg ? reg : reg + i * 4, packet[i + 1]);
            if (s->pm4.fault) {
                return false;
            }
        }
        return true;
    }

    case R128_PM4_PACKET1:
        if (payload_count != 2) {
            rage128_pm4_fault(s, "malformed packet1");
            return false;
        }
        rage128_mmio_write_reg(s,
                         (header & R128_PM4_PACKET1_REG0_MASK) << 2,
                         packet[1]);
        rage128_mmio_write_reg(s,
                         ((header & R128_PM4_PACKET1_REG1_MASK) >> 11) << 2,
                         packet[2]);
        return !s->pm4.fault;

    case R128_PM4_PACKET2:
        return true;

    case R128_PM4_PACKET3:
        return rage128_pm4_packet3(s, header & R128_PM4_PACKET3_OPCODE_MASK,
                               &packet[1], payload_count);

    default:
        rage128_pm4_fault(s, "unknown packet type");
        return false;
    }
}

static bool rage128_pm4_feed_word(rage128_t *s, uint32_t word)
{
    rage128_pm4_t *pm4 = &s->pm4;

    if (!pm4->packet_used) {
        uint32_t type = word & R128_PM4_PACKET_TYPE_MASK;
        unsigned int payload_count;

        switch (type) {
        case R128_PM4_PACKET0:
        case R128_PM4_PACKET3:
            payload_count = ((word & R128_PM4_PACKET_COUNT_MASK) >> 16) + 1;
            pm4->packet_needed = payload_count + 1;
            break;
        case R128_PM4_PACKET1:
            pm4->packet_needed = 3;
            break;
        case R128_PM4_PACKET2:
            pm4->packet_needed = 1;
            break;
        default:
            rage128_pm4_fault(s, "invalid packet header");
            return false;
        }
        if (pm4->packet_needed > ATI_PM4_PACKET_MAX_DWORDS) {
            rage128_pm4_fault(s, "packet exceeds the command-size limit");
            pm4->packet_needed = 0;
            return false;
        }
    }

    pm4->packet[pm4->packet_used++] = word;
    pm4->dwords_executed++;
    if (pm4->packet_used == pm4->packet_needed) {
        unsigned int dwords = pm4->packet_used;
        uint32_t *packet = rage128_memdup(pm4->packet,
                                     dwords * sizeof(*packet));
        bool result;

        pm4->packet_used = 0;
        pm4->packet_needed = 0;
        result = rage128_pm4_dispatch_packet(s, packet, dwords);
        free(packet);
        return result;
    }
    return true;
}

static bool rage128_pm4_execute_stream(rage128_t *s, uint32_t address,
                                   unsigned int dwords)
{
    if (++s->pm4.indirect_depth > ATI_PM4_MAX_INDIRECT_DEPTH) {
        rage128_pm4_fault(s, "indirect-buffer nesting is too deep");
        s->pm4.indirect_depth--;
        return false;
    }
    if (dwords > ATI_PM4_MAX_EXEC_DWORDS) {
        rage128_pm4_fault(s, "indirect buffer is too large");
        s->pm4.indirect_depth--;
        return false;
    }

    if (s->pm4.packet_used) {
        rage128_pm4_fault(s, "indirect buffer entered in the middle of a packet");
        s->pm4.indirect_depth--;
        return false;
    }
    for (unsigned int i = 0; i < dwords && !s->pm4.fault; i++) {
        uint32_t word;

        if (!rage128_pm4_read_dword(s, address + (uint32_t)i * 4, &word) ||
            !rage128_pm4_feed_word(s, word)) {
            s->pm4.indirect_depth--;
            return false;
        }
    }
    if (s->pm4.packet_used) {
        s->pm4.packet_used = 0;
        s->pm4.packet_needed = 0;
        rage128_pm4_fault(s, "indirect buffer ended in the middle of a packet");
    }
    s->pm4.indirect_depth--;
    return !s->pm4.fault;
}

static void rage128_pm4_execute_indirect(rage128_t *s)
{
    if (!s->pm4.iw_indsize || s->pm4.fault) {
        return;
    }
    rage128_pm4_execute_stream(s, s->pm4.iw_indoff, s->pm4.iw_indsize);
    s->pm4.iw_indsize = 0;
}

void rage128_pm4_run(rage128_t *s)
{
    rage128_pm4_t *pm4 = &s->pm4;
    uint32_t size_l2qw = pm4->buffer_cntl & 0x3f;
    uint32_t mode = (pm4->buffer_cntl >> 28) & 0xf;
    uint32_t ring_dwords;
    uint32_t mask;
    unsigned int executed = 0;

    if (pm4->executing || pm4->fault ||
        !rage128_pm4_bus_master_mode(mode) ||
        !(pm4->micro_cntl & R128_PM4_MICRO_FREERUN)) {
        return;
    }
    /* PIO modes consume PM4_FIFO_DATA; only BM modes fetch a DMA ring. */
    if (!pm4->microcode_loaded) {
        rage128_pm4_fault(s, "ring started before the CCE microcode was loaded");
        return;
    }
    if (size_l2qw > 24) {
        rage128_pm4_fault(s, "invalid ring size");
        return;
    }
    ring_dwords = 2U << size_l2qw;
    mask = ring_dwords - 1;
    pm4->rptr &= mask;
    pm4->wptr &= mask;

    pm4->executing = true;
    pm4->busy = true;
    while (pm4->rptr != pm4->wptr && !pm4->fault) {
        uint32_t word;
        uint32_t address;

        if (++executed > ATI_PM4_MAX_EXEC_DWORDS) {
            rage128_pm4_fault(s, "ring execution exceeded the safety limit");
            break;
        }
        address = pm4->buffer_offset + (uint32_t)pm4->rptr * 4;
        if (!rage128_pm4_read_dword(s, address, &word) ||
            !rage128_pm4_feed_word(s, word)) {
            break;
        }
        pm4->rptr = (pm4->rptr + 1) & mask;
    }
    if (pm4->rptr_addr) {
        rage128_pm4_write_dword(s, pm4->rptr_addr, pm4->rptr);
    }
    pm4->busy = false;
    pm4->executing = false;
}

uint32_t rage128_pm4_gui_status(const rage128_t *s)
{
    return 64 | (s->pm4.busy ? GUI_ACTIVE : 0);
}

static uint32_t rage128_pm4_status(const rage128_t *s)
{
    uint32_t value = rage128_pm4_fifo_size(s->pm4.buffer_cntl);

    if (s->pm4.busy) {
        value |= R128_PM4_BUSY | R128_PM4_GUI_ACTIVE;
    }
    return value;
}

void rage128_pm4_reset(rage128_t *s, bool full)
{
    rage128_pm4_t saved = { 0 };

    if (!full) {
        /*
         * SOFT_RESET_GUI stops and drains the command processor, but the
         * historical DRM driver programs the GART and ring location before
         * issuing that reset and does not program them again afterwards.
         * Preserve the CCE configuration and uploaded microcode while
         * clearing live execution, pointers, packet assembly, and faults.
         */
        saved.bus_cntl = s->pm4.bus_cntl;
        saved.pci_gart_page = s->pm4.pci_gart_page;
        saved.buffer_offset = s->pm4.buffer_offset;
        saved.buffer_cntl = s->pm4.buffer_cntl;
        saved.buffer_wm_cntl = s->pm4.buffer_wm_cntl;
        saved.rptr_addr = s->pm4.rptr_addr;
        memcpy(saved.microcode, s->pm4.microcode,
               sizeof(saved.microcode));
        saved.microcode_words = s->pm4.microcode_words;
        saved.microcode_loaded = s->pm4.microcode_loaded;
    }
    memset(&s->pm4, 0, sizeof(s->pm4));
    if (!full) {
        s->pm4.bus_cntl = saved.bus_cntl;
        s->pm4.pci_gart_page = saved.pci_gart_page;
        s->pm4.buffer_offset = saved.buffer_offset;
        s->pm4.buffer_cntl = saved.buffer_cntl;
        s->pm4.buffer_wm_cntl = saved.buffer_wm_cntl;
        s->pm4.rptr_addr = saved.rptr_addr;
        memcpy(s->pm4.microcode, saved.microcode,
               sizeof(s->pm4.microcode));
        s->pm4.microcode_words = saved.microcode_words;
        s->pm4.microcode_loaded = saved.microcode_loaded;
        if (s->pm4.rptr_addr && rage128_pm4_bus_master_enabled(s)) {
            uint32_t zero = 0;

            /* Keep the driver-visible ring-head shadow coherent with reset. */
            rage128_pm4_dma_rw(s, s->pm4.rptr_addr, &zero, sizeof(zero), true);
        }
    }
    /*
     * The GUI clip registers reset to the full drawable coordinate range.
     * Leaving them at zero silently clips every legacy 2D operation after a
     * device or GUI reset, including PM4 BITBLT and HOSTDATA packets that
     * intentionally use the default clip rectangle.
     */
    rage128_accel_reset(s);
    rage128_3d_reset(s);
}

bool rage128_pm4_mm_read(rage128_t *s, uint32_t addr, unsigned int size,
                     uint32_t *value)
{
    rage128_pm4_t *pm4 = &s->pm4;
    uint32_t val;

    if (rage128_3d_mm_read(s, addr, size, value)) {
        return true;
    }
    if (size != 4 || (addr & 3)) {
        return false;
    }

    switch (addr) {
    case BUS_CNTL:
        val = pm4->bus_cntl;
        break;
    case PCI_GART_PAGE:
        val = pm4->pci_gart_page;
        break;
    case PM4_BUFFER_OFFSET:
        val = pm4->buffer_offset;
        break;
    case PM4_BUFFER_CNTL:
        val = pm4->buffer_cntl;
        break;
    case PM4_BUFFER_WM_CNTL:
        val = pm4->buffer_wm_cntl;
        break;
    case PM4_BUFFER_DL_RPTR_ADDR:
        val = pm4->rptr_addr;
        break;
    case PM4_BUFFER_DL_RPTR:
        val = pm4->rptr;
        break;
    case PM4_BUFFER_DL_WPTR:
        val = pm4->wptr;
        break;
    case PM4_VC_FPU_SETUP:
        val = pm4->vc_fpu_setup;
        break;
    case PM4_IW_INDOFF:
        val = pm4->iw_indoff;
        break;
    case PM4_IW_INDSIZE:
        val = pm4->iw_indsize;
        break;
    case PM4_STAT:
        val = rage128_pm4_status(s);
        break;
    case PM4_MICROCODE_ADDR:
        val = pm4->microcode_addr;
        break;
    case PM4_MICROCODE_RADDR:
        val = pm4->microcode_raddr;
        break;
    case PM4_MICROCODE_DATAH:
        val = pm4->microcode[pm4->microcode_raddr & 0xff] >> 32;
        break;
    case PM4_MICROCODE_DATAL:
        val = pm4->microcode[pm4->microcode_raddr & 0xff];
        pm4->microcode_raddr = (pm4->microcode_raddr + 1) & 0xff;
        break;
    case PM4_BUFFER_ADDR:
        val = pm4->buffer_offset + pm4->rptr * 4;
        break;
    case PM4_MICRO_CNTL:
        val = pm4->micro_cntl;
        break;
    case PC_GUI_CTLSTAT:
        val = pm4->pc_gui_ctlstat & ~PC_BUSY;
        break;
    case WAIT_UNTIL:
        val = pm4->wait_until;
        break;
    default:
        return false;
    }
    *value = val;
    return true;
}

bool rage128_pm4_mm_write(rage128_t *s, uint32_t addr, uint64_t data,
                      unsigned int size)
{
    rage128_pm4_t *pm4 = &s->pm4;
    uint32_t value = data;

    if (rage128_3d_mm_write(s, addr, data, size)) {
        return true;
    }
    if (size != 4 || (addr & 3)) {
        return false;
    }

    switch (addr) {
    case BUS_CNTL:
        pm4->bus_cntl = value;
        return true;
    case PCI_GART_PAGE:
        /* Bit zero selects the host-chipset AGP aperture. */
        pm4->pci_gart_page = value & ATI_PM4_GART_PAGE_STORE_MASK;
        return true;
    case PM4_BUFFER_OFFSET:
        pm4->buffer_offset = value & ~7U;
        return true;
    case PM4_BUFFER_CNTL:
        pm4->buffer_cntl = value;
        rage128_pm4_run(s);
        return true;
    case PM4_BUFFER_WM_CNTL:
        pm4->buffer_wm_cntl = value;
        return true;
    case PM4_BUFFER_DL_RPTR_ADDR:
        pm4->rptr_addr = value & ~3U;
        return true;
    case PM4_BUFFER_DL_RPTR:
        pm4->rptr = value & 0x00ffffffU;
        pm4->packet_used = 0;
        pm4->packet_needed = 0;
        if (pm4->rptr_addr && rage128_pm4_bus_master_enabled(s)) {
            uint32_t shadow = cpu_to_le32(pm4->rptr);

            rage128_pm4_dma_rw(s, pm4->rptr_addr, &shadow, sizeof(shadow), true);
        }
        return true;
    case PM4_BUFFER_DL_WPTR:
        pm4->wptr = value & 0x00ffffffU;
        rage128_pm4_run(s);
        return true;
    case PM4_VC_FPU_SETUP:
        pm4->vc_fpu_setup = value;
        return true;
    case PM4_IW_INDOFF:
        pm4->iw_indoff = value & ~7U;
        return true;
    case PM4_IW_INDSIZE:
        pm4->iw_indsize = value & 0x00ffffffU;
        rage128_pm4_execute_indirect(s);
        return true;
    case PM4_MICROCODE_ADDR:
        pm4->microcode_addr = value & 0xff;
        pm4->microcode_words = 0;
        pm4->microcode_loaded = false;
        return true;
    case PM4_MICROCODE_RADDR:
        pm4->microcode_raddr = value & 0xff;
        return true;
    case PM4_MICROCODE_DATAH:
        pm4->microcode_datah = value;
        return true;
    case PM4_MICROCODE_DATAL:
        pm4->microcode[pm4->microcode_addr & 0xff] =
            ((uint64_t)pm4->microcode_datah << 32) | value;
        pm4->microcode_addr = (pm4->microcode_addr + 1) & 0xff;
        if (pm4->microcode_words < 256) {
            pm4->microcode_words++;
        }
        pm4->microcode_loaded = pm4->microcode_words == 256;
        return true;
    case PM4_MICRO_CNTL:
        pm4->micro_cntl = value & R128_PM4_MICRO_FREERUN;
        rage128_pm4_run(s);
        return true;
    case PM4_FIFO_DATA_EVEN:
    case PM4_FIFO_DATA_ODD:
        if (!rage128_pm4_feed_word(s, value)) {
            return true;
        }
        return true;
    case PC_GUI_CTLSTAT:
        pm4->pc_gui_ctlstat = value & UINT32_C(0x000000ff);
        return true;
    case WAIT_UNTIL:
        pm4->wait_until = value;
        return true;
    default:
        return false;
    }
}
