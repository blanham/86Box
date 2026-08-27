#!/usr/bin/env python3
"""Preserve diagnostic argument use in the standalone Rage 128 harness.

The generated 86Box compatibility layer suppresses QEMU diagnostics while
running the strict standalone renderer tests.  An empty variadic macro also
makes every diagnostic-only local look unused to -Werror.  Route those calls
through a no-op inline function instead, so arguments remain semantically
consumed without producing output.
"""

from pathlib import Path

renderer = Path("src/video/vid_ati_rage128_3d.c")
text = renderer.read_text(encoding="utf-8")

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

if anchor in text:
    text = text.replace(anchor, replacement, 1)
elif replacement not in text:
    raise SystemExit("unexpected Rage 128 texture diagnostic anchor")

renderer.write_text(text, encoding="utf-8")

compat = Path("src/video/vid_ati_rage128_qemu_compat.h")
text = compat.read_text(encoding="utf-8")
old = '''#ifdef RAGE128_STANDALONE_TEST
# define rage128_log(...) do { } while (0)
#else
# define rage128_log(...) pclog(__VA_ARGS__)
#endif
'''
new = '''#ifdef RAGE128_STANDALONE_TEST
static inline void
rage128_standalone_log(const char *format, ...)
{
    (void) format;
}
# define rage128_log(...) rage128_standalone_log(__VA_ARGS__)
#else
# define rage128_log(...) pclog(__VA_ARGS__)
#endif
'''

if text.count(old) != 1:
    raise SystemExit("unexpected Rage 128 standalone logging boundary")

compat.write_text(text.replace(old, new, 1), encoding="utf-8")
