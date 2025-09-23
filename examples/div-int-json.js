'use strict';

// JSON output example for division by zero
const { causeDivisionInt, setOutputFormat } = require('..');

console.log('Enabling JSON output format...');
setOutputFormat(true);

console.log('Triggering division by zero with JSON output...');
causeDivisionInt();