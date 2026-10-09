#!/bin/bash
set -euo pipefail
source "$(dirname "$0")/clip-build-environment.sh"
if [[ "${1:-}" == --help ]]; then
    echo 'Usage: package-clip-shot.sh [clip-shot-macos-{arm64|x64}-release] [--parallel JOBS] [--skip-build]'
    echo 'Builds an ad-hoc signed DMG; Developer ID signing/notarization is a separate release step.'
    exit 0
fi
clip_require_macos
preset=''
if [[ $# -gt 0 && "$1" != --* ]]; then preset="$1"; shift; fi
clip_select_preset "${preset:-clip-shot-macos-$(clip_default_arch)-release}"
[[ "$clip_preset" == *-release ]] || clip_die 'Packaging requires a macOS release preset.'
parallelism=4
skip_build=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --parallel) [[ $# -ge 2 ]] || clip_die '--parallel needs a positive integer'; parallelism="$2"; shift 2 ;;
        --skip-build) skip_build=1; shift ;;
        *) clip_die "Unknown argument: $1" ;;
    esac
done
[[ "$parallelism" =~ ^[1-9][0-9]*$ ]] || clip_die '--parallel needs a positive integer'
clip_setup_tools
if [[ "$skip_build" == 0 ]]; then
    targets=(--target clip_shot)
    if [[ "$clip_preset" == *-arm64-* ]]; then targets+=(--target clip_shot_mini); fi
    "$clip_repo_root/scripts/build.sh" "$clip_preset" --clean \
        "${targets[@]}" --parallel "$parallelism"
fi
cache="$clip_build_dir/CMakeCache.txt"
[[ -f "$cache" ]] || clip_die "CMake cache was not found: $cache"
if [[ "$clip_vcpkg_triplet" == arm64-* ]]; then
    grep -Eq '^CLIP_APPS_BUILD_CLIP_SHOT_MINI:BOOL=(ON|TRUE|1)$' "$cache" ||
        clip_die 'Coordinated ARM64 packaging requires CLIP_APPS_BUILD_CLIP_SHOT_MINI=ON.'
fi
for entry in \
    'CMAKE_BUILD_TYPE:STRING=Release' \
    'CLIP_APPS_BUILD_TESTS:BOOL=OFF' \
    'CLIP_APPS_BUILD_BENCHMARKS:BOOL=OFF' \
    'CLIP_APPS_RELEASE_STATIC:BOOL=ON' \
    'CLIP_APPS_QT_STATIC:BOOL=ON' \
    'CLIP_APPS_PACKAGE_CLIP_SHOT:BOOL=ON' \
    'CLIP_SHOT_IMAGE_CODEC_BACKEND_STATIC:INTERNAL=ON' \
    'QT_FEATURE_static:INTERNAL=ON'; do
    grep -Fqx "$entry" "$cache" || clip_die "Release cache is not production-safe; missing '$entry'."
done
grep -Eq "^VCPKG_TARGET_TRIPLET:.*=$clip_vcpkg_triplet$" "$cache" || clip_die 'Release cache does not use the static macOS vcpkg triplet.'
cd "$clip_repo_root"
cmake -P "$clip_build_dir/clip_shot/GenerateClipShotDiagnosticsSymbols-Release.cmake"
if [[ "$clip_vcpkg_triplet" == arm64-* ]]; then
    cmake -P "$clip_build_dir/clip_shot/GenerateClipShotMiniDiagnosticsSymbols-Release.cmake"
fi
cpack --preset "package-$clip_preset"

if [[ "$clip_vcpkg_triplet" == arm64-* ]]; then
    cpack --config "$clip_build_dir/CPackClipShotMiniConfig.cmake"
fi
