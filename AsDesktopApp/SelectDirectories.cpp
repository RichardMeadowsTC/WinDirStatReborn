#include "stdafx.h"
#include "AsDesktopApp.h"
#include "Window.h"
#include "SelectDirectories.h"

#include "Logging.h"

INT_PTR CALLBACK Select(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static SelectDirectories *pDlgSelDir = NULL;
    switch (message)
    {
    case WM_INITDIALOG:
        pDlgSelDir = reinterpret_cast<SelectDirectories *>(lParam);
        pDlgSelDir->Attach(hDlg);
        pDlgSelDir->Initialize();
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_B_ADD_FOLDER:
            pDlgSelDir->OnAddFolder();
            return (INT_PTR)TRUE;
            break;
        }

    case WM_NOTIFY:
        {
            if (IDC_LIST_FOLDERS == wParam)
            {
                LPNMHDR pnmhdr = reinterpret_cast<LPNMHDR>(lParam);
                LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(lParam);
                WCHAR szText[MAX_PATH];
                LVITEM item = { 0 };
                item.iItem = pnmv->iItem;
                item.mask = LVIF_STATE | LVIF_TEXT;
                item.stateMask = LVIS_SELECTED;
                item.pszText = szText;
                item.cchTextMax = MAX_PATH;
                ListView_GetItem(pnmhdr->hwndFrom, &item);
                if (NM_CLICK == pnmhdr->code)
                {
                    LPNMITEMACTIVATE pnmItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                    if (pnmItemActivate->ptAction.x > 20)
                    {
                        ListView_SetCheckState(pnmhdr->hwndFrom, item.iItem, (LVIS_SELECTED == item.state));
                    }
                }

                if (LVN_ITEMCHANGED == pnmhdr->code)
                {
                    LRESULT checked = ListView_GetCheckState(pnmhdr->hwndFrom, pnmv->iItem);
                    pDlgSelDir->UpdateSelection(szText, 0 != checked);
                }
            }
        }
        break;

    case WM_DESTROY:
        pDlgSelDir->Detach();
        break;
    }

    return (INT_PTR)FALSE;
}

INT_PTR SelectDirectories::DoModal(HINSTANCE hInstance, HWND hwndParent)
{
    m_hInstance = hInstance;
    m_hwndParent = hwndParent;
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SELECT_DIRECTORIES), 
                            hwndParent, Select, reinterpret_cast<LPARAM>(this));
}

void SelectDirectories::Initialize()
{
    m_hwndList = GetDlgItem(m_hWnd, IDC_LIST_FOLDERS);

    // Put the list control into report view style.  Add ownerdata
    DWORD dwStyle = GetWindowLong(m_hwndList, GWL_STYLE);
    SetWindowLong(m_hwndList, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | LVS_REPORT | LVS_OWNERDATA);

    // Add check boxes
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_CHECKBOXES | 
     //   LVS_EX_ONECLICKACTIVATE |  // Getting WM_NOTIFY messages even when this is not set.
        LVS_EX_AUTOCHECKSELECT |
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_FLATSB);

    LVCOLUMN LvColumn = { 0 };
    LvColumn.mask = LVCF_TEXT;
    LvColumn.pszText = const_cast<LPWSTR>(TEXT("Folder"));
    int col = ListView_InsertColumn(m_hwndList, 0, &LvColumn);
    ListView_SetColumnWidth(m_hwndList, col, LVSCW_AUTOSIZE_USEHEADER);

    // Add local drives
    LVITEM item = { 0 };
    item.mask = LVIF_TEXT;
    item.iItem = 0;
    WCHAR szDrives[MAX_PATH];
    GetLogicalDriveStrings(MAX_PATH, szDrives);  // And use GetDriveType to filter out removalbe drives?
    DWORD dwDriveType;
    LPWSTR pszDrive = szDrives;
    while (NULL != *pszDrive)
    {
        // Add all except for removable drives.  They can be added manually if needed.
        dwDriveType = GetDriveType(pszDrive);
        if (DRIVE_REMOVABLE != dwDriveType && DRIVE_CDROM != dwDriveType)
        {
            item.pszText = pszDrive;
            item.iItem = ListView_InsertItem(m_hwndList, &item) + 1;
        }

        pszDrive = pszDrive + wcslen(pszDrive) + 1;
    }
}

static int CALLBACK BrowseFolderCallback(
    HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        LPCTSTR path = reinterpret_cast<LPCTSTR>(lpData);
        ::SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
    }
    return 0;
}

void SelectDirectories::OnAddFolder()
{
    WCHAR szNewFolder[MAX_PATH];
    GetDlgItemText(m_hWnd, IDC_E_ADD_FOLDER, szNewFolder, MAX_PATH);

    // Get the desired folder.
    BROWSEINFO info = { 0 };
    info.hwndOwner = m_hWnd;
    info.lParam = reinterpret_cast<LPARAM>(szNewFolder);
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
    info.lpfn = BrowseFolderCallback;
    LPITEMIDLIST pItemIdList = SHBrowseForFolder(&info);
    if (NULL != pItemIdList)
    {
        // Add the directory to list.
        SHGetPathFromIDList(pItemIdList, szNewFolder);

        LVITEM item = { 0 };
        item.mask = LVIF_TEXT;
        item.pszText = szNewFolder;
        int iItem = ListView_InsertItem(m_hwndList, &item);
        ListView_SetCheckState(m_hwndList, iItem, TRUE);

        // Cleanup shell memory allocations.
        CComPtr<IMalloc> pIMaloc;
        HRESULT hr = SHGetMalloc(&pIMaloc);
        if (FAILED(hr))
        {
            throw hr;
        }

        pIMaloc->Free(pItemIdList);

        // Clear out the edit box.
        SetDlgItemText(m_hWnd, IDC_E_ADD_FOLDER, NULL);
        HWND hWndEditAddFolder = GetDlgItem(m_hWnd, IDC_E_ADD_FOLDER);
        SetFocus(hWndEditAddFolder);
    }
}

void SelectDirectories::UpdateSelection(LPCWSTR szFolder, bool bAdd)
{
    std::wstring sFolder = szFolder;
    if (bAdd)
    {
        m_searchDirectories.insert(sFolder);
    }
    else
    {
        m_searchDirectories.erase(sFolder);
    }
}