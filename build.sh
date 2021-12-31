#!/usr/bin/env bash

set -uo pipefail

FIBER_CROSS=$CROSS_TARGET

SYS_LINUX=
SYS_BSD=
SYS_WINDOWS=

cmake_reconfigure=0
cmake_generator=
cmake_configure_args=( -DCMAKE_EXPORT_COMPILE_COMMANDS=True )

BITS32=
BITS64=

if [[ ! $FIBER_CROSS ]]; then

    case "$(uname -s)" in
        Linux) SYS_LINUX=1 ;;
        *BSD) SYS_BSD=1 ;;
        MSYS_NT*) SYS_WINDOWS=1 ;;
    esac

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

    ccs=( )
    [[ ! $SYS_WINDOWS ]] && type -P gcc &>/dev/null && ccs+=( gcc )
    [[ ! $SYS_WINDOWS ]] && type -P clang &>/dev/null && ccs+=( clang )
    [[ $SYS_WINDOWS ]] && type -P cl &>/dev/null && ccs+=( cl )
    type -P icc &>/dev/null && ccs+=( icc )
else
    native_bits=_
    ccs=( 'cross' )
fi

build_types=( debug release )

link_modes=( static shared )

compiler_supports_m32() {
    [[ ! $FIBER_CROSS ]] || return 1
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

add_build_config() {
    local bt="$1" cc="$2" bits="$3" link_mode="$4" check_align="$5"
    local cm_flags=( -DCMAKE_BUILD_TYPE="${bt^^}" )

    if [[ $FIBER_CROSS ]]; then
        cm_flags+=( -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE" )
    else
        cm_flags+=( -DCMAKE_C_COMPILER="$cc" )
    fi

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

    local conf_name
    if [[ $FIBER_CROSS ]]; then
        conf_name="$FIBER_CROSS-${bt}-${link_mode}-${check_align}"
    else
        conf_name="${bt}-${cc}${bits}-${link_mode}-${check_align}"
    fi
    eval "export cmake_flags_${conf_name//-/_}=\"${cm_flags[*]}\""
    build_configs+=( "$conf_name" )
}

init_configs() {
    for cc in "${ccs[@]}"; do
        bit_modes=( "$native_bits" )
        compiler_supports_m32 "$cc" && bit_modes+=( 32 )
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
}

do_all() {
    local IFS=$'\n'
    echo "${build_configs[*]}" | xargs -rn1 -d'\n' -P "$(nproc)" "$0" "$@"
}


__configure1() {
    local bc="$1" d
    d=build/$bc
    (( cmake_reconfigure )) || [[ ! -r $d/CMakeCache.txt ]] || return 0
    rm -rf "$d"
    mkdir "$d" || return $?
    cd "$d" || exit $?
    echo "************** $bc **************"
    local flagsvar=cmake_flags_${bc//-/_}
    local all_flags=( "${flags[@]}" ${!flagsvar} )
    local gen=(  )
    [[ ! ${cmake_generator:-} ]] || gen+=( -G "$cmake_generator" )
    cmake "${gen[@]}" "${cmake_configure_args[@]}" "${all_flags[@]}" ../..
}

cmd_configure() {
    local flags=( "$@" )
    mkdir -p build || return $?
    do_all __configure1
}

__build1() {
    local bc="$1"
    cd "build/$bc" || exit $?
    echo "************** $bc **************"
    cmake --build . "${flags[@]}"
}

cmd_build() {
    local flags=( "$@" )
    mkdir -p build || return $?
    do_all __build1
}

__test1() {
    local bc="$1"
    local d="build/$bc"
    echo "************** $bc **************"
    cd "$d" || return $?
    ctest >/dev/null || {
        ret=$?
        echo "************** FAILED **************"
        return $ret
    }
}

cmd_test() {
    local flags=( "$@" )
    mkdir -p build || return $?
    do_all __test1
}

cmd_all() {
    cmd_configure "$@"
    cmd_test
}

case "$1" in
    configure|build|test|all)
        init_configs
        "cmd_$1" "${@:2}"
        ;;
    __*)
        "$1" "${@:2}" ;;
    *)
        echo "invalid args: $*" >&2
        ;;
esac
