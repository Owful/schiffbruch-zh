#include <windows.h>
#include "resource.h"
#include <filesystem>
#include <iostream>
#include <string>
int main()
{
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
    const auto game=std::filesystem::path(exe).parent_path()/L"Schiffbruch.exe";
    const auto module=LoadLibraryExW(game.c_str(),nullptr,LOAD_LIBRARY_AS_DATAFILE);
    if(!module){std::cerr<<"Cannot inspect compiled game resources\n";return 1;}
    struct Entry{unsigned id;const wchar_t* expected;};
    const Entry entries[]={
#include "localization_expected.inc"
    };
    int failures=0;
    for(const auto& e:entries)
    {
        wchar_t actual[2048]{};
        LoadStringW(module,e.id,actual,2048);
        if(std::wstring(actual)!=e.expected){std::cerr<<"Resource encoding mismatch at "<<e.id<<'\n';++failures;}
    }
    FreeLibrary(module);
    std::cout<<"Compiled Chinese resources: "<<std::size(entries)-failures<<" / "<<std::size(entries)<<" exact matches\n";
    return failures?1:0;
}
