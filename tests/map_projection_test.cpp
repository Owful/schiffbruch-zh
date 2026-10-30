#include "MapProjection.hpp"
#include <iostream>
#include <cmath>
int main()
{
    int failures=0;const auto check=[&](bool okay,const char* message){if(!okay){++failures;std::cerr<<message<<'\n';}};
    for(int x=0;x<60;++x)for(int y=0;y<60;++y)for(int h=0;h<=3;++h)
    {
        const auto p=MapProjection::TileOrigin(x,y,h);
        check(p.x==54/2*60+32+x*54/2-y*44/2-6*y+x&&p.y==x*54/2+y*44/2-16*h-13*x-8*y,"unified projection preserves legacy tile and save coordinates");
        const auto c=MapProjection::TileCenter(x,y);const auto grid=MapProjection::Grid(c.x,c.y);
        check(std::abs(grid.x-x)<.0001&&std::abs(grid.y-y)<.0001,"isometric world to grid conversion is reversible");
    }
    const auto origin=MapProjection::TileCenter(0,0),right=MapProjection::TileCenter(59,0),left=MapProjection::TileCenter(0,59),bottom=MapProjection::TileCenter(59,59);
    check(origin.x==bottom.x&&right.y==left.y&&right.x>origin.x&&left.x<origin.x&&bottom.y>right.y,"square grid forms a symmetric diamond");
    for(const auto frame:{MapProjection::Frame::Fit(4,4,112,112),MapProjection::Frame::Fit(134,172,1012,468)})
        for(int x:{0,15,30,59})for(int y:{0,15,30,59})
        {
            const auto world=MapProjection::TileCenter(x,y);const auto screen=frame.project(world.x,world.y);const auto back=frame.unproject(screen.x,screen.y);
            check(std::abs(world.x-back.x)<=.51/frame.scale&&std::abs(world.y-back.y)<=.51/frame.scale,"minimap and overview clicks invert the rendering transform");
            check(MapProjection::Inside(back.x,back.y),"all tile centers stay within the diamond at both scales");
        }
    check(!MapProjection::Inside(0,0)&&!MapProjection::Inside(3300,0)&&!MapProjection::Inside(0,1690),"blank rectangle corners are outside the map");
    return failures?1:0;
}
