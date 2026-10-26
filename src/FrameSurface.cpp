#include "FrameSurface.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

std::function<void()> NativeSurface::present;
NativeSurface::NativeSurface(int width,int height):m_width(width),m_height(height)
{
    if(width<=0||height<=0)throw std::runtime_error("Invalid surface size");
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    m_dc=CreateCompatibleDC(nullptr);
    m_bitmap=CreateDIBSection(m_dc,&info,DIB_RGB_COLORS,reinterpret_cast<void**>(&m_pixels),nullptr,0);
    if(!m_dc||!m_bitmap){if(m_bitmap)DeleteObject(m_bitmap);if(m_dc)DeleteDC(m_dc);throw std::runtime_error("Cannot allocate frame surface");}
    m_previous=SelectObject(m_dc,m_bitmap);
    std::fill_n(m_pixels,std::size_t(width)*height,0);
}
NativeSurface::~NativeSurface(){SelectObject(m_dc,m_previous);DeleteObject(m_bitmap);DeleteDC(m_dc);}
void NativeSurface::flush(){if(m_gdiDirty){GdiFlush();m_gdiDirty=false;}}
const std::uint8_t* NativeSurface::pixels(){flush();return reinterpret_cast<const std::uint8_t*>(m_pixels);}
HRESULT NativeSurface::GetPixelFormat(DDPIXELFORMAT* f)
{
    *f={};f->dwSize=sizeof(*f);f->dwFlags=DDPF_RGB;f->dwRGBBitCount=32;
    f->dwRBitMask=0xff0000;f->dwGBitMask=0xff00;f->dwBBitMask=0xff;return DD_OK;
}
HRESULT NativeSurface::GetSurfaceDesc(DDSURFACEDESC2* d)
{
    *d={};d->dwSize=sizeof(*d);d->dwWidth=m_width;d->dwHeight=m_height;
    d->lPitch=m_width*4;d->lpSurface=m_pixels;GetPixelFormat(&d->ddpfPixelFormat);return DD_OK;
}
HRESULT NativeSurface::Lock(RECT*,DDSURFACEDESC2* d,DWORD,void*){flush();return GetSurfaceDesc(d);}
HRESULT NativeSurface::Flip(void*,DWORD){if(present)present();return DD_OK;}
HRESULT NativeSurface::Blt(RECT* target,NativeSurface* source,RECT* sourceRect,DWORD flags,DDBLTFX* fx)
{
    RECT d=target?*target:RECT{0,0,m_width,m_height};
    const int dw=d.right-d.left,dh=d.bottom-d.top;
    if(dw<=0||dh<=0)return DD_OK;
    const int left=std::max(0L,d.left),right=std::min(LONG(m_width),d.right);
    const int top=std::max(0L,d.top),bottom=std::min(LONG(m_height),d.bottom);
    if(left>=right||top>=bottom)return DD_OK;
    flush();
    if(flags&DDBLT_COLORFILL)
    {
        for(int y=top;y<bottom;++y)std::fill(m_pixels+y*m_width+left,m_pixels+y*m_width+right,fx?fx->dwFillColor:0);
        return DD_OK;
    }
    if(!source)return E_INVALIDARG;
    source->flush();
    RECT s=sourceRect?*sourceRect:RECT{0,0,source->m_width,source->m_height};
    const int sw=s.right-s.left,sh=s.bottom-s.top;
    if(sw<=0||sh<=0)return DD_OK;
    const bool key=(flags&DDBLT_KEYSRC)!=0;
    // Self-blits are used by the original credits/zoom code and must not overwrite input.
    std::vector<std::uint32_t> copy;
    const std::uint32_t* input=source->m_pixels;
    if(source==this){copy.assign(input,input+std::size_t(m_width)*m_height);input=copy.data();}
    if(dw==sw&&dh==sh)
    {
        const int x0=std::max(left,int(d.left-s.left));
        const int x1=std::min(right,int(d.left+source->m_width-s.left));
        if(x0>=x1)return DD_OK;
        for(int y=top;y<bottom;++y){const int sy=s.top+y-d.top;if(sy<0||sy>=source->m_height)continue;
            auto* destination=m_pixels+y*m_width+x0;const auto* sourceRow=input+sy*source->m_width+s.left+x0-d.left;
            if(!key)std::memcpy(destination,sourceRow,(x1-x0)*4);
            else for(int x=0;x<x1-x0;++x)if((sourceRow[x]&0xffffff)!=source->m_key)destination[x]=sourceRow[x];}
        return DD_OK;
    }
    for(int y=top;y<bottom;++y)
    {
        const int sy=s.top+int(std::int64_t(y-d.top)*sh/dh);if(sy<0||sy>=source->m_height)continue;
        auto* row=m_pixels+y*m_width;
        for(int x=left;x<right;++x)
        {
            const int sx=s.left+int(std::int64_t(x-d.left)*sw/dw);if(sx<0||sx>=source->m_width)continue;
            const auto pixel=input[sy*source->m_width+sx];if(!key||(pixel&0xffffff)!=source->m_key)row[x]=pixel;
        }
    }
    return DD_OK;
}
