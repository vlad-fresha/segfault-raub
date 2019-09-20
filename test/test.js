'use strict';

const { expect } = require('chai');

const Segfault = require('..');


describe('Segfault', () => {
	
	it('exports an object', () => {
		expect(Segfault).to.be.an('object');
	});
	
	it('contains `causeSegfault` function', () => {
		expect(Segfault.causeSegfault).to.be.a('function');
	});
	
});
