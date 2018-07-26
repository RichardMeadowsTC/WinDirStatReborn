#pragma once
class Window
{
protected:
    HWND m_hWnd;
public:
    Window() : m_hWnd(NULL)
    {

    }

    void Attach(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    void Detach()
    {
        m_hWnd = NULL;
    }

    // Size to a rectangle that is compatible with RECT from GetClientRect.  The left and top members specify location. The right and bottom members represent the width and height.
    void Size(const RECT &rect)
    {
        SetWindowPos(m_hWnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOSENDCHANGING | SWP_NOZORDER);
    }

    void GetWindowRect(RECT &rc)
    {
        ::GetWindowRect(m_hWnd, &rc);
    }

    void RepaintWindow()
    {
        ::InvalidateRect(m_hWnd, NULL, TRUE);
        ::UpdateWindow(m_hWnd);
    }
};
