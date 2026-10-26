#pragma once

#include "headers.hpp"
#include "extern.hpp"

namespace Direct
{
    void QueueKeyPress(int key);
    // finished with all objects we use; release them
    void finiObjects();

    void InitDDraw(HWND hWnd); // DirectDraw initialisieren
    void Reset();
    void ClearInput();
    void CheckMouse(); // Maus abfragen
    short CheckKey(); // Tastatur abfragen
}
