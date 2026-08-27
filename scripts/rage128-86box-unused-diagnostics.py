#!/usr/bin/env python3
"""Silence QEMU diagnostic-only locals after adapting the Rage 128 renderer.

The 86Box port intentionally strips qemu_log_mask() calls.  Keep the source
shape close to the validated QEMU renderer while explicitly consuming locals
that only supplied text to those diagnostics.
"""

from pathlib import Path

path = Path("src/video/vid_ati_rage128_3d.c")
text = path.read_text(encoding="utf-8")

anchor = '''        enable_bit = ATI_3D_TEXMAP_ENABLE;
        name = "primary";
    }
    unit->enabled = rage128_3d_reg(s, R128_TEX_CNTL_C) & enable_bit;
'''
replacement = '''        enable_bit = ATI_3D_TEXMAP_ENABLE;
        name = "primary";
    }
    (void) name;
    unit->enabled = rage128_3d_reg(s, R128_TEX_CNTL_C) & enable_bit;
'''

if text.count(anchor) != 1:
    raise SystemExit("unexpected Rage 128 texture diagnostic anchor")

path.write_text(text.replace(anchor, replacement, 1), encoding="utf-8")
