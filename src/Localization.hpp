#pragma once
#include <string>
namespace Localization
{
    std::wstring Wide(int resource);
    std::wstring ToWide(const std::string& utf8);
    std::string Utf8(const std::wstring& wide);
    int Load(int resource,char* buffer,int capacity);
}
