#pragma once

class Directory
{
    Directory() {}
    Directory(const Directory &) {}

    HWND m_hwndTree;
    Directory *m_pParent;
    HTREEITEM m_hTreeItem;
    HTREEITEM m_hTreeItemParent;

    bool m_bSizeKnown;

    int m_cFiles;   // Number of files in the current directory.
    int m_cFolders; // Number of folders in the current directory.

    size_t m_FileBytes;
    std::map<std::wstring, size_t> m_FolderSizes;
    typedef std::map<std::wstring, size_t>::const_iterator FolderSizesItor;
    size_t m_FolderBytes;

    std::wstring m_ParentPath;   // Full path to the parent directory.
    std::wstring m_Name;         // Name of this directory.
    std::wstring m_FullName;     // Full path of this directory.
    std::wstring m_SearchString;    // Parameter used for finding all files and folders in this directory.

    std::wstring m_TotalSize;
    DWORD m_dwFindFileError;

    HANDLE m_hAbortEvent;
    ThreadInfo m_threadInfo;

    // TODO: See if SHGetDiskFreeSpaceEx can be used to speed up size calculations.
    //// Random function I found that will help speed up getting disk sizes.
    //ULARGE_INTEGER ulFreeCaller;    // Amount remaining before the volume is full or disk quota is exhausted.
    //ULARGE_INTEGER ulTotal;         // Total space that the volume has.
    //ULARGE_INTEGER ulFree;          // Amount of free space remaining on the volume.
    //SHGetDiskFreeSpaceEx(TEXT("C:\\"), &ulFreeCaller, &ulTotal, &ulFree);
    //if (ulFreeCaller.QuadPart != ulFree.QuadPart)
    //{
    //    MessageBeep(0);
    //}
public:
    Directory(HWND hwndTree, Directory *pParent, LPCWSTR szParentPath, LPCWSTR szName, HANDLE hAbortEvent);

    void SetThreadHandle(ThreadInfo threadInfo)
    {
        m_threadInfo = threadInfo;
    }

    HTREEITEM GetTreeItemHandle() { return m_hTreeItem; }
    size_t GetSize() { return m_FileBytes + m_FolderBytes; }
    LPCWSTR GetName() const { return m_Name.c_str(); }

    void AddSelfToTree();

    // Gets number and size of all files.
    void ReadSize();

    void SendThreadCompletedMsg()
    {
        SendMessage(m_hwndTree, WMU_WORKER_FINISHED, 0, reinterpret_cast<LPARAM>(&m_threadInfo));
    }

    // Sets a size for a child folder.
    void AddFolderSize(LPCWSTR szName, size_t Size);
    void DoubleCheckFolderSizes();

    void GetDrawItems(DirectoryDrawTextItems &dirDrawItems)
    {
        dirDrawItems.szName = m_Name.c_str();
        dirDrawItems.cName = static_cast<int>(m_Name.length());
        dirDrawItems.totalSize = m_FileBytes + m_FolderBytes;
        dirDrawItems.bSizeKnown = m_bSizeKnown;
        dirDrawItems.dwFindFileError = m_dwFindFileError;
    }

protected:
    void UpdateTotalSizeSz()
    {
        size_t total = m_FileBytes + m_FolderBytes;
        WCHAR szTotal[256];
        StringCbPrintf(szTotal, sizeof(szTotal), TEXT("%llu"), total);
        m_TotalSize = szTotal;
    }
};
