#include "stdafx.h"
#include "AsDesktopApp.h"
#include "Window.h"
#include "HeaderControl.h"


BOOL HeaderControl::CalcPosition(const RECT &rectParent, RECT &rect)
{
    BOOL bCalculated = FALSE;
    if (m_hWnd)
    {
        HDLAYOUT hdmLayout;
        RECT rc = rectParent;
        WINDOWPOS winposHeader = { 0 };
        hdmLayout.prc = &rc;
        hdmLayout.pwpos = &winposHeader;

        bCalculated = static_cast<BOOL>(SendMessage(m_hWnd, HDM_LAYOUT, 0, (LPARAM)&hdmLayout));
        if (bCalculated)
        {
            rect.left = winposHeader.x;
            rect.top = winposHeader.y;
            rect.bottom = winposHeader.cy;
            rect.right = winposHeader.cx;
        }
    }

    return bCalculated;
}

BOOL HeaderControl::Create(HWND hwndParent, HINSTANCE hInstance, ULONG64 id, int fileWidth, int percentWidth, int sizeWidth)
{

    DWORD dwErr = NOERROR;

    m_hWnd = CreateWindowEx(0,
        WC_HEADER,
        TEXT("HEADER CONTROL"),
        HDS_BUTTONS | HDS_HORZ |
        WS_CHILD | WS_BORDER,
        0,0,0,0,
        hwndParent,
        (HMENU)id,
        hInstance,
        NULL);

    if (NULL == m_hWnd)
    {
        dwErr = GetLastError();
    }

    // Add the collumns.
    HDITEM hdi = { 0 };
    hdi.mask = HDI_TEXT | HDI_WIDTH;
    hdi.fmt = HDF_LEFT;
    hdi.iOrder = 1;
    hdi.pszText = const_cast<LPWSTR>(L"Folder");
    hdi.cxy = fileWidth;
    int index = Header_InsertItem(m_hWnd, 0, &hdi);

    hdi.fmt = HDF_RIGHT;
    hdi.pszText = const_cast<LPWSTR>(L"Percent");
    hdi.cxy = percentWidth;
    index = Header_InsertItem(m_hWnd, index + 1, &hdi);

    hdi.pszText = const_cast<LPWSTR>(L"Size");
    hdi.cxy = sizeWidth;
    index = Header_InsertItem(m_hWnd, index + 1, &hdi);

    RECT rcParent;  // dimensions of client area 
    GetClientRect(hwndParent, &rcParent);

    WINDOWPOS wp;
    HDLAYOUT hdl;
    hdl.prc = &rcParent;
    hdl.pwpos = &wp;
    if (!SendMessage(m_hWnd, HDM_LAYOUT, 0, (LPARAM)&hdl))
    {
        dwErr = 1;
    }
    else
    {
        SetWindowPos(m_hWnd, wp.hwndInsertAfter, wp.x, wp.y,
            wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
    }

    return NOERROR == dwErr;
}

void HeaderControl::NotifyHandler(LPNMHDR pmnhdr)
{
    if (pmnhdr->hwndFrom != m_hWnd)
    {
        return;
    }

}