// AsDesktopApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AsDesktopApp.h"
#include "Window.h"
#include "HeaderControl.h"
#include "StatusControl.h"
#include "TreeView.h"
#include "SelectDirectories.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWndMain;

HeaderControl headerControl;
StatusControl statusControl;
TreeView treeView(statusControl);  // Has the full directory info.

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    DWORD dwErr;

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ASDESKTOPAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Initialize common controls.
    INITCOMMONCONTROLSEX control = { 0 };
    control.dwSize = sizeof(INITCOMMONCONTROLSEX);
    control.dwICC = ICC_BAR_CLASSES |   // Tool bar Status bar
        ICC_TAB_CLASSES |               // Tool Tip
        ICC_TREEVIEW_CLASSES;           //Tree View
    if (!InitCommonControlsEx(&control))
    {
        dwErr = GetLastError();
        return FALSE;
    }

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ASDESKTOPAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASDESKTOPAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ASDESKTOPAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void CalcChildRectangles(HWND hWnd, RECT &rcHeader, RECT &rcTreeView, RECT &rcStatus)
{
    RECT rcClient;  // dimensions of client area 
    GetClientRect(hWnd, &rcClient);

    rcHeader = rcClient;
    rcTreeView = rcClient;
    rcStatus = rcClient;

    if (!headerControl.CalcPosition(rcClient, rcHeader))
    {
        int cyHeader = GetSystemMetrics(SM_CYMENU);
        rcHeader.bottom = rcHeader.top + cyHeader;
    }

    // The rectangle bottom stores size not actual bottom.  Keeping our rectangles compatible with GetClientRect.
    rcTreeView.top = rcHeader.top + rcHeader.bottom;
    // Have to subtract from the bottom of rcTreeView amount used up in other controls.

    RECT rectStatus;  // GetWindowRect unlike GetClientRect, returns actual coordinate of bottom.  Not the height.
    statusControl.GetWindowRect(rectStatus);
    int statusHeight = rectStatus.bottom - rectStatus.top;
    rcTreeView.bottom -= (rcHeader.bottom + statusHeight);
    // Set rcStatus using client coordinates.  Bottom is height.
    rcStatus.bottom = statusHeight;
    rcStatus.top = rcClient.bottom - statusHeight;
}

void StartNewQuery(HWND hWnd)
{
    treeView.Reset();

    SelectDirectories dlgSelectDirs;
    INT_PTR sel = dlgSelectDirs.DoModal(hInst, hWnd);
    if (IDOK == sel)
    {
        std::set<std::wstring> *pDirSet = dlgSelectDirs.GetFolderListPtr();
        std::set<std::wstring>::const_iterator itr = pDirSet->begin();
        while (itr != pDirSet->end())
        {
            treeView.AddStartDirectory(itr->c_str());
            itr++;
        }
    }
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWndMain = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWndMain)
   {
      return FALSE;
   }

   int folderColumnWidth = 185;
   int percentColumnWidth = 85;
   int sizeColumnWidth = 85;
   headerControl.Create(hWndMain, hInst, ID_HEADER, folderColumnWidth, percentColumnWidth, sizeColumnWidth);

   // Create the status bar.
   statusControl.Create(hWndMain, hInst, ID_STATUS);

   RECT rcHeader, rcTreeView, rcStatus;
   CalcChildRectangles(hWndMain, rcHeader, rcTreeView, rcStatus);

   if (!treeView.Create(hWndMain, rcTreeView, hInst, ID_TREEVIEW, folderColumnWidth, percentColumnWidth, sizeColumnWidth))
   {
       return FALSE;
   }

   if (!treeView.InitImageLists())
   {
       return FALSE;
   }

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   StartNewQuery(hWndMain);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_REFRESH:
                treeView.SortItems(TVI_ROOT);
                break;
            case ID_FILE_NEWSEARCH:
                StartNewQuery(hWndMain);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        {
            RECT rcHeader, rcTreeView, rcStatus;
            CalcChildRectangles(hWnd, rcHeader, rcTreeView, rcStatus);
            headerControl.Size(rcHeader);
            treeView.Size(rcTreeView);
            statusControl.Size(rcStatus);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR>(lParam);
            if (ID_TREEVIEW == pnmhdr->idFrom)
            {
                return treeView.NotifyHandler(pnmhdr);
            }

            if (ID_HEADER == pnmhdr->idFrom)
            {
                if (HDN_ITEMCHANGED == pnmhdr->code)
                {
                    LPNMHEADER phdr = (LPNMHEADER)pnmhdr;
                    LPHDITEM pItem = phdr->pitem;
                    if (HDI_WIDTH & pItem->mask)
                    {
                        switch (phdr->iItem)
                        {
                        case 0:
                            treeView.SetFolderColumnWidth(pItem->cxy);
                            break;
                        case 1:
                            treeView.SetPercentColumnWidth(pItem->cxy);
                            break;
                        case 2:
                            treeView.SetSizeColumnWidth(pItem->cxy);
                            break;
                        }

                        treeView.RepaintWindow();
                    }
                }

            }
        }
        break;
    case WM_DESTROY:
        // Turn off any worker threads so we can properly cleanup all memory resources.
        treeView.ShutdownWorkerThreads();

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
