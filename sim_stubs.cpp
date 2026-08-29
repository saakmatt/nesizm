/*******************************************************************
  Stubs for PrizmSDK utilities whose real implementations are
  SH7305-specific and cannot build on x86:

    Ptune2_*  - CPG clock register access (ptune2_simple/Ptune2_direct.c)
    snd*      - calculator DAC audio     (snd/snd_prizm.cpp)
    CalcType_Draw - Prizm font blitter   (calctype/src/calctype_prizm.c)

  zx7 is portable and should be built from its real sources instead.
 *******************************************************************/

#include <stdio.h>
#include <string.h>
#include "calctype.h"

// --- Ptune2 (declared extern "C" in Ptune2_direct.h) ------------------------
extern "C" {
	void Ptune2_LoadSetting(int setting) { }
	void Ptune2_LoadBackup() { }
	void Ptune2_SaveBackup() { }
	int  Ptune2_GetPLLFreq() { return 118; }
	int  Ptune2_GetPFCDiv()  { return 1; }
}

// --- sound (plain C++ linkage in snd.h) -------------------------------------
bool sndInit()      { return true; }
void sndCleanup()   { }
void sndUpdate()    { }
void sndVolumeUp()  { }
void sndVolumeDown(){ }

// --- calctype ---------------------------------------------------------------
//void CalcType_Draw(const CalcTypeFont* font, const char* text, int x, int y,
//                   unsigned short color, unsigned char* vram, unsigned int pitch) {
//	if (text) fprintf(stderr, "%s\n", text);
//}

//unsigned int CalcType_Width(const CalcTypeFont* font, const char* text) {
//	return text ? (unsigned int)(strlen(text) * 8) : 0;
//}
