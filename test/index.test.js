'use strict';

const Segfault = require('..');


describe('Segfault', () => {
	it('exports an object', () => {
		expect(typeof Segfault).toBe('object');
	});
	
	it('contains `causeSegfault` function', () => {
		expect(typeof Segfault.causeSegfault).toBe('function');
	});
});
