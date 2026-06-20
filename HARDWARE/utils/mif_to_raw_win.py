#!/usr/bin/env python3
"""
Converte W_in_q.mif (Altera HEX, 100352 palavras 16-bit)
para ficheiro .raw cru: 100352 * 2 bytes, uint16 LE (ARM igual).

Exemplo:
  python utils/mif_to_raw_win.py memorias/W_in_q.mif -o tutoriais/store_img_api/W_in.raw
"""

import argparse
import re
import struct
import sys

LINE_RE = re.compile(r"^\s*(\d+)\s*:\s*([\da-fA-F]+)\s*;\s*")


def read_header_depth_width(lines):
    depth = width = None
    for ln in lines:
        if ln.strip().startswith("DEPTH"):
            depth = int(ln.split("=")[1].strip().rstrip(";"))
        elif ln.strip().startswith("WIDTH"):
            width = int(ln.split("=")[1].strip().rstrip(";"))
        if depth is not None and width is not None:
            break
    if depth is None or width != 16:
        raise ValueError(f"DEPTH/WIDTH invalidos: depth={depth} width={width}")
    return depth


def parse_mif(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        txt = f.read()
    lines = txt.splitlines()
    depth = read_header_depth_width(lines)

    vals = [0] * depth
    in_content = False
    skipped = []
    for i, ln in enumerate(lines):
        s = ln.strip()
        if s.upper().startswith("CONTENT"):
            continue
        if s.upper().startswith("BEGIN"):
            in_content = True
            continue
        if s.upper().startswith("END"):
            break
        if not in_content:
            continue
        # ignora "|" typo em algumas visualizacoes
        ln2 = ln.split("|")[-1].strip()
        m = LINE_RE.match(ln2)
        if not m:
            if s and not s.startswith("--"):
                skipped.append((i + 1, ln[:80]))
            continue
        idx = int(m.group(1))
        word = int(m.group(2), 16)
        if word < 0 or word > 0xFFFF:
            raise ValueError(f"valor fora 16-bit na linha {i+1}: {word:x}")
        if idx < 0 or idx >= depth:
            raise IndexError(f"indice MIF invalido {idx}")
        vals[idx] = word

    if skipped[:5]:
        print("# aviso: linhas ignoradas:", len(skipped), file=sys.stderr)
    missing = sum(1 for i, v in enumerate(vals) if i < depth and vals[i] == 0)
    # nao garantimos todos preenchidos; W_in deve ter todos
    if missing == depth:
        raise ValueError("nenhuma entrada CONTENT parseada")
    return vals


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mif_path", help="ex.: memorias/W_in_q.mif")
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args()

    words = parse_mif(args.mif_path)
    depth = len(words)
    blob = struct.pack("<" + "H" * depth, *words)
    with open(args.output, "wb") as fo:
        fo.write(blob)

    print(args.output, len(blob), "bytes", "=", depth, "inteiros unsigned 16-bit LE")


if __name__ == "__main__":
    main()
