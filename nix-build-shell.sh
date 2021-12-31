#!/usr/bin/env bash
set -euo pipefail

die() { echo "Error: $*" >&2; exit 1;  }

S=$PWD

target=$(basename "$1")
target=${target%%.*}
shift

crossSystem=$target
if [[ ${1:-} != "--" ]]; then
    crossSystem=${1:-$target}
fi

(( $# == 0 )) || shift

[[ ${1:-} != "--" ]] || shift

B=$S/build

mkdir -p "$B/nix-shells"

nixfile=$B/nix-shells/${target}.nix

#[[ -r $nixfile ]] ||
    cat <<EOF >"$nixfile"
{ pkgs ? import <nixpkgs> { } }:

with pkgs.pkgsCross.${crossSystem}; mkShell {}
EOF


toolchain=$S/toolchains/any-cross.cmake
mkdir -p "$B/toolchains"
toolchain_link=$B/toolchains/${target}-cc.cmake
export CMAKE_TOOLCHAIN_FILE=$toolchain_link
ln -sfr "$toolchain" "$toolchain_link"

(( $# > 0 )) || set -- "$S"/build-cross.sh "$toolchain_link"

export CROSS_TARGET=$target
exec nix-shell --run "$*" "$nixfile"
