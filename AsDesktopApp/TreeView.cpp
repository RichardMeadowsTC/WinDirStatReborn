#include "stdafx.h"
#include "AsDesktopApp.h"
#include "Window.h"
#include "TreeView.h"
#include "StatusControl.h"

#include "Logging.h"
#include "Directory.h"
#include "Gdi.h"
#include "ItemSizeText.h"


static volatile LONG64 s_cWorkerThreads = 0;

TreeView::TreeView(StatusControl &statusControl) : 
    m_hInstance(NULL), m_statusControl(statusControl), 
    m_Indent(0), 
    m_hbmpClosed(NULL), m_hbmpOpen(NULL), 
    m_hdcOpen(NULL), m_hdcClosed(NULL),
    m_origBmpOpen(NULL), m_origBmpClosed(NULL),
    m_folderColumnWidth(0), m_percentColumnWidth(0), m_sizeColumnWidth(0)
{
    m_hAbortEvent = NULL;
}

TreeView::~TreeView()
{
    if (m_hAbortEvent)
    {
        CloseHandle(m_hAbortEvent);
        m_hAbortEvent = NULL;
    }
}

LRESULT CALLBACK TreeViewWndProc(HWND hWnd, UINT msg,
    WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    switch (msg)
    {
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (IDM_REFRESH == wmId)
            {
                TreeView *pTV = (TreeView *)dwRefData;
                pTV->SortItems(TVI_ROOT);
            }
        }
        break;
        case WMU_TREE_SORT:
        {
            TreeView *pTV = (TreeView *)dwRefData;
            LPWMU_TREE_SORT_DATA pData = reinterpret_cast<LPWMU_TREE_SORT_DATA>(lParam);
            pTV->SortItems(pData->hTreeItemParent);
            pTV->RepaintSizeStats(pData->hTreeItem);
        }
        break;
        case WMU_UPDATE_TREE_ITEM:
        {
            TreeView *pTV = (TreeView *)dwRefData;
            HTREEITEM hTreeItem = reinterpret_cast<HTREEITEM>(lParam);
            pTV->RepaintSizeStats(hTreeItem);
        }
        break;
        case WMU_WORKER_FINISHED:
        {
            TreeView *pTV = (TreeView *)dwRefData;
            ThreadInfo threadInfo = *(reinterpret_cast<ThreadInfo *>(lParam));
            pTV->DecrementWorkerThreadCount(threadInfo);
        }
        break;
        case WM_DESTROY:
        {
            TreeView *pTV = (TreeView *)dwRefData;
            pTV->RemoveAll();
            pTV->CleanupAllResources();
        }
        break;
    }

    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

BOOL TreeView::Create(HWND hwndParent, RECT &rect, HINSTANCE hInstance, ULONG64 id, int fileWidth, int percentWidth, int sizeWidth)
{
    m_hInstance = hInstance;
    m_folderColumnWidth = fileWidth;
    m_percentColumnWidth = percentWidth;
    m_sizeColumnWidth = sizeWidth;

    DWORD dwErr = NOERROR;
    
    m_hWnd = CreateWindowEx(0,
        WC_TREEVIEW,
        TEXT("Tree View"),
        TVS_DISABLEDRAGDROP | TVS_NOTOOLTIPS |
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        rect.left,
        rect.top,
        rect.right,
        rect.bottom,
        hwndParent,
        (HMENU)id,
        hInstance,
        NULL);

    if (NULL == m_hWnd)
    {
        dwErr = GetLastError();
    }

    if (!SetWindowSubclass(m_hWnd, TreeViewWndProc, id, (DWORD_PTR) this))
    {
        dwErr = GetLastError();
        return FALSE;
    }

    m_Indent = TreeView_GetIndent(m_hWnd);

    m_hAbortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    return NOERROR == dwErr;
}

