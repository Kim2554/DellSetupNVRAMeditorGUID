#!/bin/sh
set -e
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SRC="$ROOT/src"
OBJ="$ROOT/build"
mkdir -p "$OBJ" "$ROOT/EFI/BOOT"
CFLAGS="--target=x86_64-pc-windows-msvc -ffreestanding -fshort-wchar -mno-red-zone -fno-stack-protector -fno-builtin -nostdinc -Wall -Wextra -Wpedantic -Werror"
clang $CFLAGS -c "$SRC/main.c" -o "$OBJ/main.obj"
clang $CFLAGS -c "$SRC/ia_data.c" -o "$OBJ/ia_data.obj"
lld-link "$OBJ/main.obj" "$OBJ/ia_data.obj" /entry:efi_main /subsystem:efi_application /nodefaultlib /out:"$ROOT/EFI/BOOT/BOOTX64.EFI"
file "$ROOT/EFI/BOOT/BOOTX64.EFI"
