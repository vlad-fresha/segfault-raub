'use strict';


if (global['segfault-raub']) {
	module.exports = global['segfault-raub'];
} else {
	const { bin } = require('addon-tools-raub');
	
	const core = require(`./${bin}/segfault`);
	
	global['segfault-raub'] = core;
	module.exports = core;
}
