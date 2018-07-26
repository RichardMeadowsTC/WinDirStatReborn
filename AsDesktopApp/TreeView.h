#pragma once

class StatusControl;

class TreeView : public Window
{
    HINSTANCE m_hInstance;

    StatusControl &m_statusControl;
    UINT m_Indent;

    HTREEITEM hRoot1;

    int m_nOpen;      // Index to open image in image list.
    int m_nClosed;    // Index to close image in image list.

    HBITMAP m_hbmpOpen;
    HBITMAP m_hbmpClosed;
    HDC m_hdcOpen;
    HDC m_hdcClosed;
    HGDIOBJ m_origBmpOpen;
    HGDIOBJ m_origBmpClosed;

    int m_folderColumnWidth, m_percentColumnWidth, m_sizeColumnWidth;

    std::set<ThreadInfo> m_workerThreads;
    typedef std::set<ThreadInfo>::const_iterator WorkerThreadItor;

    HANDLE m_hAbortEvent;
public:
    TreeView(StatusControl &statusControl);
    ~TreeView();

    BOOL Create(HWND hwndParent, RECT &rect, HINSTANCE hInstance, ULONG64 id, int folderWidth, int percentWidth, int sizeWidth);
    BOOL InitImageLists();
    void Reset()
    {
        ShutdownWorkerThreads();
        RemoveAll();
        ResetEvent(m_hAbortEvent);
    }

    void ShutdownWorkerThreads();
    void RemoveAll();
    void CleanupAllResources();

    void SetFolderColumnWidth(int width) { m_folderColumnWidth = width;  }
    void SetPercentColumnWidth(int width) { m_percentColumnWidth = width;  }
    void SetSizeColumnWidth(int width) { m_sizeColumnWidth = width;  }

    void AddStartDirectory(LPCWSTR szPath);
    void SortItems(HTREEITEM hNode);
    void RepaintSizeStats(HTREEITEM hNode);
    void DecrementWorkerThreadCount(const ThreadInfo &threadInfo);

    LRESULT NotifyHandler(LPNMHDR pmnhdr);  // Handle WM_NOTIFY messages that the control sent to its parent.

protected:
    // Adds up sizes of all the items off the root.
    size_t SumRootSizes();
};

