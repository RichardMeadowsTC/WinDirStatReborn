#include "stdafx.h"
#include "Window.h"
#include "StatusControl.h"

BOOL StatusControl::Create(HWND hwndParent, HINSTANCE hInstance, ULONG64 id)
{
    m_hWnd = CreateWindowEx(
        0,                      // no extended styles
        STATUSCLASSNAME,        // name of status bar class
        (LPCTSTR)NULL,          // no text when first created
        SBARS_SIZEGRIP |        // includes a sizing grip
        WS_CHILD,               // creates a child window
        0, 0, 0, 0,             // ignores size and position
        hwndParent,             // handle to parent window
        (HMENU)id,              // child window identifier
        hInstance,              // handle to application instance
        NULL);                  // no window creation data

    if (NULL == m_hWnd)
    {
        return FALSE;
    }

    int aParts[2];
    aParts[0] = 200;
    aParts[1] = 260;
    SendMessage(m_hWnd, SB_SETPARTS, 1, (LPARAM)aParts);
    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);

    return TRUE;
}

void StatusControl::SetNumActiveWorkerThreads(LONG64 cThreads)
{
    WCHAR szMsg[256];
    StringCbPrintf(szMsg, sizeof(szMsg), TEXT("%u worker threads running"), cThreads);
    SendMessage(m_hWnd, SB_SETTEXT, 0, (LPARAM)szMsg);
}