/*******************************************************************
  POSIX replacement for the Win32 sim_file.cpp in PrizmSDK/utils/winsim.
  Same Bfile and MCS surface, implemented with open/read/write and glob.

  Paths:
    \\fls0\<name>  ->  $HOME/Prizm/ROM/<name>
    \\dev0\<name>  ->  ../<name>   (project directory, as in winsim)
    MCS items      ->  $HOME/Prizm/RAM/<dir>/<item>
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "prizmsim.h"
#include "prizmfont.h"

static char romPath[256] = { 0 };
static char ramPath[256] = { 0 };
static char devPath[256] = { 0 };

void InvalidateFullFile();

static void ResolvePaths() {
	if (romPath[0] == 0) {
		const char* home = getenv("HOME");
		if (!home) home = ".";

		char prizmPath[256];
		snprintf(prizmPath, sizeof(prizmPath), "%s/Prizm", home);
		mkdir(prizmPath, 0755);

		char d[300];
		snprintf(d, sizeof(d), "%s/ROM", prizmPath); mkdir(d, 0755);
		snprintf(romPath, sizeof(romPath), "%s/ROM/", prizmPath);

		snprintf(d, sizeof(d), "%s/RAM", prizmPath); mkdir(d, 0755);
		snprintf(ramPath, sizeof(ramPath), "%s/RAM/", prizmPath);

		strcpy(devPath, "../");
	}
}

static void WideToChar(const unsigned short* src, char* dst, int cap) {
	int i = 0;
	while (i < cap - 1 && src[i]) { dst[i] = (char)src[i]; i++; }
	dst[i] = 0;
}

static void CharToWide(const char* src, unsigned short* dst, int cap) {
	int i = 0;
	while (i < cap - 1 && src[i]) { dst[i] = (unsigned short)(unsigned char)src[i]; i++; }
	dst[i] = 0;
}

static bool ResolveROMPath(const unsigned short* prizmPath, char* intoPath) {
	ResolvePaths();

	char filename[256];
	WideToChar(prizmPath, filename, sizeof(filename));

	for (char* c = filename; *c; c++) if (*c == '\\') *c = '/';

	if (!strncmp(filename, "//fls0/", 7)) {
		memmove(&filename[0], &filename[7], sizeof(filename) - 7);
		strcpy(intoPath, romPath);
		strcat(intoPath, filename);
		return true;
	}

	if (!strncmp(filename, "//dev0/", 7)) {
		memmove(&filename[0], &filename[7], sizeof(filename) - 7);
		strcpy(intoPath, devPath);
		strcat(intoPath, filename);
		return true;
	}

	return false;
}

int Bfile_OpenFile_OS(const unsigned short *filenameW, int mode, int null) {
	InvalidateFullFile();

	char docFilename[512];
	if (!ResolveROMPath(filenameW, docFilename)) return -5;

	int flags;
	switch (mode) {
		case READ:      flags = O_RDONLY; break;
		case WRITE:     flags = O_WRONLY; break;
		case READWRITE: flags = O_RDWR;   break;
		default:        return -1;
	}

	int fd = open(docFilename, flags);
	if (fd < 0) return -1;
	return fd;
}

int Bfile_CreateEntry_OS(const unsigned short*filename, int mode, size_t *size) {
	InvalidateFullFile();

	char docFilename[512];
	if (!ResolveROMPath(filename, docFilename)) return -5;

	if (mode == CREATEMODE_FOLDER) {
		return mkdir(docFilename, 0755) == 0 ? 0 : -1;
	}
	else if (mode == CREATEMODE_FILE) {
		int fd = open(docFilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) return -1;

		int zero = 0;
		for (unsigned int i = 0; i < *size; i += 4) {
			if (write(fd, &zero, 4) != 4) break;
		}
		close(fd);
		return 0;
	}

	return -1;
}

int Bfile_SeekFile_OS(int handle, int pos) {
	return (int)lseek(handle, pos, SEEK_SET);
}

int Bfile_ReadFile_OS(int handle, void *buf, int size, int readpos) {
	if (readpos >= 0) lseek(handle, readpos, SEEK_SET);
	ssize_t got = read(handle, buf, size);
	return (int)(got < 0 ? 0 : got);
}

int Bfile_WriteFile_OS(int handle, const void* buf, int size) {
	ssize_t put = write(handle, buf, size);
	if (put != size) return -1;
	return (int)lseek(handle, 0, SEEK_CUR);
}

int Bfile_GetFileSize_OS(int handle) {
	struct stat st;
	if (fstat(handle, &st) != 0) return -1;
	return (int)st.st_size;
}

int Bfile_TellFile_OS(int handle) {
	return (int)lseek(handle, 0, SEEK_CUR);
}

int Bfile_CloseFile_OS(int handle) {
	InvalidateFullFile();
	close(handle);
	return 0;
}

int Bfile_DeleteEntry(const unsigned short *filename) {
	InvalidateFullFile();

	char docFilename[512];
	if (!ResolveROMPath(filename, docFilename)) return -5;

	return unlink(docFilename) == 0 ? 0 : -1;
}

struct simFindData {
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;

	glob_t g;
	size_t index;

	void Fill() {
		struct stat st;
		if (index < g.gl_pathc && stat(g.gl_pathv[index], &st) == 0) {
			fsize = (unsigned long)st.st_size;
			dsize = (unsigned long)st.st_size;
		} else {
			fsize = dsize = 0;
		}
	}

	const char* CurrentName() {
		if (index >= g.gl_pathc) return "";
		const char* full = g.gl_pathv[index];
		const char* slash = strrchr(full, '/');
		return slash ? slash + 1 : full;
	}
};

#define MAX_FIND_HANDLES 8
static simFindData* g_findHandles[MAX_FIND_HANDLES] = { 0 };

int Bfile_FindFirst(const char *pathAsWide, int *FindHandle, char *foundfile, void *fileinfo) {
	InvalidateFullFile();

	char docPath[512];
	*FindHandle = 0;

	if (!ResolveROMPath((const unsigned short*)pathAsWide, docPath)) return -5;

	simFindData* data = new simFindData;
	memset(&data->g, 0, sizeof(data->g));
	data->index = 0;

	if (glob(docPath, 0, NULL, &data->g) != 0 || data->g.gl_pathc == 0) {
		globfree(&data->g);
		delete data;
		return -1;
	}

	data->Fill();
	CharToWide(data->CurrentName(), (unsigned short*)foundfile, 256);
	if (fileinfo) memcpy(fileinfo, data, 20);
		int slot = -1;
	for (int i = 1; i < MAX_FIND_HANDLES; i++) {
		if (!g_findHandles[i]) { slot = i; break; }
	}
	if (slot < 0) { globfree(&data->g); delete data; return -1; }
	g_findHandles[slot] = data;
	*FindHandle = slot;
  return 0;
}

int Bfile_FindNext(int FindHandle, char *foundfile, char *fileinfo) {
	InvalidateFullFile();

  simFindData* data = (FindHandle > 0 && FindHandle < MAX_FIND_HANDLES)
	                    ? g_findHandles[FindHandle] : 0;
	if (!data) return -1;

	data->index++;
	if (data->index >= data->g.gl_pathc) return -16;

	data->Fill();
	CharToWide(data->CurrentName(), (unsigned short*)foundfile, 256);
	if (fileinfo) memcpy(fileinfo, data, 20);
	return 0;
}

int Bfile_FindClose(int FindHandle) {
	InvalidateFullFile();

	simFindData* data = (FindHandle > 0 && FindHandle < MAX_FIND_HANDLES)
	                    ? g_findHandles[FindHandle] : 0;
  if (data) {
		globfree(&data->g);
		delete data;
    g_findHandles[FindHandle] = 0;
	}
	return 0;
}

int MCS_CreateDirectory(unsigned char*dir) {
	if (!dir || dir[0] == 0) return 0xF0;

	ResolvePaths();

	char fullPath[512];
	strcpy(fullPath, ramPath);
	strcat(fullPath, (const char*)dir);

	if (mkdir(fullPath, 0755) != 0) {
		if (errno == EEXIST) return 0x42;
		return 0x43;
	}
	return 0;
}

static char curReadFile[512] = { 0 };

int MCSGetData1(int offset, int len_to_copy, void*buffer) {
	if (curReadFile[0] == 0) return -1;

	int fd = open(curReadFile, O_RDONLY);
	if (fd < 0) { curReadFile[0] = 0; return -1; }

	lseek(fd, offset, SEEK_SET);
	ssize_t got = read(fd, buffer, len_to_copy);
	close(fd);

	if (got != len_to_copy) return -1;
	return 0;
}

int MCSGetDlen2(unsigned char*dir, unsigned char*item, int*data_len) {
	ResolvePaths();
	snprintf(curReadFile, sizeof(curReadFile), "%s%s/%s", ramPath, (const char*)dir, (const char*)item);

	struct stat st;
	if (stat(curReadFile, &st) != 0) { curReadFile[0] = 0; return -1; }

	*data_len = (int)st.st_size;
	return 0;
}

int MCS_WriteItem(unsigned char*dir, unsigned char*item, short itemtype, int data_length, int buffer) {
	ResolvePaths();

	if (data_length % 4 != 0) return 0x10;

	char fullPath[512];
	snprintf(fullPath, sizeof(fullPath), "%s%s/%s", ramPath, (const char*)dir, (const char*)item);

	int fd = open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) return 0x40;

	ssize_t written = write(fd, (const void*)(size_t)buffer, data_length);
	close(fd);

	if (written != data_length) return 0x11;
	return 0;
}

unsigned char* fullFile = nullptr;
unsigned int fullFileSize = 0;
int curBlockedFile = 0;

void ResolveFullFile(int handle) {
	curBlockedFile = handle;

	int tellPos = Bfile_TellFile_OS(handle);
	Bfile_SeekFile_OS(handle, 0);
	fullFileSize = Bfile_GetFileSize_OS(handle);
	fullFile = (unsigned char*)malloc(fullFileSize + 4095);
	Bfile_ReadFile_OS(handle, fullFile, fullFileSize, 0);
	Bfile_SeekFile_OS(handle, tellPos);
}

void InvalidateFullFile() {
	if (fullFile) {
		curBlockedFile = 0;
		memset(fullFile, 0, fullFileSize);
		free(fullFile);
		fullFileSize = 0;
		fullFile = nullptr;
	}
}

int Bfile_GetBlockAddress(int handle, int blockAddress, unsigned char** outPtr) {
	Assert(blockAddress % 4096 == 0);
	Assert(curBlockedFile == 0 || curBlockedFile == handle);

	if (!fullFile) ResolveFullFile(handle);

	Assert((unsigned int)blockAddress < fullFileSize);

	*outPtr = fullFile + blockAddress;
	return 0;
}
