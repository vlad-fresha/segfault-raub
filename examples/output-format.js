'use strict';

// Example demonstrating output format configuration
const { setOutputFormat, getOutputFormat, causeSegfault } = require('..');

console.log('=== Output Format Configuration Example ===\n');

// Check default format
console.log('Default output format (false = plain text, true = JSON):', getOutputFormat());

console.log('\n--- Testing Plain Text Output ---');
setOutputFormat(false);
console.log('Current format:', getOutputFormat());
console.log('Triggering segfault with plain text output...');

setTimeout(() => {
  console.log('\n--- Testing JSON Output ---');
  setOutputFormat(true);
  console.log('Current format:', getOutputFormat());
  console.log('Triggering segfault with JSON output...');

  setTimeout(() => {
    causeSegfault();
  }, 1000);
}, 2000);