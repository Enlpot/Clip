# Clip Apps

Clip Apps repository, providing source code for Clip and Clip Image Viewer.

For native Windows ARM64 builds and packages, see
[Windows ARM64 build support](docs-windows-arm64-build.md), including cross compilation,
OCR preparation, and release validation prerequisites.

<div style="font-size: 128px">🏗️🚧🦺</div>

## Install Clip on Windows

Download the installer from [Clip releases](https://github.com/mg-chao/clip-apps/releases).
Install the offline Windows x64 package from the WinGet community repository with:

```powershell
winget install --exact --id mg-chao.clip-shot --source winget
```

This package includes published beta versions and the default OCR resources. See
[WinGet release support](docs/clip-shot-releases.md#winget) for upgrades, removal,
and maintainer setup.

Once [Clip's submission](https://github.com/ScoopInstaller/Extras/issues/18870)
is accepted into Scoop's official
[Extras bucket](https://github.com/ScoopInstaller/Extras), install the portable
Windows x64 package with [Scoop](https://scoop.sh):

```powershell
scoop bucket add extras
scoop install extras/clipshot
```

The Scoop package follows stable releases and includes the default OCR resources.
Quit Clip before running `scoop update clipshot`; use Scoop rather than the
app's built-in updater. Settings and history persist across upgrades and ordinary
uninstall. See [Scoop release support](docs/clip-shot-releases.md#scoop) for removal,
data locations, and maintainer setup.

## Install Clip on macOS

Apple Silicon Macs running macOS 15 or later can install the stable release with:

```sh
brew update
brew install --cask mg-chao/tap/clip-shot
brew upgrade --cask clip-shot
brew uninstall --cask clip-shot
```

Installation reuses Clip's local signing identity and may request Keychain
access and macOS privacy permissions. See [Homebrew installation and migration](docs-macos-build.md#homebrew-installation)
for existing installations, custom app directories, and recovery.

## Open Source Licenses

This is a multi-license repository. See [LICENSE.md](LICENSE.md) for the
repository-level scope rules and third-party material policy.

| Project | License |
| --- | --- |
| `ant_design_qt/` | [Apache License 2.0](ant_design_qt/LICENSE) |
| `clip-crates/` | [Apache License 2.0](clip-crates/LICENSE) |
| `clip_draw_engine_qt/` | [Apache License 2.0](clip_draw_engine_qt/LICENSE) |
| `clip_rust_ffi/` | [Apache License 2.0](clip_rust_ffi/LICENSE) |
| `clip_image/` | [GNU GPL v3.0 or later](clip_image/COPYRIGHT) |
| `clip_image_viewer/` | [GNU GPL v3.0 or later](clip_image_viewer/COPYRIGHT) |
| `clip_shot/` | [GNU GPL v3.0 or later](clip_shot/COPYRIGHT) |

Synchronized and bundled third-party materials retain their upstream licenses.
See [Ant Design Qt third-party notices](ant_design_qt/THIRD_PARTY_NOTICES.md)
and [Clip third-party notices](clip_shot/THIRD_PARTY_NOTICES.md).
