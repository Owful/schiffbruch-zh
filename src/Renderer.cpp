#include "Localization.hpp"
#include "Renderer.hpp"

#include "Action.hpp"
#include "Direct.hpp"
#include "Game.hpp"
#include "Math.hpp"
#include "Menu.hpp"
#include "Sound.hpp"
#include "Routing.hpp"
#include "World.hpp"
#include "Expedition.hpp"
#include "PixelFormat.hpp"
#include "ArtPalette.hpp"
#include "NativeDisplay.hpp"
#include "ModernUI.hpp"
#include "SceneView.hpp"
#include "Survival.hpp"
#include "Campfires.hpp"
#include "Localization.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Renderer
{
    void GradeTerrain(NativeSurface* surface)
    {
        if (!surface) return;
        DDSURFACEDESC2 desc{};desc.dwSize=sizeof(desc);
        if (FAILED(surface->Lock(nullptr,&desc,DDLOCK_WAIT,nullptr))) return;
        const auto& format=desc.ddpfPixelFormat;
        for(unsigned y=0;y<desc.dwHeight;++y)for(unsigned x=0;x<desc.dwWidth;++x)
        {
            const auto value=PixelFormat::read(desc.lpSurface,desc.lPitch,x,y,format.dwRGBBitCount);
            const auto color=ArtPalette::grade(PixelFormat::unpack(value,format.dwRBitMask),PixelFormat::unpack(value,format.dwGBitMask),PixelFormat::unpack(value,format.dwBBitMask));
            const auto graded=PixelFormat::pack(color[0],color[1],color[2],format.dwRBitMask,format.dwGBitMask,format.dwBBitMask);
            PixelFormat::write(desc.lpSurface,desc.lPitch,x,y,format.dwRGBBitCount,graded|(value&~(format.dwRBitMask|format.dwGBitMask|format.dwBBitMask)));
        }
        surface->Unlock(nullptr);
    }
    double pi = 3.1415926535; // pi, was sonst

    void Fade(short r,short g,short b){NativeDisplay::SetFade(r/100.f,g/100.f,b/100.f);}

    void LimitScroll()
    {
        if (Camera.x < ScapeGrenze.left)
            Camera.x = static_cast<short>(ScapeGrenze.left);
        if (Camera.x + rcSpielflaeche.right > ScapeGrenze.right)
            Camera.x = static_cast<short>(ScapeGrenze.right) - static_cast<short>(rcSpielflaeche.right);
        if (Camera.y < ScapeGrenze.top)
            Camera.y = static_cast<short>(ScapeGrenze.top);
        if (Camera.y + rcSpielflaeche.bottom > ScapeGrenze.bottom)
            Camera.y = static_cast<short>(ScapeGrenze.bottom) - static_cast<short>(rcSpielflaeche.bottom);
    }

    ZWEID GetKachel(short PosX, short PosY)
    {
        ZWEID Erg;

        for (short y = 0; y < MAXYKACH; y++)
            for (short x = 0; x < MAXXKACH; x++)
            {
                // Die in Betracht kommenden Kacheln rausfinden
                if ((PosX > Scape[x][y].xScreen) && (PosX < Scape[x][y].xScreen + KXPIXEL) &&
                    (PosY > Scape[x][y].yScreen) && (PosY < Scape[x][y].yScreen + KYPIXEL))
                {
                    if ((Math::InDreieck(PosX, PosY,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][0].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][0].y,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][1].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][1].y,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][3].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][3].y)) ||
                        (Math::InDreieck(PosX, PosY,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][2].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][2].y,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][1].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][1].y,
                                         Scape[x][y].xScreen + EckKoor[Scape[x][y].Typ][3].x,
                                         Scape[x][y].yScreen + EckKoor[Scape[x][y].Typ][3].y)))
                    {
                        Erg.x = x;
                        Erg.y = y;
                        return Erg;
                    }
                }
            }
        Erg.x = -1;
        Erg.y = -1;
        return Erg;
    }

    DWORD RGB2DWORD(BYTE r, BYTE g, BYTE b)
    {
        return PixelFormat::pack(r,g,b,ddpf.dwRBitMask,ddpf.dwGBitMask,ddpf.dwBBitMask);
    }

    void DWORD2RGB(DWORD color)
    {
        rgbStruct.r = PixelFormat::unpack(color,ddpf.dwRBitMask);
        rgbStruct.g = PixelFormat::unpack(color,ddpf.dwGBitMask);
        rgbStruct.b = PixelFormat::unpack(color,ddpf.dwBBitMask);
    }

    void Blitten(NativeSurface* lpDDSVon, NativeSurface* lpDDSNach, bool Transp)
    {
        HRESULT hr;
        short z = 0;

        while (true)
        {
            z++;
            hr = lpDDSNach->GetBltStatus(/*DDGBS_ISBLTDONE |*/ DDGBS_CANBLT);

            if (hr == DD_OK) break;
            Sleep(1);

            if (z == 1000)
            {
                MessageBeep(MB_OK);
                break;
            }
        }

        while (true)
        {
            z++;
            if (Transp) hr = lpDDSNach->Blt(&rcRectdes, lpDDSVon, &rcRectsrc, DDBLT_KEYSRC | DDBLT_WAIT, nullptr);
            else hr = lpDDSNach->Blt(&rcRectdes, lpDDSVon, &rcRectsrc, DDBLT_WAIT, nullptr);
            if (hr != DDERR_WASSTILLDRAWING)
                break;
            if (z == 1000)
            {
                MessageBeep(MB_OK);
                break;
            }
        }
    }

    void PutPixel(short x, short y, DWORD color, LPDDSURFACEDESC2 surface)
    {
        if (x < 0 || y < 0 || x >= surface->dwWidth || y >= surface->dwHeight) return;
        PixelFormat::write(surface->lpSurface,surface->lPitch,x,y,surface->ddpfPixelFormat.dwRGBBitCount,color);
    }

    void GetPixel(short x, short y, LPDDSURFACEDESC2 surface)
    {
        if (x < 0 || y < 0 || x >= surface->dwWidth || y >= surface->dwHeight)
        {
            rgbStruct = {}; return;
        }
        DWORD2RGB(PixelFormat::read(surface->lpSurface,surface->lPitch,x,y,surface->ddpfPixelFormat.dwRGBBitCount));
    }

    void ZeichneBilder(short x, short y, short i, RECT Ziel, bool Reverse, short Frucht)
    {
        short Phase;

        if (Frucht == -1) Phase = Bmp[i].Phase;
        else Phase = Frucht;
        rcRectsrc = Bmp[i].rcSrc;
        if (!Reverse)
        {
            rcRectsrc.top += Phase * (Bmp[i].Hoehe);
        }
        else
        {
            rcRectsrc.top = Bmp[i].rcSrc.top + (Bmp[i].Anzahl - 1) * Bmp[i].Hoehe - Phase * Bmp[i].Hoehe;
        }
        rcRectsrc.bottom = rcRectsrc.top + (Bmp[i].Hoehe);
        rcRectdes.left = x;
        rcRectdes.top = y;
        rcRectdes.right = x + (Bmp[i].Breite);
        rcRectdes.bottom = y + (Bmp[i].Hoehe);
        Math::CalcRect(Ziel);
        Blitten(Bmp[i].Surface, lpDDSBack, true);
    }

    void ZeichneObjekte()
    {
        for (short y = 0; y < MAXYKACH; y++)
            for (short x = 0; x < MAXXKACH; x++)
            {
                bool Guyzeichnen = false;
                if ((Guy.Pos.x == x) && (Guy.Pos.y == y)) Guyzeichnen = true;
                // Die nichtsichbaren Kacheln (oder nicht betroffenen) ausfiltern

                if (!((Scape[x][y].xScreen > Camera.x + rcSpielflaeche.left - KXPIXEL) &&
                    (Scape[x][y].xScreen < Camera.x + rcSpielflaeche.right + KXPIXEL) &&
                    (Scape[x][y].yScreen > Camera.y + rcSpielflaeche.top - KYPIXEL) &&
                    (Scape[x][y].yScreen < Camera.y + rcSpielflaeche.bottom + KYPIXEL) &&
                    (Scape[x][y].Entdeckt) &&
                    ((Scape[x][y].Markiert) || (Scape[x][y].Objekt != -1) || (Guyzeichnen))))
                    continue;

                if (Scape[x][y].Markiert) // Die Rahmen um die markierten Kacheln malen
                {
                    rcRectsrc.left = KXPIXEL * Scape[x][y].Typ;
                    rcRectsrc.right = KXPIXEL * Scape[x][y].Typ + KXPIXEL;
                    rcRectsrc.top = 2 * KYPIXEL;
                    rcRectsrc.bottom = 3 * KYPIXEL;
                    rcRectdes.left = Scape[x][y].xScreen - Camera.x;
                    rcRectdes.right = rcRectdes.left + KXPIXEL;
                    rcRectdes.top = Scape[x][y].yScreen - Camera.y;
                    rcRectdes.bottom = rcRectdes.top + KYPIXEL;
                    Math::CalcRect(rcSpielflaeche);
                    Blitten(lpDDSMisc, lpDDSBack, true);
                }
                // Landschaftsanimationen malen (und Feld)
                if ((Scape[x][y].Objekt != -1) && (LAnimation) &&
                    ((Scape[x][y].Objekt <= SCHLEUSE6))
                    || (Scape[x][y].Objekt == FELD) // Der Guy ist immer vor diesen Objekten
                    || (Scape[x][y].Objekt == ROHR)
                    || (Scape[x][y].Objekt == SOS))
                {
                    ZeichneBilder(Scape[x][y].xScreen + Scape[x][y].ObPos.x - Camera.x,
                                  Scape[x][y].yScreen + Scape[x][y].ObPos.y - Camera.y,
                                  Scape[x][y].Objekt, rcSpielflaeche, Scape[x][y].Reverse,
                                  static_cast<short>(Scape[x][y].Phase));
                }
                else
                {
                    if (((Scape[x][y].Objekt >= BAUM1) && (Scape[x][y].Objekt <= BAUM4DOWN)) ||
                        (Scape[x][y].Objekt == BAUMGROSS) || (Scape[x][y].Objekt == FEUER) ||
                        (Scape[x][y].Objekt == WRACK) || (Scape[x][y].Objekt == WRACK2) ||
                        (Scape[x][y].Objekt >= ZELT)) // Bäume und Früchte (und alle anderen Objekte) malen
                    {
                        if (Guyzeichnen)
                        {
                            if ((Guy.PosScreen.y) < (Scape[x][y].yScreen + Scape[x][y].ObPos.y
                                + Bmp[Scape[x][y].Objekt].Hoehe))
                            {
                                ZeichneGuy();
                                Guyzeichnen = false;
                            }
                        }

                        if(Scape[x][y].Objekt==FEUER)
                        {
                            const auto bounds=Campfires::Sprite(Scape[x][y]);const auto& b=Bmp[FEUER];
                            RECT source=b.rcSrc;source.top+=int(Scape[x][y].Phase)*b.Hoehe;source.bottom=source.top+b.Hoehe;
                            RECT destination{bounds.x-Camera.x,bounds.y-Camera.y,bounds.x-Camera.x+bounds.w,bounds.y-Camera.y+bounds.h};
                            lpDDSBack->Blt(&destination,b.Surface,&source,DDBLT_KEYSRC,nullptr);
                        }
                        else ZeichneBilder(Scape[x][y].xScreen + Scape[x][y].ObPos.x - Camera.x,
                                      Scape[x][y].yScreen + Scape[x][y].ObPos.y - Camera.y,
                                      Scape[x][y].Objekt, rcSpielflaeche, false,
                                      static_cast<short>(Scape[x][y].Phase));
                    }
                }
                if (Guyzeichnen) ZeichneGuy();
            }
    }

    void ZeichneGuy()
    {
        if (BootsFahrt)
        {
            if (Guy.Zustand == GUYSCHIFF)
            {
                ZeichneBilder(Guy.PosScreen.x - 30 - Camera.x,
                              Guy.PosScreen.y - 28 - Camera.y,
                              Guy.Zustand, rcSpielflaeche, false, -1);
            }
            else
            {
                ZeichneBilder(Guy.PosScreen.x - (Bmp[Guy.Zustand].Breite) / 2 - Camera.x,
                              Guy.PosScreen.y - (Bmp[Guy.Zustand].Hoehe) / 2 - Camera.y,
                              Guy.Zustand, rcSpielflaeche, false, -1);
            }
        }
        else
            ZeichneBilder(Guy.PosScreen.x - (Bmp[Guy.Zustand].Breite) / 2 - Camera.x,
                          Guy.PosScreen.y - (Bmp[Guy.Zustand].Hoehe) - Camera.y,
                          Guy.Zustand, rcSpielflaeche, false, -1);
    }

    void ZeichnePapier(){ModernUI::DrawDialog();}
    void ZeichnePanel(){ModernUI::DrawPanel();}

    void DrawString(char* str,short x,short y,short art)
    {
        ModernUI::Text(x,y,Localization::ToWide(str),art==1?16:13);
    }
    short DrawText(int id,short area,short art)
    {
        Textloeschen(area);TextBereich[area].Aktiv=true;
        auto value=Localization::Wide(id);
        if(id==TOD||id==TAGENDE5)value+=L"\n\n"+Survival::DeathReason();
        const auto replace=[&](const std::wstring& from,const std::wstring& to){std::size_t p=0;while((p=value.find(from,p))!=std::wstring::npos){value.replace(p,from.size(),to);p+=to.size();}};
        replace(L"/a",std::to_wstring(Tag));replace(L"/b",std::to_wstring(int(Guy.Resource[GESUNDHEIT])));
        replace(L"/c",std::to_wstring(int(std::max(0.f,Chance))));replace(L"/z",L"\n\n");
        if(value.find(L"/d")!=std::wstring::npos){Frage=0;replace(L"/d",L"");}
        HDC dc=nullptr;lpDDSSchrift->GetDC(&dc);
        HFONT f=CreateFontW(art==1?-18:-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
        auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(222,230,214));
        RECT rect=TextBereich[area].rcText;
        if(area==TXTPAPIER){rect={326,230,954,520};TextBereich[area].rcText=rect;}
        HBRUSH background=CreateSolidBrush(RGB(17,30,33));FillRect(dc,&rect,background);DeleteObject(background);
        DrawTextW(dc,value.c_str(),int(value.size()),&rect,DT_WORDBREAK|DT_NOPREFIX|DT_EDITCONTROL);
        SelectObject(dc,old);DeleteObject(f);lpDDSSchrift->ReleaseDC(dc);
        return 290;
    }

    void Textloeschen(short Bereich)
    {
        TextBereich[Bereich].Aktiv = false;
        ddbltfx.dwFillColor = RGB2DWORD(255, 0, 255);
        lpDDSSchrift->Blt(&TextBereich[Bereich].rcText, nullptr, nullptr, DDBLT_COLORFILL, &ddbltfx);
    }

    void DrawSchatzkarte()
    {
        Textloeschen(TXTPAPIER);
        TextBereich[TXTPAPIER].Aktiv = true;
        PapierText = SKARTEY;

        rcRectsrc.left = 0;
        rcRectsrc.right = SKARTEX;
        rcRectsrc.top = 0;
        rcRectsrc.bottom = SKARTEY;
        rcRectdes.left = TextBereich[TXTPAPIER].rcText.left;
        rcRectdes.top = TextBereich[TXTPAPIER].rcText.top;
        rcRectdes.right = rcRectdes.left + SKARTEX;
        rcRectdes.bottom = rcRectdes.top + SKARTEY;

        Blitten(lpDDSSchatzkarte, lpDDSSchrift, false);
    }

    void Zeige()
    {
        char Stringsave1[128], Stringsave2[128]; // Für die Zeitausgabe

        rcRectsrc.left = Camera.x + rcSpielflaeche.left;
        rcRectsrc.top = Camera.y + rcSpielflaeche.top;
        rcRectsrc.right = Camera.x + rcSpielflaeche.right;
        rcRectsrc.bottom = Camera.y + rcSpielflaeche.bottom;
        rcRectdes.left = rcSpielflaeche.left;
        rcRectdes.top = rcSpielflaeche.top;
        rcRectdes.right = rcSpielflaeche.right;
        rcRectdes.bottom = rcSpielflaeche.bottom;

        Blitten(lpDDSScape, lpDDSBack, false); // Landschaft zeichnen

        Expedition::DrawCaches(false);
        ZeichneObjekte();
        Expedition::DrawCaches(true);
        Expedition::DrawAtmosphere();
        SceneView::Apply();
        ZeichnePanel();

        if (PapierText != -1) ZeichnePapier();

        // Die Textsurface blitten
        for (short i = 0; i < TEXTANZ; i++)
        {
            if (!TextBereich[i].Aktiv) continue; // Die nicht aktiven Felder auslassen
            rcRectsrc = TextBereich[i].rcText;
            rcRectdes = TextBereich[i].rcText;
            Blitten(lpDDSSchrift, lpDDSBack, true);
        }
        // Alles schwarz übermalen und nur das Papier mit Text anzeigen
        if (Nacht)
        {
            rcRectdes.left = 0;
            rcRectdes.top = 0;
            rcRectdes.right = MAXX;
            rcRectdes.bottom = MAXY;
            ddbltfx.dwFillColor = RGB2DWORD(0, 0, 0);
            lpDDSBack->Blt(&rcRectdes, nullptr, nullptr, DDBLT_COLORFILL, &ddbltfx);

            if (PapierText != -1)
            {
                ZeichnePapier();
                rcRectsrc = TextBereich[TXTPAPIER].rcText;
                rcRectdes = TextBereich[TXTPAPIER].rcText;
                Blitten(lpDDSSchrift, lpDDSBack, true);
            }
            Fade(100, 100, 100);
        }

        {const auto mouse=NativeDisplay::MousePoint();Expedition::DrawHud(mouse.x,mouse.y);}
        ModernUI::DrawNightJournal();
        ModernUI::DrawOverlay();
        // Flippen
        while (true)
        {
            HRESULT ddrval = lpDDSPrimary->Flip(nullptr, 0);
            if (ddrval == DD_OK)
            {
                break;
            }
            if (ddrval == DDERR_SURFACELOST)
            {
                ddrval = lpDDSPrimary->Restore();
                if (ddrval != DD_OK)
                {
                    break;
                }
            }
            if (ddrval != DDERR_WASSTILLDRAWING)
            {
                break;
            }
        }


        if (Nacht) Fade(100, 100, 100); // Das muß hier stehen, damit man die Textnachricht in der Nacht lesen kann
    }

    void ZeigeIntro()
    {
        rcRectdes.left = 0;
        rcRectdes.top = 0;
        rcRectdes.right = MAXX;
        rcRectdes.bottom = MAXY;
        ddbltfx.dwFillColor = RGB2DWORD(0, 0, 0);
        short z = 0;
        while (true)
        {
            z++;
            HRESULT ddrval = lpDDSBack->Blt(&rcRectdes, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
            if (ddrval != DDERR_WASSTILLDRAWING) break;
            if (z == 1000)
            {
                MessageBeep(MB_OK);
                break;
            }
        }

        rcRectsrc.left = Camera.x + rcSpielflaeche.left;
        rcRectsrc.top = Camera.y + rcSpielflaeche.top;
        rcRectsrc.right = Camera.x + rcSpielflaeche.right;
        rcRectsrc.bottom = Camera.y + rcSpielflaeche.bottom;
        rcRectdes.left = rcSpielflaeche.left;
        rcRectdes.top = rcSpielflaeche.top;
        rcRectdes.right = rcSpielflaeche.right;
        rcRectdes.bottom = rcSpielflaeche.bottom;

        Blitten(lpDDSScape, lpDDSBack, false); // Landschaft zeichnen

        ZeichneObjekte();

        if (PapierText != -1) ZeichnePapier();

        // Die Textsurface blitten
        for (short i = 0; i < TEXTANZ; i++)
        {
            if (!TextBereich[i].Aktiv) continue; // Die nicht aktiven Felder auslassen
            rcRectsrc = TextBereich[i].rcText;
            rcRectdes = TextBereich[i].rcText;
            Blitten(lpDDSSchrift, lpDDSBack, true);
        }

        // Flippen
        while (true)
        {
            HRESULT ddrval = lpDDSPrimary->Flip(nullptr, 0);
            if (ddrval == DD_OK)
            {
                break;
            }
            if (ddrval == DDERR_SURFACELOST)
            {
                ddrval = lpDDSPrimary->Restore();
                if (ddrval != DD_OK)
                {
                    break;
                }
            }
            if (ddrval != DDERR_WASSTILLDRAWING)
            {
                break;
            }
        }
    }

    void ZeigeAbspann(){ModernUI::DrawCredits();}
    void ZeigeLogo(){ModernUI::DrawStart();}

    void AbspannBlt(short Bild, short Prozent)
    {
        Bmp[Bild].Surface->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr);
        lpDDSBack->Lock(nullptr, &ddsd2, DDLOCK_WAIT, nullptr);

        for (short x = 0; x < Bmp[Bild].Breite; x++)
            for (short y = 0; y < Bmp[Bild].Hoehe; y++)
            {
                if ((x + Bmp[Bild].rcDes.left >= MAXX) || (x + Bmp[Bild].rcDes.left <= 0) ||
                    (y + Bmp[Bild].rcDes.top >= MAXY) || (y + Bmp[Bild].rcDes.top <= 0))
                    continue;
                Renderer::GetPixel(static_cast<short>(x + Bmp[Bild].rcDes.left),
                                   static_cast<short>(y + Bmp[Bild].rcDes.top), &ddsd2);
                RGBSTRUCT rgbalt = rgbStruct;
                Renderer::GetPixel(static_cast<short>(x + Bmp[Bild].rcSrc.left),
                                   static_cast<short>(y + Bmp[Bild].rcSrc.top), &ddsd);
                if ((rgbStruct.r == 0) && (rgbStruct.g == 0) && (rgbStruct.b == 0)) continue;
                PutPixel(static_cast<short>(x + Bmp[Bild].rcDes.left),
                         static_cast<short>(y + Bmp[Bild].rcDes.top),
                         RGB2DWORD(rgbalt.r + (rgbStruct.r - rgbalt.r) * Prozent / 100,
                                   rgbalt.g + (rgbStruct.g - rgbalt.g) * Prozent / 100,
                                   rgbalt.b + (rgbStruct.b - rgbalt.b) * Prozent / 100),
                         &ddsd2);
            }

        Bmp[Bild].Surface->Unlock(nullptr);
        lpDDSBack->Unlock(nullptr);
    }
} // namespace Renderer
