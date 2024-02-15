Dumps the code and data memory of emulator applications based on `SimU8.dll`.

**NOTE: `SimU8.dll` MUST NOT BE MODIFIED, OR ELSE THE SCRIPT WON'T WORK!**

## Usage
```
dump.exe <code|data|both> <pid>
```

## Building
You will need the headers and libraries for the Windows API. Then just use your favorite compiler to compile `dump.c`.
Make sure to add `-lversion`.

## Supported versions
The dumper supports these `SimU8.dll` versions (check the "File version" field in the DLL properties):
- 1.11.100.0
- 1.15.200.0
- 2.0.100.0
- 2.10.1.0
