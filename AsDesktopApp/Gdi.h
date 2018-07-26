#pragma once

//using namespace Gdiplus;

// Extra GDI operations to help with things that seam to be missing in Windows API.
// Actually, some of the things we need are in GDI Plus.
// Am avoiding COM dependency.

// Draws a dotted line that has point drawn every other pixel.
// Line is drawn accross the screen left to right.
BOOL DrawDottedLineAcross(HDC hdc, const int xStart, const int yStart, const int length)
{
    //Gdiplus::Graphics graphics(hdc);
    //Gdiplus::Pen pen(Gdiplus::Color(0, 0, 0), 1.0);
    //pen.SetDashStyle(Gdiplus::DashStyle::DashStyleDot);  // This draws a proper line. Not sure what the cost of it is.
    //graphics.DrawLine(&pen, xStart, yStart, xStart + length, yStart);
    
    int xCurrent = xStart;
    do
    {
        SetPixel(hdc, xCurrent, yStart, RGB(0, 0, 0));
        xCurrent += 2;
    } while ((xCurrent - xStart) < length);

    return TRUE;
}

// Draws a dotted line that has point drawn every other pixel.
// Line is drawn straight down the screen.
BOOL DrawDottedLineDown(HDC hdc, int xStart, int yStart, int length)
{
    int yCurrent = yStart;
    do
    {
        SetPixel(hdc, xStart, yCurrent, RGB(0, 0, 0));
        yCurrent += 2;
    } while ((yCurrent - yStart) < length);

    return TRUE;
}