# Examples

This directory contains example files demonstrating various features of the segfault-raub module.

## Basic Examples (Plain Text Output)

- `segfault.js` - Basic segfault example
- `div-int.js` - Division by zero example
- `illegal.js` - Illegal instruction example
- `overflow.js` - Stack overflow example

## JSON Output Examples

- `segfault-json.js` - Segfault with JSON output
- `div-int-json.js` - Division by zero with JSON output
- `illegal-json.js` - Illegal instruction with JSON output
- `overflow-json.js` - Stack overflow with JSON output

## Configuration Examples

- `output-format.js` - Demonstrates switching between plain text and JSON output formats
- `signal-config-json.js` - Shows how to configure signals with JSON output enabled

## Running Examples

To run any example:

```bash
node examples/segfault.js
node examples/segfault-json.js
node examples/output-format.js
```

**Warning**: These examples intentionally crash the Node.js process to demonstrate segfault handling. This is expected behavior.

## JSON Output Format

When JSON output is enabled, errors are reported in this structure:

```json
{
  "type": "segfault",
  "signal": 11,
  "signal_name": "SIGSEGV",
  "address": "0x0",
  "pid": 12345,
  "stack": [
    {
      "address": "0x7fff8b2a1234",
      "function": "_segfaultStackFrame1()",
      "module": "/path/to/your/app"
    }
  ]
}
```