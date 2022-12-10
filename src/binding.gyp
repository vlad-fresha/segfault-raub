{
	'variables': {
		'bin': '<!(node -p "require(\'addon-tools-raub\').bin")',
	},
	'targets': [{
		'target_name': 'segfault',
		'sources': [
			'cpp/bindings.cpp',
			'cpp/segfault-handler.cpp',
		],
		'include_dirs': [
			'<!@(node -p "require(\'addon-tools-raub\').include")',
		],
		'defines': ['UNICODE', '_UNICODE'],
		'cflags_cc': ['-std=c++17'],
		'cflags': ['-O0', '-funwind-tables'],
		'conditions': [
			['OS=="linux"', {
				'defines': ['__linux__'],
			}],
			['OS=="mac"', {
				'MACOSX_DEPLOYMENT_TARGET': '10.9',
				'defines': ['__APPLE__'],
				'CLANG_CXX_LIBRARY': 'libc++',
				'OTHER_CFLAGS': ['-std=c++17', '-O0', '-funwind-tables'],
			}],
			['OS=="win"', {
				'defines' : ['WIN32_LEAN_AND_MEAN', 'VC_EXTRALEAN', '_WIN32'],
				'sources' : ['cpp/stack-walker.cpp'],
				'msvs_settings' : {
					'VCCLCompilerTool' : {
						'AdditionalOptions' : [
							'/GL', '/GF', '/EHsc', '/GS', '/Gy', '/GR-', '/std:c++17',
						]
					},
					'VCLinkerTool' : {
						'AdditionalOptions' : ['/DEBUG:NONE', '/LTCG'],
					},
				},
			}],
		],
	}],
}
