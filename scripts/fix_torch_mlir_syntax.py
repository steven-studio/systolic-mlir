#!/usr/bin/env python3
"""
fix_torch_mlir_syntax.py

torch-mlir 綁定的 LLVM/MLIR 版本通常比系統上 apt 裝的 mlir-opt-18 新很多,
偶爾會用到 mlir-opt-18 的 parser 認不得的較新語法。這個腳本把已知會撞版本
的語法差異轉成 mlir-opt-18 能吃的等價寫法,並順便把匯出函式改名(避免跟
C harness 自己的 main() 符號衝突)。

目前已知並處理的差異:
  - tensor.expand_shape / memref.expand_shape / *.collapse_shape 的
    output_shape [...] 屬性,是較新 MLIR 版本才加的冗餘標註(目標型別已經
    在 "into <type>" 裡寫死了),對 op 語意沒有影響,mlir-opt-18 的舊版
    parser 不認得這個屬性,直接拿掉即可。

用法:
    python3 fix_torch_mlir_syntax.py <input.mlir> <output.mlir> [--rename-main NEW_NAME]
"""
import argparse
import re
import sys


def strip_output_shape_attr(text: str) -> str:
    """去掉 expand_shape/collapse_shape 的 output_shape [...] : 屬性,
    保留冒號讓後面的型別標註接得上。"""
    return re.sub(r'output_shape \[[^\]]*\]\s*:', ':', text)


def rename_main(text: str, new_name: str) -> str:
    """把 @main 換成別的符號名稱,避免跟 C harness 的 main() 衝突。
    只替換完整的 @main token,不動其他碰巧包含 main 的識別字。"""
    return re.sub(r'@main\b', f'@{new_name}', text)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('input', help='torch-mlir 產生的原始 .mlir 檔案')
    parser.add_argument('output', help='修正後要寫入的 .mlir 檔案路徑')
    parser.add_argument('--rename-main', metavar='NEW_NAME', default=None,
                         help='把 @main 換成這個名字(建議一定要指定,避免符號衝突)')
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        text = f.read()

    original = text
    text = strip_output_shape_attr(text)
    if args.rename_main:
        text = rename_main(text, args.rename_main)

    with open(args.output, 'w') as f:
        f.write(text)

    changed = (text != original)
    print(f"寫入 {args.output}"
          f"{'(語法有被修正)' if changed else '(內容未變動,可能沒有需要修正的地方)'}")


if __name__ == '__main__':
    sys.exit(main())
