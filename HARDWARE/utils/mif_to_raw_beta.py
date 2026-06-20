#!/usr/bin/env python3
"""
beta_q.mif -> .raw cru: 1280 * uint16 LE (mem_beta, CoProcessor.v).

Exemplo:
  python utils/mif_to_raw_beta.py memorias/beta_q.mif -o tutoriais/store_img_api/beta.raw
"""

import argparse
import struct
import sys
from pathlib import Path

_UTILS = Path(__file__).resolve().parent
if str(_UTILS) not in sys.path:
    sys.path.insert(0, str(_UTILS))

from mif_to_raw_win import parse_mif


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mif_path", help="ex.: memorias/beta_q.mif")
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args()

    words = parse_mif(args.mif_path)
    depth = len(words)
    if depth != 1280:
        print(
            f"# aviso: profundidade MIF={depth}, RTL mem_beta espera 1280",
            file=sys.stderr,
        )

    blob = struct.pack("<" + "H" * depth, *words)
    with open(args.output, "wb") as fo:
        fo.write(blob)
    print(args.output, len(blob), "bytes", "=", depth, "uint16 LE")


if __name__ == "__main__":
    main()
