#include "ContextBar.hpp"
#include "PointerHint.hpp"
#include <cstdlib>
#include <iostream>

int main()
{
    int failures=0;
    const auto check=[&](bool okay,const char* message){if(!okay){std::cerr<<message<<'\n';++failures;}};
    ContextBar::State row;std::array<bool,ContextBar::Count> visible{};
    visible[ContextBar::Interact]=visible[ContextBar::Water]=visible[ContextBar::Feed]=true;
    row.update(visible,.03f);
    check(!row.clickable(ContextBar::Water),"newly fading buttons must not accept clicks");
    for(int i=0;i<30;++i)row.update(visible,.02f);
    check(row.clickable(ContextBar::Feed),"settled button becomes clickable");
    const float oldFeedX=row.slots[ContextBar::Feed].x;
    visible[ContextBar::Water]=false;row.update(visible,.02f);
    check(!row.clickable(ContextBar::Water)&&row.slots[ContextBar::Water].alpha>0,"disappearing water disables immediately but fades");
    check(row.slots[ContextBar::Feed].x==oldFeedX,"outgoing button reserves space during fade");
    for(int i=0;i<9;++i)row.update(visible,.02f);
    check(!row.clickable(ContextBar::Feed),"button moving into freed space cannot be misclicked");
    for(int i=0;i<30;++i)row.update(visible,.02f);
    check(row.slots[ContextBar::Feed].x==18+ContextBar::State::Width+ContextBar::State::Gap&&row.clickable(ContextBar::Feed),"remaining button packs left and re-enables");
    visible.fill(true);for(int i=0;i<50;++i)row.update(visible,.02f);
    check(row.slots.back().x+ContextBar::State::Width<960,"even a fully transitioning row fits outside the sidebar");
    visible.fill(false);for(int i=0;i<15;++i)row.update(visible,.02f);
    for(int i=0;i<ContextBar::Count;++i)check(row.slots[i].alpha==0&&!row.clickable(i),"hidden buttons cannot retain hit areas");
    for(int x:{0,480,959})for(int y:{64,350,655})for(int width:{62,86})
    {
        const auto r=PointerHint::Place(x,y,width,20);
        check(r.x>=0&&r.y>=64&&r.x+r.w<=960&&r.y+r.h<=656,"pointer hint remains fully visible at map edges");
        check(x==959?r.x+r.w<x:r.x>x,"hint avoids cursor and prefers its right side");
    }
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
