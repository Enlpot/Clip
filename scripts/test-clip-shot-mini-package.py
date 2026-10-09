"""Configure-only regression for Mini's binary CPack configuration."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class MiniPackageConfiguration(unittest.TestCase):
    def test_source_package_state_does_not_leak_into_mini_binary_package(self):
        generator = 'NSIS'
        with tempfile.TemporaryDirectory(prefix='clip-mini-package-') as directory:
            fixture = Path(directory)
            build = fixture / 'build'
            (fixture / 'CMakeLists.txt').write_text(f'''
cmake_minimum_required(VERSION 4.2)
project(MiniPackage LANGUAGES NONE)
set(CMAKE_SOURCE_DIR "{ROOT.as_posix()}")
set(CLIP_SHOT_VERSION 1.1.9)
set(CLIP_SHOT_VERSION_NUMERIC 1.1.9)
set(CLIP_SHOT_VENDOR "Clip Apps")
set(CLIP_SHOT_COPYRIGHT "Copyright Clip Apps")
add_library(clip_shot_mini INTERFACE)
install(FILES "{ROOT.as_posix()}/clip_shot/packaging/README.txt"
    DESTINATION . COMPONENT ClipShot)
install(FILES "{ROOT.as_posix()}/clip_shot/packaging/README-mini.txt"
    DESTINATION . COMPONENT ClipShotMini)
install(FILES "{ROOT.as_posix()}/clip_shot/tests/installer_updater_stub.cpp"
    DESTINATION bin RENAME clip-shot-mini-updater.exe COMPONENT ClipShotMini)
set(CPACK_PACKAGE_NAME clip-shot)
set(CPACK_PACKAGE_VERSION "${{CLIP_SHOT_VERSION}}")
set(CPACK_GENERATOR {generator})
set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_INSTALL_CMAKE_PROJECTS "${{CMAKE_BINARY_DIR}};MiniPackage;ClipShot;/")
include("{ROOT.as_posix()}/cmake/ClipShotPackageCompression.cmake")
clip_shot_apply_nsis_compression()
include(CPack)
file(SHA256 "${{CMAKE_BINARY_DIR}}/CPackConfig.cmake" full_before)
include("{ROOT.as_posix()}/cmake/ClipShotMiniPackage.cmake")
file(SHA256 "${{CMAKE_BINARY_DIR}}/CPackConfig.cmake" full_after)
if(NOT full_before STREQUAL full_after)
    message(FATAL_ERROR "Mini changed the full binary package configuration")
endif()
''', encoding='utf-8')
            result = subprocess.run(['cmake', '-S', str(fixture), '-B', str(build)],
                                    capture_output=True, text=True, encoding='utf-8',
                                    errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            config = (build / 'CPackClipShotMiniConfig.cmake').read_text(encoding='utf-8')
            values = dict(re.findall(r'^set\((CPACK_\w+) "(.*)"\)$', config, re.MULTILINE))
            self.assertEqual(values['CPACK_GENERATOR'], generator)
            self.assertEqual(values.get('CPACK_INSTALLED_DIRECTORIES', ''), '')
            self.assertFalse(values.get('CPACK_TOPLEVEL_TAG', '').endswith('-Source'))
            self.assertNotEqual(values.get('CPACK_RPM_PACKAGE_SOURCES', ''), 'ON')
            self.assertTrue(values['CPACK_INSTALL_CMAKE_PROJECTS'].endswith(';MiniPackage;ClipShotMini;/'))
            self.assertEqual(values['CPACK_PACKAGE_NAME'], 'clip-shot-mini')
            self.assertEqual(values['CPACK_NSIS_COMPRESSOR'], '/SOLID lzma')
            self.assertIn('SetCompressorDictSize 32', config)
            # Inspect the actual NSIS directive, not an unused CPack variable.
            nsis = Path('C:/Program Files (x86)/NSIS/makensis.exe')
            if not nsis.is_file():
                self.skipTest('NSIS is required to verify the generated installer script')
            for filename in ('CPackConfig.cmake', 'CPackClipShotMiniConfig.cmake'):
                result = subprocess.run(
                    ['cpack', '--config', str(build / filename), '-G', 'NSIS',
                     '-D', f'CPACK_NSIS_EXECUTABLE={nsis.as_posix()}'],
                    cwd=build, capture_output=True, text=True, encoding='utf-8',
                    errors='replace')
                logs = list(build.glob('_CPack_Packages/*/NSIS/NSISOutput.log'))
                diagnostics = result.stdout + result.stderr + ''.join(
                    path.read_text(encoding='utf-8', errors='replace') for path in logs)
                self.assertEqual(result.returncode, 0, diagnostics)
                scripts = list(build.glob('_CPack_Packages/*/NSIS/project.nsi'))
                self.assertEqual(len(scripts), 1)
                script = scripts[0].read_text(encoding='utf-8')
                self.assertRegex(script, r'(?m)^\s*SetCompressor /SOLID lzma\s*$')
                self.assertRegex(script, r'(?m)^\s*SetCompressorDictSize 32\s*$')
            result = subprocess.run(
                ['cmake', '-S', str(fixture), '-B', str(build),
                 '-DCLIP_WINDOWS_ARCHITECTURE=arm64', '-DCLIP_WINDOWS_PLATFORM=windows-arm64'],
                capture_output=True, text=True, encoding='utf-8', errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            arm_config = (build / 'CPackClipShotMiniConfig.cmake').read_text(encoding='utf-8')
            self.assertIn('clip-shot-mini-1.1.9-windows-arm64', arm_config)
            result = subprocess.run(
                ['cpack', '--config', str(build / 'CPackClipShotMiniConfig.cmake'), '-G', 'NSIS',
                 '-D', f'CPACK_NSIS_EXECUTABLE={nsis.as_posix()}'], cwd=build,
                capture_output=True, text=True, encoding='utf-8', errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            scripts = list(build.glob('_CPack_Packages/*/NSIS/project.nsi'))
            self.assertEqual(len(scripts), 1)
            script = scripts[0].read_text(encoding='utf-8')
            self.assertIn('!define CLIP_SHOT_INSTALLER_ARCHITECTURE "arm64"', script)
            init = re.search(r'Function \.onInit\n(.*?)FunctionEnd', script, re.DOTALL).group(1)
            self.assertLess(init.index('!insertmacro ClipShotCheckArchitecture'),
                            init.index('Call ClipShotEnsureMainAppClosed'))
            self.assertLess(init.index('SetRegView 64'), init.index('ReadRegStr'))


if __name__ == '__main__':
    unittest.main()
