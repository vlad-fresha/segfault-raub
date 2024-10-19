'use strict';


if (global['segfault-raub']) {
	module.exports = global['segfault-raub'];
} else {
	const core = require("pkg-prebuilds/bindings")(
		__dirname,
		require("./binding-options")
	  );
	
	global['segfault-raub'] = core;
	module.exports = core;
}
