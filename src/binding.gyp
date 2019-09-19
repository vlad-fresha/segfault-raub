{
	'variables': {
		'bin': '<!(node -p "require(\'addon-tools-raub\').bin")',
	},
	'targets': [
		{
			'target_name': 'segfault',
			'sources': [
				'cpp/bindings.cpp',
				'cpp/segfault-handler.cpp',
			],
			'include_dirs': [
				'<!@(node -p "require(\'addon-tools-raub\').include")',
			],
			'cflags!': ['-fno-exceptions'],
			'cflags_cc!': ['-fno-exceptions'],
			'defines': ['NAPI_DISABLE_CPP_EXCEPTIONS'],
			'cflags': [ '-O0', '-funwind-tables' ],
			'xcode_settings': {
				'MACOSX_DEPLOYMENT_TARGET': '10.9',
				'OTHER_CFLAGS': [ '-O0', '-funwind-tables' ],
				'CLANG_CXX_LIBRARY': 'libc++'
			},
			'conditions': [[
			'OS=="win"',
				{
					'defines' : [
						'WIN32_LEAN_AND_MEAN',
						'VC_EXTRALEAN',
					],
					'sources' : [
						'cpp/stack-walker.cpp',
					],
					'msvs_settings' : {
						'VCCLCompilerTool' : {
							'DisableSpecificWarnings': ['4996'],
							'AdditionalOptions' : [
								'/GL', '/GF', '/EHsc', '/GS', '/Gy', '/GR-',
							]
						},
						'VCLinkerTool' : {
							'AdditionalOptions' : ['/DEBUG:NONE'],
						},
					},
				},
			]],
		},
	]
}
