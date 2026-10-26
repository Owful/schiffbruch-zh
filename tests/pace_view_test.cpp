#include "PaceRules.hpp"
#include "SceneView.hpp"
#include "headers.hpp"
#include "extern.hpp"
#include <sstream>
#include <iostream>
#include <cmath>
namespace Renderer {void LimitScroll(){}}
int main()
{
    int failures=0;const auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<"FAIL: "<<name<<'\n';}};
    PaceRules::reset();int elapsed=0;for(int i=0;i<1001;++i)elapsed+=PaceRules::workMinutes(1);
    check(elapsed==500&&PaceRules::halfMinute==1,"tiny actions retain fractional time instead of becoming free");
    check(PaceRules::resourceChange(-10)==-5&&PaceRules::resourceChange(25)==25,"losses are halved and food or water gains remain unchanged");
    PaceRules::zoomStep=8;std::stringstream saved(std::ios::in|std::ios::out|std::ios::binary);PaceRules::save(saved);check(saved.str().size()==6,"pace footer has a stable optional layout");
    PaceRules::reset();check(PaceRules::load(saved)&&PaceRules::halfMinute==1&&PaceRules::zoomStep==8,"save preserves fractional time and zoom");
    std::stringstream truncated(saved.str().substr(0,5));check(!PaceRules::load(truncated)&&PaceRules::zoomStep==8,"truncated pace data does not overwrite valid settings");
    for(unsigned zoom=0;zoom<=8;++zoom)
    {
        PaceRules::zoomStep=zoom;
        for(int x:{0,123,480,959})for(int y:{64,117,360,655}){const auto p=SceneView::ToScene(x,y),back=SceneView::ToScreen(p.x,p.y);check(std::abs(back.x-x)<=2&&std::abs(back.y-y)<=2,"scene drawing and click positions use the same inverse transform");}
    }
    PaceRules::reset();Camera={1000,1000};const auto before=SceneView::ToScene(624,333);const int oldX=Camera.x,oldY=Camera.y;SceneView::Zoom(1,624,333);const auto after=SceneView::ToScene(624,333);
    check(before.x+oldX==after.x+Camera.x&&before.y+oldY==after.y+Camera.y,"zoom preserves the world position under the cursor");
    NativeSurface frame(1280,720);lpDDSBack=&frame;DDBLTFX fill{};fill.dwFillColor=0x123456;frame.Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL,&fill);SceneView::Apply();
    DDSURFACEDESC2 lock{};frame.Lock(nullptr,&lock,0,nullptr);const auto* pixels=static_cast<const unsigned*>(lock.lpSurface);check(pixels[10*1280+10]==0x123456&&pixels[300*1280+1000]==0x123456,"zoom never changes top bar and sidebar pixels");frame.Unlock(nullptr);lpDDSBack=nullptr;
    if(failures)return 1;std::cout<<"Pace, optional save footer, view bounds and pointer mapping passed.\n";
}
