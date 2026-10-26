#include "WeatherEffects.hpp"
#include "FrameSurface.hpp"
#include "extern.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace WeatherEffects
{
    namespace
    {
        double random(unsigned seed){seed^=seed>>16;seed*=0x7feb352du;seed^=seed>>15;seed*=0x846ca68bu;seed^=seed>>16;return double(seed&0xffffff)/0x1000000;}
    }
    void DrawRain(NativeSurface& surface,ExpeditionRules::Weather weather,double seconds)
    {
        if(weather==ExpeditionRules::Weather::Clear)return;
        const int width=rcSpielflaeche.right,height=rcSpielflaeche.bottom;
        static std::unique_ptr<NativeSurface> layer;
        if(!layer||layer->width()!=width||layer->height()!=height)layer=std::make_unique<NativeSurface>(width,height);
        DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);layer->Lock(nullptr,&desc,0,nullptr);
        auto* pixels=static_cast<std::uint32_t*>(desc.lpSurface);std::fill(pixels,pixels+width*height,0u);
        const auto* scene=surface.pixels();
        const auto dot=[&](int x,int y,int alpha)
        {
            if(x<0||x>=width||y<rcSpielflaeche.top||y>=height)return;
            const auto* ground=scene+4*(y*surface.width()+x);
            // Keep the unrevealed map dark and avoid rain over HUD overlays.
            if(int(ground[0])+ground[1]+ground[2]<140)return;
            alpha=std::clamp(alpha,0,125);
            auto& p=pixels[y*width+x];if(int(p>>24)>=alpha)return;
            p=(std::uint32_t(alpha)<<24)|((181*alpha/255)<<16)|((205*alpha/255)<<8)|(213*alpha/255);
        };
        const bool storm=weather==ExpeditionRules::Weather::Storm;
        const int count=storm?250:125;
        const double wind=(storm?16:6)+std::sin(seconds*.43)*(storm?7:3);
        for(int i=0;i<count;++i)
        {
            const unsigned seed=unsigned(i+1)*127u;
            const double depth=.35+random(seed)*.65;
            const double duration=.65+random(seed+1)*.55;
            const double cycle=seconds/duration+random(seed+2)*10;
            const double phase=cycle-std::floor(cycle);
            const auto generation=unsigned(std::floor(cycle));
            // Each independent drop has its own depth, destination and lifetime.
            const double endX=random(seed+generation*937u+3)*width;
            const double endY=rcSpielflaeche.top+random(seed+generation*541u+4)*(height-rcSpielflaeche.top);
            const double travel=90+depth*150;
            const double x=endX-wind*(1-phase),y=endY-travel*(1-phase);
            if(phase<.88)
            {
                const int length=int(3+depth*(storm?13:9));
                const double fade=std::min({1.0,phase*9,(.88-phase)*7});
                for(int k=0;k<length;++k)
                {
                    const int alpha=int((storm?105:75)*depth*fade*(.3+.7*k/length));
                    dot(int(x-wind/travel*k),int(y-k),alpha);
                }
            }
            else if(depth>.65)
            {
                const double splash=(phase-.88)/.12;const int radius=1+int(splash*3);
                const int alpha=int(35*(1-splash));
                dot(int(endX)-radius,int(endY),alpha);dot(int(endX)+radius,int(endY),alpha);
                dot(int(endX),int(endY)-1,alpha/2);
            }
        }
        layer->Unlock(nullptr);
        HDC destination=nullptr,source=nullptr;surface.GetDC(&destination);layer->GetDC(&source);
        BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};AlphaBlend(destination,0,0,width,height,source,0,0,width,height,blend);
        layer->ReleaseDC(source);surface.ReleaseDC(destination);
    }
}
