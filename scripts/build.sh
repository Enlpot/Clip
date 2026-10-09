#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/clip-build-environment.sh"
if [[ "${1:-}" == --help ]]; then
    echo 'Usage: build.sh [macOS-preset] [--target TARGET] [--parallel JOBS] [--clean] [--skip-bootstrap] [-- CMAKE_OPTIONS...]'
    exit 0
fi
clip_require_macos
preset=''
if [[ $# -gt 0 && "$1" != --* ]]; then preset="$1"; shift; fi
clip_select_preset "$preset"
targets=()
parallelism=''
clean=0
skip_bootstrap=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --target) [[ $# -ge 2 ]] || clip_die '--target needs a target'; targets+=("$2"); shift 2 ;;
        --parallel) [[ $# -ge 2 ]] || clip_die '--parallel needs a positive integer'; parallelism="$2"; shift 2 ;;
        --clean) clean=1; shift ;;
        --skip-bootstrap) skip_bootstrap=1; shift ;;
        --) shift; break ;;
        *) clip_die "Unknown argument: $1" ;;
    esac
done
if [[ -n "$parallelism" && ! "$parallelism" =~ ^[1-9][0-9]*$ ]]; then
    clip_die '--parallel needs a positive integer'
fi
clip_setup_tools
cd "$clip_repo_root"
[[ -x .tools/vcpkg/vcpkg ]] || clip_die 'Run scripts/bootstrap-macos.sh first.'
if [[ "$skip_bootstrap" == 0 ]]; then
    "$clip_repo_root/scripts/bootstrap-macos.sh" "$clip_preset" --skip-dependency-install
fi
if [[ "$clean" == 1 && -e "$clip_build_dir" ]]; then
    case "$clip_build_dir" in
        "$clip_repo_root"/build/clip-shot-macos-*) rm -rf -- "$clip_build_dir" ;;
        *) clip_die "Refusing to remove an unexpected build directory: $clip_build_dir" ;;
    esac
fi
configure=(--preset "$clip_preset" -D "Qt6_DIR=$clip_qt_dir")
if [[ -f "$clip_build_dir/CMakeCache.txt" ]]; then
    if clip_cache_aligned "$clip_build_dir/CMakeCache.txt"; then
        printf 'Reusing the existing CMake cache for preset %s.\n' "$clip_preset"
    else
        printf 'The existing CMake cache does not match preset %s; configuring from a fresh cache.\n' "$clip_preset"
        configure=(--fresh "${configure[@]}")
    fi
fi
cmake "${configure[@]}" "$@"
if [[ ${#targets[@]} == 0 ]]; then
    targets=(clip_shot)
    if grep -Eq '^CLIP_APPS_BUILD_CLIP_SHOT_MINI:BOOL=(ON|TRUE|1)$' "$clip_build_dir/CMakeCache.txt"; then
        targets+=(clip_shot_mini)
    fi
fi
build=(--build --preset "build-$clip_preset" --target "${targets[@]}" --parallel)
if [[ -n "$parallelism" ]]; then build+=("$parallelism"); fi
cmake "${build[@]}"