BOOL TreeView::InitImageLists()
{
    HIMAGELIST himl;  // handle to image list 

    const int NUM_BITMAPS = 2;
    const int CX_BITMAP = 16;
    const int CY_BITMAP = 16;

    // Create the image list. 
    himl = ImageList_Create(CX_BITMAP,
                            CY_BITMAP,
                            FALSE,
                            NUM_BITMAPS, 0);
    if (NULL == himl)
    {
        return FALSE;
    }

    // Add the open file, closed file, and document bitmaps. 
    m_hbmpClosed = LoadBitmap(m_hInstance, MAKEINTRESOURCE(IDB_CLOSED));
    if (!m_hbmpClosed)
    {
        return FALSE;
    }

    m_nClosed = ImageList_Add(himl, m_hbmpClosed, (HBITMAP)NULL);
    
    m_hbmpOpen = LoadBitmap(m_hInstance, MAKEINTRESOURCE(IDB_OPEN));
    if (!m_hbmpOpen)
    {
        return FALSE;
    }

    m_nOpen = ImageList_Add(himl, m_hbmpOpen, (HBITMAP)NULL);

    // Fail if not all of the images were added. 
    if (ImageList_GetImageCount(himl) < NUM_BITMAPS)
    {
        return FALSE;
    }

    // Associate the image list with the tree-view control. 
    TreeView_SetImageList(m_hWnd, himl, TVSIL_NORMAL);

    // Add the images to in memory DCs so we can use them for owner draw rendering.
    m_hdcOpen = CreateCompatibleDC(NULL);
    m_origBmpOpen = SelectObject(m_hdcOpen, m_hbmpOpen);
    m_hdcClosed = CreateCompatibleDC(NULL);
    m_origBmpClosed = SelectObject(m_hdcClosed, m_hbmpClosed);

    return TRUE;
}

void TreeView::RemoveAll()
{
    TreeView_DeleteAllItems(m_hWnd);
}

void TreeView::CleanupAllResources()
{
    if (m_hdcOpen)
    {
        SelectObject(m_hdcOpen, m_origBmpOpen);
        m_origBmpOpen = NULL;
        DeleteDC(m_hdcOpen);
        m_hdcOpen = NULL;
    }

    if (m_hdcClosed)
    {
        SelectObject(m_hdcClosed, m_origBmpClosed);
        m_origBmpClosed = NULL;
        DeleteDC(m_hdcClosed);
        m_hdcClosed = NULL;
    }

    if (m_hbmpClosed)
    {
        DeleteObject(m_hbmpClosed);
        m_hbmpClosed = NULL;
    }

    if (m_hbmpOpen)
    {
        DeleteObject(m_hbmpOpen);
        m_hbmpOpen = NULL;
    }
}

