{
	'variables': {
		'arch': '<!(node -p "process.arch")',
	},
	'targets': [{
		'target_name': 'segfault',
		'defines!': ['UNICODE', '_UNICODE'],
		'includes': ['../node_modules/addon-tools-raub/utils/common.gypi'],
		'sources': [
			'cpp/bindings.cpp',
			'cpp/segfault-handler.cpp',
		],
		'include_dirs': [
			'<!@(node -p "require(\'addon-tools-raub\').getInclude()")',
		],
		'conditions': [
			['OS=="win"', {
				'sources': ['cpp/stack-windows.cpp'],
			}],
		],
	}],
}
