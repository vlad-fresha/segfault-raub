{
	'variables': {
		'arch': '<!(node -p "process.arch")',
		'is_musl': '<!(if ldd --version 2>&1 | grep -q musl; then echo "true"; else echo "false"; fi)',
	},
	'conditions': [
		['OS=="win"', {
			'variables': {
				'use_libunwind': 'false',
			},
		}, {
			'variables': {
				'use_libunwind': '<!(if [ "$USE_LIBUNWIND" = "true" ] || [ "$USE_LIBUNWIND" = "1" ]; then echo "true"; elif [ "$USE_LIBUNWIND" = "false" ] || [ "$USE_LIBUNWIND" = "0" ]; then echo "false"; elif ldd --version 2>&1 | grep -q musl; then echo "true"; else echo "false"; fi)',
			},
		}],
	],
	'targets': [{
		'target_name': 'vlad_fresha_segfault_handler',
		'sources': [
			'src/cpp/bindings.cpp',
			'src/cpp/segfault-handler.cpp',
		],
		'include_dirs': [
			'<!@(node -p "require(\'node-addon-api\').include")',
			'src/libunwind/include',
		],
		'cflags_cc': ['-std=c++17', '-fno-exceptions', '-Wall', '-Werror'],
		'cflags': ['-O0', '-funwind-tables', '-fno-exceptions', '-Wall', '-Werror'],
		'conditions': [
			['OS=="linux"', {
				'defines': ['__linux__'],
				'conditions': [
					['use_libunwind=="true"', {
						'conditions': [
							['is_musl=="true"', {
								# Use local static libunwind for musl systems
								'libraries': [
									'<(PRODUCT_DIR)/../../src/libunwind/libunwind.so.8.1.0',
									'<(PRODUCT_DIR)/../../src/libunwind/liblzma.so.5',
								],
								'ldflags': ['-static-libgcc', '-static-libstdc++'],
							}, {
								# Use system libunwind for glibc systems
								'conditions': [
									['"<(arch)"=="x64"', {
										'libraries': ['-lunwind', '-lunwind-x86_64'],
									}],
									['"<(arch)"=="arm64"', {
										'libraries': ['-lstdc++fs', '-lunwind', '-lunwind-aarch64'],
									}],
								],
							}],
						],
					}],
					['use_libunwind=="false"', {
						'conditions': [
							['"<(arch)"=="arm64"', {
								'libraries': ['-lstdc++fs'],
							}],
						],
					}],
				],
			}],
			['OS=="mac"', {
				'MACOSX_DEPLOYMENT_TARGET': '10.9',
				'defines': ['__APPLE__'],
				'CLANG_CXX_LIBRARY': 'libc++',
				'OTHER_CFLAGS': ['-std=c++17', '-O0', '-funwind-tables', '-fno-exceptions'],
				'cflags_cc+': ['-Wno-deprecated-volatile'],
				'cflags+': ['-Wno-deprecated-volatile'],
			}],
			['OS=="win"', {
				'defines': ['WIN32_LEAN_AND_MEAN', 'VC_EXTRALEAN', '_WIN32', '_HAS_EXCEPTIONS=0'],
				'sources': ['src/cpp/stack-windows.cpp'],
				'msvs_settings': {
					'VCCLCompilerTool': {
						'AdditionalOptions' : [
							'/GL', '/GF', '/EHa-s-c-', '/GS', '/Gy', '/GR-', '/std:c++17', '/W4', '/WX', '/wd4127', '/wd4100', '/wd4068', '/wd4189',
						],
					},
					'VCLinkerTool': {
						'AdditionalOptions' : ['/DEBUG:NONE', '/LTCG', '/OPT:NOREF'],
					},
				},
			}],
		],
	}],
}
