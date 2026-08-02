#pragma once
#include "exportdef.h"
#include "wrect.h"
#include "const.h"

void OrthoDraw(int spr, int mode, float r, float g, float b, float a);
void OrthoDrawMirroredQuarter(int spr, int mode, float r, float g, float b, float a);

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};
