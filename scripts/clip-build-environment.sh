#!/bin/bash
# Shared by the macOS entry points; compatible with Apple's Bash 3.2.
set -euo pipefail
clip_repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
clip_die() { printf '%s\n' "$*" >&2; exit 1; }
clip_load_qt_policy() {
    clip_qt_version="$(python3 "$clip_repo_root/scripts/validate-static-qt.py" --print-version)" || clip_die 'The Qt toolchain policy is invalid.'
    clip_qt_source_sha256="$(python3 "$clip_repo_root/scripts/validate-static-qt.py" --print-source-sha256)" || clip_die 'The Qt source archive policy is invalid.'
    clip_qt_deployment_target="$(python3 "$clip_repo_root/scripts/validate-static-qt.py" --print-macos-deployment-target)" || clip_die 'The macOS Qt deployment policy is invalid.'
}
clip_install_qt_license_metadata() {
    local source_dir="$1" license_root="$2" component component_source metadata_file
    # Validate every component before copying any part of the bundle.
    for component in root qtbase qtsvg qttools qttranslations; do
        component_source="$source_dir"
        [[ "$component" == root ]] || component_source="$source_dir/$component"
        metadata_file=REUSE.toml
        # Qt Translations retains upstream license rules rather than REUSE metadata.
        [[ "$component" != qttranslations ]] || metadata_file=licenseRule.json
        [[ -f "$component_source/$metadata_file" && -d "$component_source/LICENSES" ]] || clip_die "Qt licensing metadata is incomplete for $component"
    done
    mkdir -p "$license_root"
    for component in root qtbase qtsvg qttools qttranslations; do
        component_source="$source_dir"
        [[ "$component" == root ]] || component_source="$source_dir/$component"
        metadata_file=REUSE.toml
        [[ "$component" != qttranslations ]] || metadata_file=licenseRule.json
        mkdir -p "$license_root/$component"
        cp "$component_source/$metadata_file" "$license_root/$component/"
        cp -R "$component_source/LICENSES" "$license_root/$component/"
    done
}
clip_require_macos() {
    [[ "$(uname -s)" == Darwin ]] || clip_die 'This entry point requires macOS.'
}
clip_default_arch() {
    case "$(uname -m)" in
        arm64) printf arm64 ;;
        x86_64) printf x64 ;;
        *) clip_die 'Unsupported host architecture.' ;;
    esac
}
clip_require_target_tools() {
    local target_arch="$1"
    clip_requires_rosetta=0
    if [[ "$target_arch" == x64 ]] &&
        [[ "$(clip_default_arch)" == arm64 || "$(sysctl -n hw.optional.arm64 2>/dev/null || true)" == 1 ]]; then
        arch -x86_64 /usr/bin/true >/dev/null 2>&1 || clip_die \
            'macOS x64 builds on Apple Silicon require Rosetta to run Qt tools and validate the OCR worker. Install it with: softwareupdate --install-rosetta --agree-to-license'
        clip_requires_rosetta=1
    fi
}
clip_select_preset() {
    clip_preset="${1:-clip-shot-macos-$(clip_default_arch)-debug}"
    [[ "$clip_preset" =~ ^clip-shot-macos-(arm64|x64)-(debug|performance|release|fast)$ ]] || clip_die "Unsupported macOS preset: $clip_preset"
    case "$clip_preset" in
        clip-shot-macos-arm64-*) clip_arch=arm64; clip_rust_target=aarch64-apple-darwin ;;
        clip-shot-macos-x64-*) clip_arch=x64; clip_rust_target=x86_64-apple-darwin ;;
        *) clip_die "Unsupported macOS preset: $clip_preset" ;;
    esac
    case "${clip_preset##*-}" in
        debug|performance) clip_static_build=0 ;;
        release|fast) clip_static_build=1 ;;
        *) clip_die "Unsupported macOS preset: $clip_preset" ;;
    esac
    if [[ "$clip_static_build" == 1 ]]; then
        clip_vcpkg_triplet="${clip_arch}-osx-clip-shot-static"
        clip_vcpkg_installed="$clip_repo_root/.tools/macos/installed/static"
    else
        clip_vcpkg_triplet="${clip_arch}-osx-clip-shot"
        clip_vcpkg_installed="$clip_repo_root/.tools/macos/installed/dynamic"
    fi
    clip_build_dir="$clip_repo_root/build/$clip_preset"
}
clip_setup_tools() {
    export PATH="$clip_repo_root/.tools/macos-dev/bin:$clip_repo_root/.tools/macos-media/host/bin:$PATH"
    for tool in cmake ninja cargo rustup pkg-config python3; do
        command -v "$tool" >/dev/null || clip_die "Missing $tool. Install the prerequisites listed in docs-macos-build.md."
    done
    clip_require_target_tools "$clip_arch"
    clip_load_qt_policy
    export MACOSX_DEPLOYMENT_TARGET=15.0
    export CARGO_NET_GIT_FETCH_WITH_CLI=true
    if [[ -z "${LIBCLANG_PATH:-}" ]]; then
        LIBCLANG_PATH="$(xcode-select -p)/Toolchains/XcodeDefault.xctoolchain/usr/lib"
        [[ -f "$LIBCLANG_PATH/libclang.dylib" ]] || LIBCLANG_PATH="$(xcode-select -p)/usr/lib"
        export LIBCLANG_PATH
    fi
    # Release and fast presets mirror Windows by using an audited static Qt kit.
    # Development presets continue to use the official shared kit.
    if [[ "${clip_static_build:-0}" == 1 ]]; then
        clip_qt_dir="${CLIP_QT_STATIC_DIR:-${Qt6_DIR:-$HOME/Qt/$clip_qt_version/macos-static-$clip_arch/lib/cmake/Qt6}}"
    else
        clip_qt_dir="${Qt6_DIR:-${CLIP_QT_DIR:-$HOME/Qt/$clip_qt_version/macos/lib/cmake/Qt6}}"
    fi
    [[ -f "$clip_qt_dir/Qt6Config.cmake" ]] || clip_die "Set Qt6_DIR to the Qt $clip_qt_version macOS lib/cmake/Qt6 directory."
    clip_qt_dir="$(cd "$clip_qt_dir" && pwd)"
    if [[ "${clip_static_build:-0}" == 1 ]]; then
        clip_qt_prefix="$(cd "$clip_qt_dir/../../.." && pwd)"
        clip_qt_stamp="$clip_qt_prefix/share/clip-apps/static-qt-build.json"
        [[ -f "$clip_qt_stamp" ]] || clip_die "The audited static Qt build stamp was not found: $clip_qt_stamp. Run scripts/build-static-qt.sh."
        python3 "$clip_repo_root/scripts/validate-static-qt.py" --prefix "$clip_qt_prefix" \
            --arch "$clip_arch" || clip_die 'Rebuild the audited static Qt kit with scripts/build-static-qt.sh.'
        export CLIP_QT_STATIC_DIR="$clip_qt_dir"
    else
        python3 "$clip_repo_root/scripts/validate-static-qt.py" --prefix "$(cd "$clip_qt_dir/../../.." && pwd)" \
            --arch "$clip_arch" --kit-only || clip_die "Install the Qt $clip_qt_version macOS kit for $clip_arch."
    fi
    export Qt6_DIR="$clip_qt_dir"
}

clip_cache_aligned() {
    local cache_path="$1"
    local cached_qt
    [[ -f "$cache_path" ]] || return 1
    cached_qt="$(sed -n 's/^Qt6_DIR:[^=]*=//p' "$cache_path" | head -n 1)"
    [[ "$cached_qt" == "$clip_qt_dir" ]] &&
        grep -Eq "^VCPKG_TARGET_TRIPLET:.*=$clip_vcpkg_triplet$" "$cache_path" &&
        grep -Fqx "VCPKG_INSTALLED_DIR:PATH=$clip_vcpkg_installed" "$cache_path" &&
        grep -Fqx "CMAKE_HOME_DIRECTORY:INTERNAL=$clip_repo_root" "$cache_path" &&
        grep -Fqx 'CMAKE_GENERATOR:INTERNAL=Ninja' "$cache_path"
}
