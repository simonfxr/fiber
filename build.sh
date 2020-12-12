#!/usr/bin/env bash

SYS_LINUX=
SYS_BSD=
SYS_WINDOWS=

case "$(uname -s)" in
    Linux) SYS_LINUX=1 ;;
    *BSD) SYS_BSD=1 ;;
    MSYS_NT*) SYS_WINDOWS=1 ;;
esac

BITS32=
BITS64=

case "$(getconf LONG_BIT)" in
    32) BITS32=1 ;;
    64) BITS64=1 ;;
esac

case "$(uname -m)" in
    amd64|x86_64) BITS64=1  ;;
    aarch64) BITS64=1 ;;
    arm*) BITS32=1 ;;
esac

[[ $BITS32 || $BITS64 ]] || {
    echo "failed to detect 32 vs 64 bit architecture" >&2
    exit 1
}

native_bits=64
[[ $BITS32 ]] && native_bits=32

build_types=( debug release )

ccs=( )
[[ ! $SYS_WINDOWS ]] && type -P gcc &>/dev/null && ccs+=( gcc )
[[ ! $SYS_WINDOWS ]] && type -P clang &>/dev/null && ccs+=( clang )
[[ $SYS_WINDOWS ]] && type -P cl &>/dev/null && ccs+=( cl )
type -P icc &>/dev/null && ccs+=( icc )

link_modes=( static shared )

compiler_supports_m32() {
    [[ $BITS32 || $SYS_WINDOWS || "$1" = icc ]] && return 1
    local cc="$1" tmp_file
    tmp_file="$(mktemp -t test_m32_XXXXXX.c)"
    bin_file="$(mktemp -t test_m32_XXXXXX.bin)"
    echo -e '#include <stdlib.h>\nint main() { exit(0); return 0; }' > "$tmp_file"
    "$cc" -x c -o "$bin_file" -m32 "$tmp_file" &>/dev/null
    local ret=$?
    [[ $ret -eq 0 && -x "$bin_file" ]] || ret=1
    rm -f "$tmp_file" "$bin_file" &>/dev/null
    if [[ $ret -eq 0 ]]; then
        echo "Compiler $cc supports -m32"
    else
        echo "Compiler $cc does NOT support -m32"
    fi
    return $ret
}

build_configs=( )
declare -A cmake_flags
cmake_flags=( )

add_build_config() {
    local bt="$1" cc="$2" bits="$3" link_mode="$4" check_align="$5"
    local cm_flags=( -DCMAKE_BUILD_TYPE="${bt^^}" -DCMAKE_C_COMPILER="$cc" )

    [[ $SYS_WINDOWS && $cc = $cl && $link_mode = shared ]] && return

    [[ $BITS64 && $bits = 32 && $cc != "cl" ]] &&
        cm_flags+=( -DFIBER_M32=True )
    if [[ $link_mode = shared ]]; then
        cm_flags+=( -DFIBER_SHARED=True )
    else
        cm_flags+=( -DFIBER_SHARED=False )
    fi
    if [[ $check_align = "checked" ]]; then
        cm_flags+=( -DFIBER_ASM_CHECK_ALIGNMENT=True )
    else
        cm_flags+=( -DFIBER_ASM_CHECK_ALIGNMENT=False )
    fi
    local conf_name="${bt}-${cc}${bits}-${link_mode}-${check_align}"
    cmake_flags["$conf_name"]="${cm_flags[*]}"
    build_configs+=( "$conf_name" )
}

for cc in "${ccs[@]}"; do
    bit_modes=( "$native_bits" )
    #compiler_supports_m32 "$cc" && bit_modes+=( 32 )
    for bits in "${bit_modes[@]}"; do
        if [[ $cc = cl && $bits = 64 ]]; then
            { cl 2>&1 | grep "x64"; } || bits=32
        fi
        for bt in "${build_types[@]}"; do
            for link_mode in "${link_modes[@]}"; do
                for ca in "unchecked" "checked"; do
                    add_build_config "$bt" "$cc" "$bits" "$link_mode" "$ca"
                done
            done
        done
    done
done

cmd_configure() {
    local flags=( "$@" )
    mkdir -p build || return $?
    for bc in "${build_configs[@]}"; do
        rm -rf "build/$bc"
        mkdir "build/$bc" || return $?
        (
            cd "build/$bc" || exit $?
            echo "************** $bc **************"
            local all_flags=( "${flags[@]}" ${cmake_flags["$bc"]} )
            cmake -G Ninja "${all_flags[@]}" ../..
        )
    done
}

cmd_build() {
    local flags=( "$@" )
    mkdir -p build || return $?
    for bc in "${build_configs[@]}"; do
        (
            cd "build/$bc" || exit $?
            echo "************** $bc **************"
            cmake --build . "${flags[@]}"
        )
    done
}

cmd_test() {
    local flags=( "$@" )
    mkdir -p build || return $?
    for bc in "${build_configs[@]}"; do
        local d="build/$bc"
        echo "************** $bc **************"
        (
            cd "$d" || exit $?
            ctest >/dev/null || {
                echo "************** FAILED **************"
            }
        ) || exit $?
    done
}

case "$1" in
    configure|build|test)
        "cmd_$1" "${@:2}"
        ;;
    *)
        echo "invalid args: $*" >&2
        ;;
esac
