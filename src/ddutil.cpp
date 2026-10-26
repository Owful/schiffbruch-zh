#include "ddutil.h"
NativeSurface* DDLoadBitmap(IDirectDraw4*,LPCSTR name,int width,int height)
{
    HBITMAP bitmap=static_cast<HBITMAP>(LoadImageA(GetModuleHandleW(nullptr),name,IMAGE_BITMAP,width,height,LR_CREATEDIBSECTION));
    if(!bitmap)return nullptr;
    BITMAP info{};GetObject(bitmap,sizeof(info),&info);
    auto* surface=new NativeSurface(info.bmWidth,info.bmHeight);
    HDC from=CreateCompatibleDC(nullptr),to=nullptr;auto old=SelectObject(from,bitmap);
    surface->GetDC(&to);BitBlt(to,0,0,info.bmWidth,info.bmHeight,from,0,0,SRCCOPY);surface->ReleaseDC(to);
    SelectObject(from,old);DeleteDC(from);DeleteObject(bitmap);return surface;
}
HRESULT DDSetColorKey(NativeSurface* surface,COLORREF color)
{
    if(!surface)return E_FAIL;
    surface->SetKey((GetRValue(color)<<16)|(GetGValue(color)<<8)|GetBValue(color));return DD_OK;
}
