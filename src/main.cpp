#include "Application.hpp"
#include <filesystem>
#include <system_error>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Assets and saves follow the executable even when launched from a shortcut.
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
    if (length > 0 && length < 32768)
    {
        std::error_code error;
        std::filesystem::current_path(std::filesystem::path(executable).parent_path(), error);
    }
    try
    {
        Application app("Schiffbruch", hInstance);std::string command(lpCmdLine);
#ifdef SCHIFFBRUCH_QA_PREVIEW
        if(command.empty())command="--preview-workshop";
#endif
        if(command=="--benchmark")app.benchmark();
        else if(command=="--revision-check"){if(!app.checkRevision())return 2;}
        else{if(command=="--preview-workshop")app.prepareWorkshopPreview();else if(command=="--preview-rain"||command=="--preview-danger")app.prepareExperiencePreview(command=="--preview-danger");app.run();}
    }
    catch(const std::exception&) { MessageBoxW(nullptr,L"游戏启动失败。请将完整游戏包解压后，再运行开始游戏。",L"余岸 Stillshore",MB_OK|MB_ICONERROR); return 1; }
    return 0;
}
