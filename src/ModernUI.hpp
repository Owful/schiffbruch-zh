#pragma once
#include <string>
namespace ModernUI
{
    void Layout();
    void DrawPanel();
    void DrawStart();
    void DrawCredits();
    void DrawDialog();
    void DrawOverlay();
    bool HandleClick(int x,int y);
    bool HandleKey(int key);
    bool HandleWheel(float delta,int x,int y);
    bool OverlayActive();
    bool ShouldQuit();
    void Text(int x,int y,const std::wstring& text,int size=16,unsigned color=0);
    void StartGame(bool fresh);
    void Interact();
    void CheckSafety();
    bool DuskNoticeVisible();
}
