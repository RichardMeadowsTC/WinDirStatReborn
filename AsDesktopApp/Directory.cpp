#include "stdafx.h"
#include "AsDesktopApp.h"
#include "Directory.h"

#include "Logging.h"

Directory::Directory(HWND hwndTree, Directory *pParent, LPCWSTR szParentPath, LPCWSTR szName,
                        HANDLE hAbortEvent) :
    m_hwndTree(hwndTree), m_pParent(pParent), m_hTreeItem(NULL), m_bSizeKnown(false)
{
    m_hAbortEvent = hAbortEvent;
    m_ParentPath = szParentPath;
    m_Name = szName;
    if (0 == m_ParentPath.length())
    {
        m_FullName = m_Name;
    }
    else
    {
        m_FullName = m_ParentPath + TEXT("\\") + m_Name;
    }

    m_SearchString = m_FullName + TEXT("\\*");

    m_FileBytes = 0;
    m_FolderBytes = 0;
    m_dwFindFileError = NOERROR;
}

void Directory::AddSelfToTree()
{
    TVINSERTSTRUCT tv = { 0 };
    if (m_pParent)
    {
        m_hTreeItemParent = m_pParent->GetTreeItemHandle();
        
    }
    else
    {
        m_hTreeItemParent = TVI_ROOT;
    }
    tv.hParent = m_hTreeItemParent;
    tv.hInsertAfter = TVI_LAST;
    tv.item.pszText = const_cast<LPWSTR>(m_Name.c_str());
    tv.item.iImage = 0;  // m_nClosed  Would fix this.  Going to be using owner draw code in TreeView.cpp that will have to render these bitmaps.
    tv.item.iSelectedImage = 0; // m_nClosed;
    tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tv.item.lParam = reinterpret_cast<LPARAM>(this);
    m_hTreeItem = TreeView_InsertItem(m_hwndTree, &tv);
}

void Directory::ReadSize()
{
    // May want to enable long file names.
    // Starting in Windows 10, version 1607
    // REF: https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file
    // HKLM\SYSTEM\CurrentControlSet\Control\FileSystem LongPathsEnabled (Type: REG_DWORD)
    // Or Manifest:
    //<application xmlns="urn:schemas-microsoft-com:asm.v3">
    //  <windowsSettings xmlns : ws2 = "http://schemas.microsoft.com/SMI/2016/WindowsSettings">
    //    <ws2:longPathAware>true< / ws2:longPathAware>
    //  </windowsSettings>
    //</application>
    //
    // Or use the \\?\ prefix.  Assuming C++ app.

    WIN32_FIND_DATA findData = { 0 };
    HANDLE hFind = FindFirstFile(m_SearchString.c_str(), &findData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        m_dwFindFileError = GetLastError();
        std::wstring debugMsg;
        debugMsg = TEXT("No file found in ") + m_SearchString;
        Logging::DebugSystemErrorMsg(debugMsg.c_str(), m_dwFindFileError);
    }
    else
    {
        bool bAbortSignaled = false;
        do
        {
            // May need to get handle.  Not right now. Doing so will require opening the file to get 
            // a handle. Is that even possible with directories?
            // REF: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstfilea
            //      Note  In rare cases or on a heavily loaded system, file attribute information on NTFS file systems 
            //      may not be current at the time this function is called.To be assured of getting the current NTFS 
            //      file system file attributes, call the GetFileInformationByHandle function.
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                DWORD dwWaitReturn = WaitForSingleObject(m_hAbortEvent, 0);
                bAbortSignaled = (WAIT_OBJECT_0 == dwWaitReturn);
                m_cFolders++;
                if (0 != wcscmp(findData.cFileName, TEXT(".")) && 0 != wcscmp(findData.cFileName, TEXT("..")))
                {
                    // Add new folder to the tree view.
                    Directory *pFolder = new Directory(m_hwndTree, this, m_FullName.c_str(), findData.cFileName, m_hAbortEvent);
                    pFolder->AddSelfToTree();
                    pFolder->ReadSize();
                    AddFolderSize(pFolder->GetName(), pFolder->GetSize());
                }
            }
            else
            {
                m_cFiles++;
                // This formula does not work!
                // ULONG64 bytes = (findData.nFileSizeHigh * (MAXDWORD + 1)) + findData.nFileSizeLow;
                ULARGE_INTEGER largeBytes;
                largeBytes.HighPart = findData.nFileSizeHigh;
                largeBytes.LowPart = findData.nFileSizeLow;
                //if (bytes != largeBytes.QuadPart)
                //{
                //    ULONG64 b2 = largeBytes.QuadPart;
                //    throw E_FAIL;
                //}

                // size_t is same size as the ULARGE_INTEGER's QuadPart.  unsigned long long VS unsigned __int64.
                // Both are 8 bytes long. 64 bits.
                // int fileByteSize = sizeof(m_FileBytes);
                // int largeBytesSize = sizeof(largeBytes);
                // 2^64 should be large enough for my folder sizes in bytes.

                m_FileBytes += largeBytes.QuadPart;
            }

        } while (false == bAbortSignaled && FindNextFile(hFind, &findData));

        FindClose(hFind);
        m_bSizeKnown = true;
        DoubleCheckFolderSizes();
    }

    WMU_TREE_SORT_DATA data;
    data.hTreeItem = m_hTreeItem;
    data.hTreeItemParent = m_hTreeItemParent;
    SendMessage(m_hwndTree, WMU_TREE_SORT, 0, reinterpret_cast<LPARAM>(&data));
}

void Directory::AddFolderSize(LPCWSTR szName, size_t Size)
{
    m_FolderSizes[szName] = Size;
    m_FolderBytes += Size;
}

void Directory::DoubleCheckFolderSizes()
{
    size_t checkedFolderBytes = 0;
    FolderSizesItor itor = m_FolderSizes.begin();
    while (itor != m_FolderSizes.end())
    {
        checkedFolderBytes += itor->second;
        itor++;
    }

    if (m_FolderBytes != checkedFolderBytes)
    {
        throw new std::exception("Folder size doesn't add up.");
    }
}