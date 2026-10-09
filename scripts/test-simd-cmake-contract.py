#!/usr/bin/env python3
"""Focused AVX2 source-flag, unity, and precompiled-header contract tests."""

import json
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SimdCMakeFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='clip-simd-contract-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'source'
        self.source.mkdir()
        self.build = self.root / 'build'
        self.cmake = shutil.which('cmake')
        self.assertIsNotNone(self.cmake, 'CMake must be available')

    def configure(self, contents, *arguments):
        (self.source / 'CMakeLists.txt').write_text(contents, encoding='utf-8')
        result = subprocess.run(
            [self.cmake, '-S', str(self.source), '-B', str(self.build), '-G', 'Ninja',
             *arguments], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


class SimdCMakeContract(SimdCMakeFixture):
    def properties(self, *, processor, apple=False, architectures='', msvc=False,
                   windows_architecture='', compiler='AppleClang'):
        self.configure(f'''
cmake_minimum_required(VERSION 3.22)
project(ClipSimdContract NONE)
set(APPLE {"TRUE" if apple else "FALSE"})
set(MSVC {"TRUE" if msvc else "FALSE"})
set(CMAKE_SYSTEM_PROCESSOR "{processor}")
set(CMAKE_OSX_ARCHITECTURES "{architectures}")
set(CMAKE_CXX_COMPILER_ID "{compiler}")
set(CLIP_WINDOWS_ARCHITECTURE "{windows_architecture}")
include("{ROOT.as_posix()}/cmake/ClipSimd.cmake")
set_property(SOURCE filter.cpp pen.cpp PROPERTY COMPILE_OPTIONS "-DCLIP_KEEP_OPTION")
clip_configure_avx2_sources(filter.cpp pen.cpp)
foreach(source IN ITEMS filter pen baseline)
    foreach(property IN ITEMS COMPILE_OPTIONS SKIP_UNITY_BUILD_INCLUSION SKIP_PRECOMPILE_HEADERS)
        get_source_file_property(value "${{source}}.cpp" "${{property}}")
        file(APPEND "${{CMAKE_BINARY_DIR}}/properties.txt" "${{source}}.${{property}}=${{value}}\n")
    endforeach()
endforeach()
''')
        return dict(line.split('=', 1) for line in
                    (self.build / 'properties.txt').read_text(encoding='utf-8').splitlines())

    def assert_kernels(self, properties, options='', pch=False):
        expected = '-DCLIP_KEEP_OPTION' + (';' + options if options else '')
        for source in ('filter', 'pen'):
            self.assertEqual(properties[f'{source}.COMPILE_OPTIONS'], expected)
            self.assertEqual(properties[f'{source}.SKIP_UNITY_BUILD_INCLUSION'],
                             'ON' if options else 'NOTFOUND')
            self.assertEqual(properties[f'{source}.SKIP_PRECOMPILE_HEADERS'],
                             'ON' if pch else 'NOTFOUND')
        self.assertEqual(properties['baseline.COMPILE_OPTIONS'], 'NOTFOUND')
        self.assertEqual(properties['baseline.SKIP_UNITY_BUILD_INCLUSION'], 'NOTFOUND')
        self.assertEqual(properties['baseline.SKIP_PRECOMPILE_HEADERS'], 'NOTFOUND')

    def test_windows_x64_uses_selected_compiler_architecture(self):
        self.assert_kernels(self.properties(processor='arm64', msvc=True,
                                           windows_architecture='x64', compiler='MSVC'),
                            '/arch:AVX2')

    def test_windows_clang_x64_isolates_pch(self):
        self.assert_kernels(self.properties(processor='arm64', msvc=True,
                                           windows_architecture='x64', compiler='Clang'),
                            '/arch:AVX2', pch=True)

    def test_windows_arm_does_not_inherit_host_x64_flags(self):
        self.assert_kernels(self.properties(processor='AMD64', msvc=True,
                                           windows_architecture='arm64', compiler='MSVC'))

    def test_other_x86_compilers_keep_avx2_sources_out_of_unity(self):
        self.assert_kernels(self.properties(processor='x86_64', compiler='GNU'), '-mavx2')

    def test_other_arm_compilers_keep_baseline_flags(self):
        self.assert_kernels(self.properties(processor='aarch64', compiler='GNU'))


if __name__ == '__main__':
    unittest.main()