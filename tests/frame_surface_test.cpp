#include "FrameSurface.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
int main()
{
    int failures=0;auto check=[&](bool c,const char* name){if(!c){++failures;std::cerr<<"FAIL: "<<name<<'\n';}};
    NativeSurface src(4,4),dst(8,8);DDBLTFX fx{};fx.dwFillColor=0x123456;
    src.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx);
    RECT rectangle{2,2,6,6};dst.Blt(&rectangle,&src,nullptr,0,nullptr);
    DDSURFACEDESC2 d{};dst.Lock(nullptr,&d,0,nullptr);auto* pixels=static_cast<unsigned*>(d.lpSurface);
    check(pixels[2*8+2]==0x123456&&pixels[1*8+2]==0,"opaque blit respects destination offset");
    fx.dwFillColor=0xff00ff;src.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx);src.SetKey(0xff00ff);
    dst.Blt(&rectangle,&src,nullptr,DDBLT_KEYSRC,nullptr);check(pixels[2*8+2]==0x123456,"transparent pixels preserve the destination");
    RECT clipped{-2,-2,2,2};dst.Blt(&clipped,&src,nullptr,0,nullptr);check(pixels[0]==0xff00ff,"clipped blits remain in bounds");
    HDC dc=nullptr;src.GetDC(&dc);SetPixel(dc,0,0,RGB(10,20,30));src.ReleaseDC(dc);
    src.Lock(nullptr,&d,0,nullptr);check(static_cast<unsigned*>(d.lpSurface)[0]==0x0a141e,"GDI and CPU access share the same pixel storage");
    NativeSurface large(1280,720),sprite(48,64);fx.dwFillColor=0x234f51;large.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx);sprite.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fx);
    const auto begin=std::chrono::steady_clock::now();
    for(int frame=0;frame<120;++frame){large.Blt(nullptr,&large,nullptr,0,nullptr);for(int i=0;i<200;++i){RECT r{(i*53)%1200,(i*29)%650,(i*53)%1200+48,(i*29)%650+64};large.Blt(&r,&sprite,nullptr,DDBLT_KEYSRC,nullptr);}}
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/120;
    std::cout<<"1280x720 composition + 200 sprites: "<<ms<<" ms/frame\n";
    check(ms<100,"composition avoids display-driver stalls");
    if(failures)return EXIT_FAILURE;
}
