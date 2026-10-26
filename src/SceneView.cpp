#include "SceneView.hpp"
#include "PaceRules.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include "Renderer.hpp"
namespace SceneView
{
    float Scale(){return 1.f+PaceRules::zoomStep*.1f;}
    Rect Crop(){const int w=int(960/Scale()),h=int(592/Scale());return {(960-w)/2,64+(592-h)/2,(960-w)/2+w,64+(592-h)/2+h};}
    Point ToScene(int x,int y){const auto r=Crop();return {r.left+x*(r.right-r.left)/960,r.top+(y-64)*(r.bottom-r.top)/592};}
    Point ToScreen(int x,int y){const auto r=Crop();return {(x-r.left)*960/(r.right-r.left),64+(y-r.top)*592/(r.bottom-r.top)};}
    void Zoom(float delta,int x,int y)
    {
        const auto before=ToScene(x,y);const int steps=delta>0?1:delta<0?-1:0;PaceRules::zoomStep=unsigned(std::clamp(int(PaceRules::zoomStep)+steps,0,8));
        const auto after=ToScene(x,y);Camera.x+=short(before.x-after.x);Camera.y+=short(before.y-after.y);Renderer::LimitScroll();
    }
    void Apply()
    {
        if(!lpDDSBack||PaceRules::zoomStep==0)return;static NativeSurface copy(960,592);const auto r=Crop();
        RECT source{r.left,r.top,r.right,r.bottom},temporary{0,0,r.right-r.left,r.bottom-r.top},target{0,64,960,656};
        copy.Blt(&temporary,lpDDSBack,&source,0,nullptr);lpDDSBack->Blt(&target,&copy,&temporary,0,nullptr);
    }
}
