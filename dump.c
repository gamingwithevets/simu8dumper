#include <windows.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <psapi.h>	
#include <libloaderapi.h>

/*
 * memory format
 * 0x00:  ??
 * 0x04:  Memory buffer
 * 0x08:  Buffer size
 * 0x0C:  ??
 * 0x10:  ??
 * 0x14:  Lower memory bound
 * 0x18:  Upper memory bound
*/
struct memory_region {
	uint8_t  unk0[4];
	uint32_t buf_addr;
	uint32_t buf_size;
	uint8_t  unk1[8];
	uint32_t lower_mem_bound;
	uint32_t upper_mem_bound;
	uint8_t  unk2[4];
};

static const uint8_t pattern_csimu8[] = {0x57, 0x44, 0x54, 0x49, 0x4E, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t pattern_romname[] = {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

HMODULE getModule(HANDLE proc, char *name) {
	HMODULE mods[100];
	uint32_t cbNeeded;
	char modName[100];

	if (EnumProcessModulesEx(proc, mods, sizeof(mods), &cbNeeded, 1)) {
		for(int x = 0; x < (cbNeeded/sizeof(HMODULE)); x++) {
			GetModuleBaseNameA(proc, mods[x], modName, sizeof(modName));
			if(strcmp(name, modName) == 0) {
				return mods[x];
			}
		}
	}

	return NULL;
}

int *GetExeFileVersion(const TCHAR *filePath, HANDLE handle, char *out) {
	DWORD infoSize = GetFileVersionInfoSize(filePath, &handle);
	if (infoSize == 0) {
		printf("Failed to get file version info size [%d]\n", GetLastError());
		return 0;
	}

	LPVOID versionInfo = malloc(infoSize);
	if (!versionInfo) {
		printf("Memory allocation failed [%d]\n", GetLastError());
		return 0;
	}

	if (!GetFileVersionInfo(filePath, handle, infoSize, versionInfo)) {
		printf("Failed to get file version info [%d]\n", GetLastError());
		free(versionInfo);
		return 0;
	}

	VS_FIXEDFILEINFO *fileInfo;
	UINT fileInfoSize;
	if (!VerQueryValue(versionInfo, "\\", (LPVOID *)&fileInfo, &fileInfoSize)) {
		printf("Failed to query file version info [%d]\n", GetLastError());
		free(versionInfo);
		return 0;
	}

	sprintf(out, "%d.%d.%d.%d",
		HIWORD(fileInfo->dwFileVersionMS),
		LOWORD(fileInfo->dwFileVersionMS),
		HIWORD(fileInfo->dwFileVersionLS),
		LOWORD(fileInfo->dwFileVersionLS));

	free(versionInfo);
	return 1;
}

// https://stackoverflow.com/a/14422935
int digits_only(const char *s)
{
	while (*s) {
		if (isdigit(*s++) == 0) return 0;
	}

	return 1;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s <option> <pid>\n\n", argv[0]);
		printf("  <option>        Can be 'code' for code memory; 'data' for data segment 0;\n");
		printf("                  or 'both' for both of the above.\n");
		printf("  <pid>           The process ID (PID) of the emulator.\n");
		return -1;
	}

	// Check what we want to dump
	bool dump_rom = false;
	bool dump_ram = false;
	uint32_t pid;

	if (strcmp(argv[1], "code") == 0) dump_rom = true;
	else if (strcmp(argv[1], "data") == 0) dump_ram = true;
	else if (strcmp(argv[1], "both") == 0) {
		dump_rom = true;
		dump_ram = true;
	} else {
		printf("Invalid option: %s\n", argv[1]);
		return -1;
	}

	if (digits_only(argv[2]) == 0 || sscanf(argv[2], "%" SCNd32, &pid) == 0) {
		printf("Invalid PID: %s\n", argv[2]);
		return -1;
	}

	// Get handle on the emulator
	HANDLE proc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, pid);

	if (proc == NULL) {
		printf("Failed to open handle [%d]\n", GetLastError());
		return -1;
	}

	// Get the full path of the executable
	char exePath[MAX_PATH];
	if (GetModuleFileNameEx(proc, NULL, exePath, MAX_PATH) == 0) {
		printf("Failed to get emulator EXE path [%d]\n", GetLastError());
		return -1;
	}

	// Remove the extension
	char *ptr = strrchr(exePath, 0x2e);
	if (ptr != NULL) *ptr = 0;

	// Extract the filename without extension
	char *exeName = NULL;
	for (int i = strlen(exePath) - 1; i >= 0; i--) {
		if (exePath[i] == '\\') {
			exeName = &exePath[i + 1];
			break;
		}
	}

	// Get SimU8.dll base address
	HMODULE SimU8_mod = getModule(proc, "SimU8.dll");
	if (SimU8_mod == NULL) {
		printf("Failed to get handle for SimU8.dll\n");
		return -1;
	}

	char filename[MAX_PATH];
	if (GetModuleFileNameEx(proc, SimU8_mod, filename, MAX_PATH) == 0) {
		printf("Failed to get SimU8.dll path [%d]\n", GetLastError());
		return -1;
	}

	char version[20];
	if (GetExeFileVersion(&filename, SimU8_mod, version) == 0) {
		
	}

	MODULEINFO modinfo;
	if(GetModuleInformation(proc, SimU8_mod, &modinfo, sizeof(modinfo)) == 0) {
		printf("Failed to get module info for SimU8.dll [%d]\n", GetLastError());
		return -1;
	}
	uint32_t SimU8_base_addr = (uint32_t)modinfo.lpBaseOfDll;

	uint32_t sim_state_addr;
	if (strcmp(version, "1.11.100.0") == 0) sim_state_addr = SimU8_base_addr + 0x282C0;
	else if (strcmp(version, "1.15.200.0") == 0) sim_state_addr = SimU8_base_addr + 0x392AC;
	else if (strcmp(version, "2.0.100.0") == 0) sim_state_addr = SimU8_base_addr + 0x16CE20;
	else if (strcmp(version, "2.10.1.0") == 0) sim_state_addr = SimU8_base_addr + 0x16BE28;
	else {
		printf("Unknown SimU8.dll version: %s\n", version);
		return -1;
	}

	printf("SimU8.dll version: %s\n", version);
	uint8_t buffer[0xFF];
	if(ReadProcessMemory(proc, (LPCVOID)sim_state_addr, buffer, 0xFF, NULL) == 0) {
		printf("Failed to read memory [%d]\n", GetLastError());
		return -1;
	}

	// ROM Segment 0 +0x2C
	// ROM Segment 1 +0x64
	// RAM           +0x48
	struct memory_region *rom_seg0 = (struct memory_region *)(buffer + 0x2C);
	struct memory_region *rom_seg1 = (struct memory_region *)(buffer + 0x64);
	struct memory_region *ram = (struct memory_region *)(buffer + 0x48);

	printf("                      Start     End       Size\n");
	printf("Code segment 0        %08lX  %08lX  %08lX\n", rom_seg0->buf_addr, rom_seg0->buf_addr + rom_seg0->buf_size, rom_seg0->buf_size);
	printf("Code/data segment 1+  %08lX  %08lX  %08lX\n", rom_seg1->buf_addr, rom_seg1->buf_addr + rom_seg1->buf_size, rom_seg1->buf_size);
	printf("Data segment 0        %08lX  %08lX  %08lX\n", ram->buf_addr, ram->buf_addr + ram->buf_size, ram->buf_size);

	// Allocate space for the ROM
	uint32_t rom_size = rom_seg0->buf_size + rom_seg1->buf_size;
	uint8_t *rom_buf = malloc(sizeof(uint8_t) * rom_size);
	
	// ROM Segment 0
	if(ReadProcessMemory(proc, (LPCVOID)rom_seg0->buf_addr, rom_buf, rom_seg0->buf_size, NULL) == 0) {
		printf("Failed to read code segment 0 @ %08lX [%d]\n", rom_seg0->buf_addr, GetLastError());
		return -1;
	}

	// ROM Segment 1
	if(ReadProcessMemory(proc, (LPCVOID)rom_seg1->buf_addr, rom_buf + rom_seg0->buf_size, rom_seg1->buf_size, NULL) == 0) {
		printf("Failed to read code/data segment 1+ @ %lX [%d]\n", rom_seg1->buf_addr, GetLastError());
		return -1;
	}

	// RAM Segment 0
	uint8_t *ram_buf = malloc(sizeof(uint8_t) * ram->buf_size);
	if(ReadProcessMemory(proc, (LPCVOID)ram->buf_addr, ram_buf, ram->buf_size, NULL) == 0) {
		printf("Failed to read data segment 0 @ %lX [%d]\n", ram->buf_addr, GetLastError());
		return -1;
	}

	// Generate filenames
	char rom_filename[255];
	sprintf(rom_filename, "%s_code.bin", exeName);
	char ram_filename[255];
	sprintf(ram_filename, "%s_data.bin", exeName);

	// Write rom dump to file
	if (dump_rom) {
		FILE *f;
		f = fopen(rom_filename, "wb");
		fwrite(rom_buf, sizeof(uint8_t), rom_size, f);
		fclose(f);
		printf("Wrote code memory dump to %s\n", rom_filename);
	}
	
	// Write ram dump to file
	if (dump_ram) {
		FILE *f = fopen(ram_filename, "wb");
		fwrite(ram_buf, sizeof(uint8_t), ram->buf_size, f);
		fclose(f);
		printf("Wrote data segment 0 dump to %s\n", ram_filename);
	}
	

	printf("Done!\n");

	return 0;
}