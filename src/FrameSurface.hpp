#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <ddraw.h>
#include <cstdint>
#include <functional>

// Cached 32-bit DIBs replace display-driver surfaces. GDI and pixel operations
// share the same memory, and the completed frame is uploaded to OpenGL once.
class NativeSurface
{
public:
    NativeSurface(int width,int height);
    ~NativeSurface();
    NativeSurface(const NativeSurface&)=delete;
    NativeSurface& operator=(const NativeSurface&)=delete;
    HRESULT Blt(RECT* destination,NativeSurface* source,RECT* sourceRect,DWORD flags,DDBLTFX* effect);
    HRESULT Lock(RECT*,DDSURFACEDESC2* desc,DWORD,void*);
    HRESULT Unlock(void*) { return DD_OK; }
    HRESULT GetDC(HDC* dc) { *dc=m_dc;return DD_OK; }
    HRESULT ReleaseDC(HDC) { m_gdiDirty=true;return DD_OK; }
    HRESULT GetSurfaceDesc(DDSURFACEDESC2* desc);
    HRESULT GetPixelFormat(DDPIXELFORMAT* format);
    HRESULT GetBltStatus(DWORD) { return DD_OK; }
    HRESULT Restore() { return DD_OK; }
    HRESULT Flip(void*,DWORD);
    void Release() { delete this; }
    void SetKey(std::uint32_t color) { m_key=color; }
    const std::uint8_t* pixels();
    int width() const {return m_width;}
    int height() const {return m_height;}
    static std::function<void()> present;
private:
    void flush();
    HDC m_dc=nullptr;
    HBITMAP m_bitmap=nullptr;
    HGDIOBJ m_previous=nullptr;
    std::uint32_t* m_pixels=nullptr;
    int m_width=0,m_height=0;
    std::uint32_t m_key=0xff00ff;
    bool m_gdiDirty=false;
};
