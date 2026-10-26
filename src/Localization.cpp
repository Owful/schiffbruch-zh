#include "Localization.hpp"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <unordered_map>
namespace Localization
{
    std::wstring Wide(int id)
    {
        static std::unordered_map<int,std::wstring> cache;
        if(auto it=cache.find(id);it!=cache.end())return it->second;
        wchar_t buffer[2048]{};const int n=LoadStringW(GetModuleHandleW(nullptr),id,buffer,2048);
        return cache[id]=n?std::wstring(buffer,n):L"暂无说明";
    }
    std::wstring ToWide(const std::string& s)
    {
        if(s.empty())return {};
        const int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);
        std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),out.data(),n);return out;
    }
    std::string Utf8(const std::wstring& s)
    {
        const int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);
        std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;
    }
    int Load(int id,char* buffer,int capacity)
    {
        if(!buffer||capacity<=0)return 0;const auto str=Utf8(Wide(id));
        const auto n=std::min<std::size_t>(str.size(),capacity-1);std::memcpy(buffer,str.data(),n);buffer[n]=0;return int(n);
    }
}
