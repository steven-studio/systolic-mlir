#!/usr/bin/env bash
#
# configure.sh -- 設定 build 目錄。
#
#   ./configure.sh              設定
#   ./configure.sh --build      設定後接著編譯
#   ./configure.sh --test       設定、編譯、跑 check-systolic
#   ./configure.sh --fresh      先丟掉舊的 build 目錄再設定
#
# 環境變數可覆寫自動偵測的結果:
#   LLVM_VERSION=18  BUILD_DIR=build  BUILD_TYPE=Release  ./configure.sh
#
# ---------------------------------------------------------------------------
# 為什麼需要這支
#
#   CMake 的 build 目錄記著它被建立時的來源路徑。專案搬過家(從
#   ~/systolic-mlir 移到 ~/work/systolic-mlir)之後,舊的 CMakeCache.txt
#   仍然指向不存在的舊位置,任何 cmake --build 都會失敗:
#
#     CMake Error: The current CMakeCache.txt directory .../build/CMakeCache.txt
#     is different than the directory /home/steven-studio/systolic-mlir/build
#     where CMakeCache.txt was created.
#
#   那個錯誤訊息沒有說「重新設定就好」,所以這支腳本自己偵測這個情況,
#   並且在重設之前把舊的 cache 移開而不是刪掉 -- 那份 cache 是當初
#   configure 參數的唯一紀錄。
# ---------------------------------------------------------------------------

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$PWD"

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

DO_BUILD=0
DO_TEST=0
FRESH=0
for arg in "$@"; do
    case "$arg" in
        --build)  DO_BUILD=1 ;;
        --test)   DO_BUILD=1; DO_TEST=1 ;;
        --fresh)  FRESH=1 ;;
        -h|--help) sed -n '3,20p' "$0"; exit 0 ;;
        *) echo "不認得的參數: $arg（--build / --test / --fresh）" >&2; exit 2 ;;
    esac
done

say() { printf '\n\033[36m==> %s\033[0m\n' "$*"; }
die() { printf '\033[31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }


# ---------------------------------------------------------------------------
say "尋找 LLVM / MLIR"

find_llvm() {
    # 1) 明確指定的版本
    if [ -n "${LLVM_VERSION:-}" ]; then
        echo "/usr/lib/llvm-${LLVM_VERSION}/lib/cmake"
        return
    fi
    # 2) PATH 上的 llvm-config
    for lc in llvm-config llvm-config-18 llvm-config-17 llvm-config-19 llvm-config-20; do
        if command -v "$lc" >/dev/null 2>&1; then
            local libdir
            libdir="$("$lc" --libdir 2>/dev/null)" || continue
            [ -d "$libdir/cmake/mlir" ] && { echo "$libdir/cmake"; return; }
        fi
    done
    # 3) 掃 /usr/lib,取版本號最大的
    local best=""
    for d in /usr/lib/llvm-*/lib/cmake; do
        [ -d "$d/mlir" ] || continue
        best="$d"
    done
    echo "$best"
}

CMAKE_ROOT="$(find_llvm)"
[ -n "$CMAKE_ROOT" ] || die "找不到 MLIR 的 cmake 目錄。裝了 llvm-*-dev 和 libmlir-*-dev 嗎?
  也可以直接指定:  LLVM_VERSION=18 ./configure.sh"

LLVM_CMAKE="$CMAKE_ROOT/llvm"
MLIR_CMAKE="$CMAKE_ROOT/mlir"
[ -d "$MLIR_CMAKE" ] || die "$MLIR_CMAKE 不存在（少了 libmlir-dev?）"

echo "  LLVM_DIR = $LLVM_CMAKE"
echo "  MLIR_DIR = $MLIR_CMAKE"


# ---------------------------------------------------------------------------
say "尋找 lit"

LIT="${LIT:-}"
if [ -z "$LIT" ]; then
    LIT="$(command -v lit 2>/dev/null || true)"
fi
if [ -z "$LIT" ]; then
    LIT="$(command -v llvm-lit 2>/dev/null || true)"
fi
if [ -z "$LIT" ]; then
    echo "  找不到 lit,check-systolic 會無法執行。"
    echo "  裝法:  pip install lit"
else
    echo "  LLVM_EXTERNAL_LIT = $LIT"
fi


# ---------------------------------------------------------------------------
say "檢查 build 目錄"

CACHE="$BUILD_DIR/CMakeCache.txt"
STALE=0

if [ -f "$CACHE" ]; then
    OLD_HOME="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$CACHE" | head -1)"
    if [ -n "$OLD_HOME" ] && [ "$OLD_HOME" != "$ROOT" ]; then
        echo "  舊的 cache 指向 $OLD_HOME"
        echo "  但專案在        $ROOT"
        echo "  -> 陳舊,必須重新設定"
        STALE=1
    else
        echo "  現有 cache 沒問題（來源路徑相符）"
    fi
else
    echo "  沒有現有的 cache,全新設定"
fi

if [ "$FRESH" = 1 ] || [ "$STALE" = 1 ]; then
    if [ -d "$BUILD_DIR" ]; then
        STAMP="$(date +%Y%m%d-%H%M%S)"
        # 移開而不是刪掉:那份 cache 是舊 configure 參數的唯一紀錄,
        # 新的設定確定能用之前不要銷毀它。
        mv "$BUILD_DIR" "${BUILD_DIR}.stale-${STAMP}"
        echo "  舊目錄移到 ${BUILD_DIR}.stale-${STAMP}（沒有刪除）"
    fi
fi


# ---------------------------------------------------------------------------
say "設定"

ARGS=(
    -S "$ROOT" -B "$BUILD_DIR" -G Ninja
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DLLVM_DIR="$LLVM_CMAKE"
    -DMLIR_DIR="$MLIR_CMAKE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)
[ -n "$LIT" ] && ARGS+=(-DLLVM_EXTERNAL_LIT="$LIT")
[ -x /usr/bin/gcc ] && ARGS+=(-DCMAKE_C_COMPILER=/usr/bin/gcc)
[ -x /usr/bin/g++ ] && ARGS+=(-DCMAKE_CXX_COMPILER=/usr/bin/g++)

command -v ninja >/dev/null 2>&1 || die "找不到 ninja。  sudo apt install ninja-build"

printf '  cmake'; printf ' %q' "${ARGS[@]}"; printf '\n\n'
cmake "${ARGS[@]}"

# compile_commands.json 放到根目錄,編輯器才找得到
if [ -f "$BUILD_DIR/compile_commands.json" ] && [ ! -e compile_commands.json ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" compile_commands.json
    echo "  已連結 compile_commands.json -> $BUILD_DIR/"
fi


# ---------------------------------------------------------------------------
if [ "$DO_BUILD" = 1 ]; then
    say "編譯"
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

if [ "$DO_TEST" = 1 ]; then
    say "測試"
    cmake --build "$BUILD_DIR" --target check-systolic
fi

say "完成"
echo "  編譯:  cmake --build $BUILD_DIR -j\$(nproc)"
echo "  測試:  cmake --build $BUILD_DIR --target check-systolic"
if ls -d "${BUILD_DIR}".stale-* >/dev/null 2>&1; then
    echo
    echo "  確認新的能用之後,舊目錄可以刪掉:"
    for d in "${BUILD_DIR}".stale-*; do echo "    rm -rf $d"; done
fi
