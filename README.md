Dumps the code and data memory of emulator applications based on `SimU8.dll`.

**NOTE: `SimU8.dll` MUST NOT BE MODIFIED, OR ELSE THE SCRIPT WON'T WORK!**

## Download
Pre-built binaries can be found on the [Releases](../../releases/latest) page.

## Requirements
At least Windows Vista.

## Usage
```
Usage: dump <option> <pid>

  <option>        Can be 'code' for code memory; 'data' for data segment 0;
                  or 'both' for both of the above.
  <pid>           The process ID (PID) of the emulator.
```

## Building
Run `make` in the root of the repository.

## Supported versions
The dumper supports these `SimU8.dll` versions (check the "File version" field in the DLL properties):
- 1.11.100.0
- 1.15.200.0
- 2.0.100.0
- 2.10.1.0
