#!/usr/bin/env python3
"""Gera imagem_teste.raw com 784 bytes (28x28) para testar elm_store_img."""

import argparse

W, H = 28, 28
N = W * H


def main():
    p = argparse.ArgumentParser()
    p.add_argument("-o", "--output", default="imagem_teste.raw")
    args = p.parse_args()

    data = bytearray(N)
    for i in range(N):
        x = i % W
        y = i // W
        data[i] = (17 * x + 31 * y + i) % 256

    data[0] = 200
    data[N // 2] = 128
    data[N - 1] = 55

    with open(args.output, "wb") as f:
        f.write(data)

    print(args.output, len(data), "bytes")


if __name__ == "__main__":
    main()
