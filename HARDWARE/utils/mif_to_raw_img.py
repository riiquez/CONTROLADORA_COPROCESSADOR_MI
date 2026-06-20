#!/usr/bin/env python3
"""
imagem_x.mif (784 x WIDTH=8 HEX) -> .raw 784 bytes por pixel STORE_IMG na ordem MIF.

Exemplo:
  python utils/mif_to_raw_img.py memorias/imagem_0.mif -o tutoriais/store_img_api/imagem_0.raw
"""


import argparse
import re
import sys

LINE_RE = re.compile(r"^\s*(\d+)\s*:\s*([\da-fA-F]+)\s*;\s*")


def parse_mif_image(path):
    """WIDTH=8, DEPTH esperado 784."""
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()

    depth = width = None
    for ln in lines:
        t = ln.strip()
        if t.startswith("DEPTH"):
            depth = int(t.split("=")[1].strip().rstrip(";"))
        elif t.startswith("WIDTH"):
            width = int(t.split("=")[1].strip().rstrip(";"))
        if depth is not None and width is not None:
            break

    if depth is None or width != 8:
        raise ValueError(f"MIF esperado DEPTH definido com WIDTH=8; depth={depth} width={width}")
    if depth != 784:
        print("# aviso: DEPTH=", depth, " (mem_img Maik espera exatamente 784)", file=sys.stderr)

    vals = [0] * depth
    in_content = False
    parsed_rows = 0
    for ln in lines:
        s = ln.strip()
        su = s.upper()
        if su.startswith("CONTENT"):
            if "BEGIN" in su:
                in_content = True
            continue
        if su.startswith("BEGIN"):
            in_content = True
            continue
        if su.startswith("END"):
            break
        if not in_content:
            continue
        ln2 = ln.split("|")[-1].strip()
        m = LINE_RE.match(ln2)
        if not m:
            continue
        idx = int(m.group(1))
        byte = int(m.group(2), 16)
        if byte > 0xFF or byte < 0:
            raise ValueError(f"valor fora 8-bit linha proxima ao indice MIF={idx}: {byte}")
        if not (0 <= idx < depth):
            raise IndexError(f"indice invalido {idx}")
        vals[idx] = byte
        parsed_rows += 1

    if parsed_rows == 0:
        raise ValueError("nenhuma entrada CONTENT parseada")
    if parsed_rows < depth:
        print(
            f"# aviso: linhas parseadas={parsed_rows}, DEPTH={depth}",
            file=sys.stderr,
        )

    return bytes(vals)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mif_path")
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args()

    blob = parse_mif_image(args.mif_path)
    with open(args.output, "wb") as fo:
        fo.write(blob)

    print(args.output, len(blob), "bytes (grayscale uint8 raster MIF-order)")


if __name__ == "__main__":
    main()