LRESULT TreeView::NotifyHandler(LPNMHDR pnmhdr)
{
    if (pnmhdr->hwndFrom != m_hWnd)
    {
        return 0;
    }

    if (TVN_DELETEITEM == pnmhdr->code)
    {
        LPNMTREEVIEW pNmTreeView = reinterpret_cast<LPNMTREEVIEW>(pnmhdr);
        Directory *pDir = reinterpret_cast<Directory *>(pNmTreeView->itemOld.lParam);
        delete pDir;
    }

    if (TVN_ITEMEXPANDED == pnmhdr->code)
    {
        TVITEM item = { 0 };
        LPNMTREEVIEW pNmTreeView = reinterpret_cast<LPNMTREEVIEW>(pnmhdr);
        HTREEITEM hItem = pNmTreeView->itemNew.hItem;
        switch (pNmTreeView->action)
        {
        case TVE_EXPAND:
            item.iImage = m_nOpen;
            item.iSelectedImage = m_nOpen;
            break;
        case TVE_COLLAPSE:
            item.iImage = m_nClosed;
            item.iSelectedImage = m_nClosed;
            break;
        default:
            return 0;
            break;
        }

        item.hItem = hItem;
        item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        TreeView_SetItem(m_hWnd, &item);

        RepaintWindow();
    }

    // Secrect Sauce.  We can owner draw tree view, if we look closely at NM_CUSTOMDRAW.
    // Most controls have some flag that can be passed to CreateWindow, but not Tree View.
    if (NM_CUSTOMDRAW == pnmhdr->code)
    {
        // Return 0 here to disable custom drawing.
        //return 0;

        LPNMTVCUSTOMDRAW lpNMCustomDraw = (LPNMTVCUSTOMDRAW)pnmhdr;
        // Need CDRF_NOTIFYSUBITEMDRAW and CDRF_SKIPDEFAULT returned to prevent the control from drawing items.
        if (CDDS_PREPAINT == lpNMCustomDraw->nmcd.dwDrawStage)
        {
            return CDRF_NOTIFYSUBITEMDRAW;
        }

        if (CDDS_ITEMPREPAINT == lpNMCustomDraw->nmcd.dwDrawStage)
        {
            Directory *pDir = reinterpret_cast<Directory *>(lpNMCustomDraw->nmcd.lItemlParam);
            DirectoryDrawTextItems itemFields;
            pDir->GetDrawItems(itemFields);
            size_t totalDiskSize = SumRootSizes();
            ItemSizeText itemText(itemFields, totalDiskSize, DisplayUnits::Bytes);

            // Draw +, - and other lines.
            RECT rcLines = lpNMCustomDraw->nmcd.rc;
            rcLines.left += lpNMCustomDraw->iLevel * m_Indent;

            HTREEITEM hItem = reinterpret_cast<HTREEITEM>(lpNMCustomDraw->nmcd.dwItemSpec);
            UINT state = TreeView_GetItemState(m_hWnd, hItem, TVIF_STATE);

            // Draw a closed box.  9x9 with 5x5 plus or minus sign in it.
            POINT aPoints[5];
            const int boxSize = 8;
            int h = rcLines.bottom - rcLines.top;
            aPoints[0].y = rcLines.top + (h / 2) - boxSize / 2;
            aPoints[0].x = rcLines.left + 4;
            aPoints[1].y = aPoints[0].y;
            aPoints[1].x = aPoints[0].x + boxSize;
            aPoints[2].y = aPoints[1].y + boxSize;
            aPoints[2].x = aPoints[1].x;
            aPoints[3].y = aPoints[2].y;
            aPoints[3].x = aPoints[2].x - boxSize;
            aPoints[4].y = aPoints[3].y - boxSize;
            aPoints[4].x = aPoints[3].x;

            // Draw this box only if the item can be expanded.
            HTREEITEM hChild = TreeView_GetChild(m_hWnd, hItem);
            if (NULL != hChild)
            {
                Polyline(lpNMCustomDraw->nmcd.hdc, aPoints, 5);
            }

            int xMid = aPoints[0].x + ((aPoints[1].x - aPoints[0].x) / 2);
            int yMid = aPoints[1].y + ((aPoints[2].y - aPoints[1].y) / 2);
            const int signRadious = 3;

            if (NULL != hChild)
            {
                MoveToEx(lpNMCustomDraw->nmcd.hdc, xMid - signRadious + 1, yMid, NULL);
                LineTo(lpNMCustomDraw->nmcd.hdc, xMid + signRadious, yMid);
            }
            else
            {
                // Draw a dotted line wher the + would be in the box.
            }

            HDC hdcDraw;  // Bitmap to draw.
            if (state & TVIS_EXPANDED)
            {
                hdcDraw = m_hdcOpen;
            }
            else
            {
                if (NULL != hChild)
                {
                    MoveToEx(lpNMCustomDraw->nmcd.hdc, xMid, yMid - signRadious + 1, NULL);
                    LineTo(lpNMCustomDraw->nmcd.hdc, xMid, yMid + signRadious);
                }
                else
                {
                    DrawDottedLineDown(lpNMCustomDraw->nmcd.hdc, xMid, yMid - signRadious - 2, 2 * signRadious);
                }

                hdcDraw = m_hdcClosed;
            }

            // Draw dotted lines between boxes.
            DrawDottedLineAcross(lpNMCustomDraw->nmcd.hdc, aPoints[1].x + 1, yMid, 5);

            // Draw line going down if there is a sibling below.
            HTREEITEM hNextItemSibling = TreeView_GetNextSibling(m_hWnd, hItem);
            if (NULL != hNextItemSibling)
            {
                RECT rcNextItemSibling;
                if (TreeView_GetItemRect(m_hWnd, hNextItemSibling, &rcNextItemSibling, FALSE))
                {
                    DrawDottedLineDown(lpNMCustomDraw->nmcd.hdc, xMid, aPoints[2].y + 1, (rcNextItemSibling.top - aPoints[2].y));
                }
            }

            // Draw the Icon.
            RECT rcIcon = rcLines;
            rcIcon.left = rcLines.left + 18;
            rcIcon.right = rcIcon.left + 18;

            BitBlt(lpNMCustomDraw->nmcd.hdc, rcIcon.left, rcIcon.top, 16, 16, hdcDraw, 0, 0, SRCCOPY);

            // Now the text.  Name, Percent, and Size.
            RECT rcText = rcIcon;
            rcText.left = rcIcon.right;
            rcText.right = m_folderColumnWidth -1;
            DrawText(lpNMCustomDraw->nmcd.hdc, itemFields.szName, itemFields.cName, &rcText, DT_SINGLELINE);
            
            rcText.left = m_folderColumnWidth + 1;
            rcText.right = rcText.left + m_percentColumnWidth - 1;
            DrawText(lpNMCustomDraw->nmcd.hdc, itemText.GetPercentText(), itemText.GetPercentTextLen(), &rcText, DT_SINGLELINE | DT_CENTER);

            rcText.left += m_percentColumnWidth + 2;
            rcText.right = rcText.left + m_sizeColumnWidth - 1;
            DrawText(lpNMCustomDraw->nmcd.hdc, itemText.GetSizeText(), itemText.GetSizeTextLen(), &rcText, DT_SINGLELINE | DT_RIGHT);

            return CDRF_SKIPDEFAULT;
        }
    }

    return 0;
}

