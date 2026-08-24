# ATI Rage 128 ROM images

These firmware images were supplied by repository owner Bryce Lanham for Rage
128 emulation development and are retained byte-for-byte in this development
fork. `SHA256SUMS` is the canonical byte-integrity record; `verify.py` also
checks each PCI data structure, code type, declared image size, checksum, and
known PLL table values.

Run the complete local audit from this directory with:

```sh
python3 verify.py
sha256sum -c SHA256SUMS
```

## `113-57403-102.rom`

- Original upload: `vbios.rom`
- Board: ATI Xpert 128
- Image type: x86 PCI option ROM
- PCI identity: `1002:5245`
- Subsystem identity: `1002:0008`
- ATI part number: `113-57403-102`
- File and declared image size: 36,864 bytes
- BIOS: `BK1.0.13`, dated 1999-06-21
- PLL table: 90.00 MHz XCLK; 29.50 MHz reference; divider 65;
  125–250 MHz PLL range
- SHA-256: `91837fab2f2a71df54d3f031fd15bc5ed658b148d2df806b05e30f370fe60afe`
- Use: default firmware for the 86Box Xpert 128 RE/PCI profile

## `113-53008-100.rom`

- Original upload: `rage128-pci.rom`
- Board: ATI All-in-Wonder 128, display function
- Image type: x86 PCI option ROM
- PCI identity: `1002:5245`
- Subsystem identity: `1002:0068`
- ATI part number: `113-53008-100`
- File size: 65,536 bytes; PCIR-declared executable image: 45,056 bytes
- BIOS: `BK1.0.12`, dated 1999-04-07
- PLL table: 90.00 MHz XCLK; 28.64 MHz reference; divider 63;
  125–250 MHz PLL range
- SHA-256: `9100ea06532a08e50afd496ddddb943ff69e9b8dbf9b03bd375a4de17a85741b`
- Use: default firmware for the 86Box All-in-Wonder 128 RE/PCI display
  profile

## `Rage128PCI-Mac-OpenFirmware.ROM`

- Original upload: `rage-128-pci-mac-6a496d313fec8451228008.ROM`
- Board family: ATI Rage 128 GL/RE PCI
- Image type: Macintosh Open Firmware option ROM, PCI code type 1
- PCI identity: `1002:5245`
- Open Firmware identity: `ATY,Rage128`
- File size: 32,768 bytes
- PCIR-declared image size: 75,776 bytes, larger than the supplied dump
- SHA-256: `e8d6c829b22e4a4a9900b761b9394e5e2dddba594c78a473647e7b0df9e492d1`
- Use: reference and cross-emulator research only; the current 86Box PC
  device does not execute PowerPC Open Firmware option ROMs

These are historical firmware binaries. Their inclusion does not imply an
additional copyright, patent, or redistribution license beyond whatever rights
apply to the original hardware and firmware.
