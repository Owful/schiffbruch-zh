#include "PixelFormat.hpp"
#include "ArtPalette.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
int main()
{
    int failures = 0;
    auto check = [&](bool condition, const char* name) { if(!condition){ std::cerr << "FAIL: " << name << '\n'; ++failures; } };
    using namespace PixelFormat;
    const auto p = pack(217,143,69,0xff0000,0xff00,0xff);
    check(p == 0xd98f45 && unpack(p,0xff0000) == 217 && unpack(p,0xff00) == 143, "32-bit RGB retains full red channel");
    check(pack(255,255,255,0xf800,0x7e0,0x1f) == 65535, "RGB565 white");
    check(unpack(0xf800,0xf800) == 255 && unpack(0x7c00,0x7c00) == 255, "16-bit channels expand to full range");
    check(ArtPalette::grade(255,0,255)==std::array<std::uint8_t,3>{255,0,255},"palette preserves magenta transparency");
    check(ArtPalette::grade(0,0,0)==std::array<std::uint8_t,3>{0,0,0},"palette preserves black details");
    const auto sea=ArtPalette::grade(0,0,255),sand=ArtPalette::grade(255,255,0);
    check(sea[1]>70&&sea[2]<180&&sand[2]>100,"terrain grading softens blue ocean and yellow sand");
    for (unsigned bits : {16u,24u,32u})
    {
        std::array<std::uint8_t,64> buffer{};
        write(buffer.data(),16,1,1,bits,p);
        check(read(buffer.data(),16,1,1,bits) == (bits == 16 ? p & 65535 : p), "pixel round trip respects bit depth");
        check(read(buffer.data(),16,0,1,bits) == 0 && read(buffer.data(),16,2,1,bits) == 0, "neighbors are not overwritten");
        check(buffer[15] == 0 && buffer[31] == 0 && buffer[32] == 0, "row padding and following row remain intact");
    }
    if(failures) return EXIT_FAILURE;
    std::cout << "Pixel format: all checks passed.\n";
}
