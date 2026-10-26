#pragma once
#include "FrameSurface.hpp"
NativeSurface* DDLoadBitmap(IDirectDraw4*,LPCSTR name,int width,int height);
HRESULT DDSetColorKey(NativeSurface* surface,COLORREF color);
