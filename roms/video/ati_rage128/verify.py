#!/usr/bin/env python3
"""Verify the ATI Rage 128 firmware images retained by this development fork."""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
EXPECTED = {
    "113-57403-102.rom": {
        "size": 36_864,
        "sha256": "91837fab2f2a71df54d3f031fd15bc5ed658b148d2df806b05e30f370fe60afe",
        "device": 0x5245,
        "code_type": 0,
        "image_size": 36_864,
        "checksum": True,
        "pll": (9000, 2950, 65, 12500, 25000),
    },
    "113-53008-100.rom": {
        "size": 65_536,
        "sha256": "9100ea06532a08e50afd496ddddb943ff69e9b8dbf9b03bd375a4de17a85741b",
        "device": 0x5245,
        "code_type": 0,
        "image_size": 45_056,
        "checksum": True,
        "pll": (9000, 2864, 63, 12500, 25000),
    },
    "Rage128PCI-Mac-OpenFirmware.ROM": {
        "size": 32_768,
        "sha256": "e8d6c829b22e4a4a9900b761b9394e5e2dddba594c78a473647e7b0df9e492d1",
        "device": 0x5245,
        "code_type": 1,
        "image_size": 75_776,
        "checksum": False,
        "pll": None,
    },
}


def le16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def le32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def inspect(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    if data[:2] != b"\x55\xaa":
        raise ValueError("missing 55 AA signature")

    pcir = le16(data, 0x18)
    if pcir + 0x18 > len(data) or data[pcir : pcir + 4] != b"PCIR":
        raise ValueError("missing or truncated PCIR structure")

    image_size = le16(data, pcir + 0x10) * 512
    declared_checksum = (
        image_size <= len(data) and sum(data[:image_size]) % 256 == 0
    )

    pll = None
    if data[pcir + 0x14] == 0 and len(data) >= 0x4A:
        bios_header = le16(data, 0x48)
        if bios_header + 0x32 <= len(data):
            pll_info = le16(data, bios_header + 0x30)
            if pll_info + 0x1A <= len(data):
                pll = (
                    le16(data, pll_info + 0x08),
                    le16(data, pll_info + 0x0E),
                    le16(data, pll_info + 0x10),
                    le32(data, pll_info + 0x12),
                    le32(data, pll_info + 0x16),
                )

    return {
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "vendor": le16(data, pcir + 4),
        "device": le16(data, pcir + 6),
        "code_type": data[pcir + 0x14],
        "image_size": image_size,
        "checksum": declared_checksum,
        "pll": pll,
    }


def main() -> None:
    for name, expected in EXPECTED.items():
        actual = inspect(ROOT / name)
        assert actual["size"] == expected["size"], (name, actual)
        assert actual["sha256"] == expected["sha256"], (name, actual)
        assert actual["vendor"] == 0x1002, (name, actual)
        assert actual["device"] == expected["device"], (name, actual)
        assert actual["code_type"] == expected["code_type"], (name, actual)
        assert actual["image_size"] == expected["image_size"], (name, actual)
        assert actual["checksum"] is expected["checksum"], (name, actual)
        assert actual["pll"] == expected["pll"], (name, actual)
        print(
            f"{name}: OK; {actual['size']} bytes; "
            f"1002:{actual['device']:04x}; code={actual['code_type']}; "
            f"image={actual['image_size']}"
        )


if __name__ == "__main__":
    main()