size_t TreeView::SumRootSizes()
{
    size_t sum = 0;
    TVITEM item = { 0 };
    item.hItem = TreeView_GetRoot(m_hWnd);
    item.mask = TVIF_PARAM;
    while (item.hItem)
    {
        if (TreeView_GetItem(m_hWnd, &item))
        {
            Directory *pDir = reinterpret_cast<Directory *>(item.lParam);
            sum += pDir->GetSize();
        }

        item.hItem = TreeView_GetNextSibling(m_hWnd, item.hItem);
    }

    return sum;
}

unsigned __stdcall AddDirectory(void *threadParam)
{
    Directory *pDir = reinterpret_cast<Directory *>(threadParam);
    pDir->AddSelfToTree();
    pDir->ReadSize();
    pDir->SendThreadCompletedMsg();
    return 0;
}

void TreeView::AddStartDirectory(LPCWSTR szPath)
{
    Directory *pDir = new Directory(m_hWnd, NULL, TEXT(""), szPath, m_hAbortEvent);
    LONG64 threadCount = InterlockedIncrement64(&s_cWorkerThreads);
    m_statusControl.SetNumActiveWorkerThreads(threadCount);
    unsigned threadId;
    HANDLE hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &AddDirectory, reinterpret_cast<void *>(pDir), 0, &threadId));
    ThreadInfo threadInfo(hThread, threadId);
    pDir->SetThreadHandle(threadInfo);
    m_workerThreads.insert(threadInfo);
}

void TreeView::DecrementWorkerThreadCount(const ThreadInfo &threadInfo)
{
    // I think this function and AddStartDirectory are called only on the main thread.
    m_workerThreads.erase(threadInfo);
    CloseHandle(threadInfo.GetThreadHandle());

    LONG64 threadCount = InterlockedDecrement64(&s_cWorkerThreads);
    m_statusControl.SetNumActiveWorkerThreads(threadCount);
}

void TreeView::ShutdownWorkerThreads()
{
    SetEvent(m_hAbortEvent);

    while (s_cWorkerThreads > 0)
    {
        MSG msg;
        if ((PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(100);
    }
}

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int result = 0;
    Directory *pLeft = reinterpret_cast<Directory *>(lParam1);
    Directory *pRight = reinterpret_cast<Directory *>(lParam2);
    if (pLeft->GetSize() < pRight->GetSize())
    {
        result = 1;
    }
    else if (pLeft->GetSize() > pRight->GetSize())
    {
        result = -1;
    }

    return result;
}

void TreeView::SortItems(HTREEITEM hNode)
{
    // ** This sort is not recursive! ** 
    TVSORTCB sortcpb = { 0 };
    sortcpb.lpfnCompare = CompareFunc;
    sortcpb.hParent = hNode;
    TreeView_SortChildrenCB(m_hWnd, &sortcpb, 0);
}

void TreeView::RepaintSizeStats(HTREEITEM hNode)
{
    // If hNode is visible, repaint its percent and size fields.
    RECT rcText;
    if (TreeView_GetItemRect(m_hWnd, hNode, &rcText, TRUE))
    {
        RECT rcClient;
        GetClientRect(m_hWnd, &rcClient);

        RECT rcRepaint;
        rcRepaint.top = rcText.top;
        rcRepaint.bottom = rcText.bottom;
        rcRepaint.left = m_folderColumnWidth;
        rcRepaint.right = rcClient.right;
        ::InvalidateRect(m_hWnd, &rcRepaint, TRUE);
        ::UpdateWindow(m_hWnd);
    }
}