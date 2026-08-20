"""
Patch SysConfig-generated ti_enet_init.c for tiarmclang compatibility.

SysConfig 1.28.0 generates a static struct initializer that copies a
const-qualified global (gEnetRmResCfg) into a field of gEnetCpswCfg:

    static Cpsw_Cfg gEnetCpswCfg = {
        ...
        .resCfg = gEnetRmResCfg,   <-- NOT a compile-time constant in C
        ...
    };

tiarmclang (and all C compilers) reject this because const variables are not
compile-time constants in C (unlike C++).  This is a known SysConfig code-gen
issue.  The fix: remove the field from the static initializer and assign it
at the top of EnetApp_getCpswCfg() so it is always set before any caller
accesses resCfg.
"""

import sys
import re

def patch(path):
    with open(path, "r") as f:
        src = f.read()

    original = src

    # 1. Remove ".resCfg = gEnetRmResCfg," from the static initializer.
    #    The line looks like (indented with spaces):
    #        .resCfg = gEnetRmResCfg,
    src = re.sub(
        r'[ \t]*\.resCfg\s*=\s*gEnetRmResCfg\s*,[ \t]*\n',
        '    /* .resCfg set at runtime by EnetApp_getCpswCfg() */\n',
        src,
        count=1,
    )

    # 2. Inside EnetApp_getCpswCfg(), after "pCpswCfg = &gEnetCpswCfg;"
    #    add the runtime assignment.
    #    We look for the exact assignment line and insert after it.
    MARKER = 'pCpswCfg = &gEnetCpswCfg;'
    INSERT = '\n        pCpswCfg->resCfg = gEnetRmResCfg;  /* patched */'
    if MARKER in src and 'pCpswCfg->resCfg = gEnetRmResCfg;' not in src:
        src = src.replace(MARKER, MARKER + INSERT, 1)

    if src == original:
        print(f"patch_ti_enet_init.py: {path} — nothing to patch (already patched or pattern not found)")
        return

    with open(path, "w") as f:
        f.write(src)
    print(f"patch_ti_enet_init.py: {path} patched successfully")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path/to/ti_enet_init.c>")
        sys.exit(1)
    patch(sys.argv[1])
