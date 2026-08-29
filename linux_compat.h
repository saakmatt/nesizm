#pragma once
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdint>

#define Sleep(ms) usleep((ms)*1000)
#define OutputDebugString(s) fputs((s), stderr)
#define sprintf_s(buf, sz, ...) snprintf((buf), (sz), __VA_ARGS__)

// --- Win32 input / string shims -------------------------------------------
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_DIVIDE 0x6F
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PLUS 0xBB
#define VK_OEM_PERIOD 0xBE
#define VK_DELETE 0x2E
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B
#define VK_TAB 0x09
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_SPACE 0x20
#define VK_BACK 0x08
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_OEM_1 0xBA

inline void DebugBreak() { __builtin_trap(); }

extern void* GWnd;
extern unsigned char g_keyState[256];
inline void* GetFocus() { return GWnd; }
inline short GetAsyncKeyState(int vk) {
	return (vk >= 0 && vk < 256 && g_keyState[vk]) ? (short)0x8000 : 0;
}

typedef void* HFONT;
typedef unsigned long DWORD;

#define CP_ACP 0
typedef unsigned short* LPWSTR;
inline int WideCharToMultiByte(unsigned, unsigned, const unsigned short* src, int,
                               char* dst, int cap, void*, void*) {
	int i = 0; while (i < cap - 1 && src[i]) { dst[i] = (char)src[i]; i++; } dst[i] = 0; return i;
}
inline int MultiByteToWideChar(unsigned, unsigned, const char* src, int,
                               unsigned short* dst, int cap) {
	int i = 0; while (i < cap - 1 && src[i]) { dst[i] = (unsigned short)src[i]; i++; } dst[i] = 0; return i;
}

typedef void* HDC;
typedef void* HWND;
typedef long long LONGLONG;
typedef union { struct { unsigned int LowPart; int HighPart; }; long long QuadPart; } LARGE_INTEGER;

inline int QueryPerformanceCounter(LARGE_INTEGER* p) {
	struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
	p->QuadPart = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
	return 1;
}
inline int QueryPerformanceFrequency(LARGE_INTEGER* p) { p->QuadPart = 1000000000LL; return 1; }

// Win32 message pump shims (no-ops for a headless build)
typedef struct { unsigned int message; void* hwnd; } MSG;
#define PM_REMOVE 1
inline int PeekMessage(MSG*, void*, unsigned, unsigned, unsigned) { return 0; }
inline int TranslateMessage(const MSG*) { return 0; }
inline int DispatchMessage(const MSG*) { return 0; }
#define WM_QUIT 0x0012

inline unsigned long long GetTickCount64() {
	struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}
typedef struct { unsigned short wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;
inline void GetLocalTime(SYSTEMTIME* st) {
	time_t t = time(0); struct tm lt; localtime_r(&t, &lt);
	st->wYear = lt.tm_year + 1900; st->wMonth = lt.tm_mon + 1; st->wDayOfWeek = lt.tm_wday;
	st->wDay = lt.tm_mday; st->wHour = lt.tm_hour; st->wMinute = lt.tm_min;
	st->wSecond = lt.tm_sec; st->wMilliseconds = 0;
}

#define MB_OK 0
#define NULL_HWND 0
inline int MessageBox(void*, const char* text, const char* cap, unsigned) {
	fprintf(stderr, "[%s] %s\n", cap, text); return 0;
}
