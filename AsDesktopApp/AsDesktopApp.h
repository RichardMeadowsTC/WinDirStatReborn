#pragma once

#include "resource.h"
#define WMU_TREE_SORT           WM_USER + 1
#define WMU_UPDATE_TREE_ITEM    WM_USER + 2
#define WMU_WORKER_FINISHED     WM_USER + 3

typedef struct tagDirectoryDrawTextItems
{
    LPCWSTR szName;
    int cName;
    size_t totalSize;
    bool bSizeKnown;
    DWORD dwFindFileError;
} DirectoryDrawTextItems;

enum DisplayUnits { Bytes, KiloBytes, MegaBytes, GigaByptes, TeraBytes };

typedef struct tagWMU_TREE_SORT_DATA
{
    HTREEITEM hTreeItemParent;
    HTREEITEM hTreeItem;
} WMU_TREE_SORT_DATA, *LPWMU_TREE_SORT_DATA;

class ThreadInfo
{
    HANDLE m_hThread;
    unsigned m_threadId;
public:
    ThreadInfo() : m_hThread(NULL), m_threadId(0) {}

    ThreadInfo(HANDLE hThread, unsigned threadId) :
        m_hThread(hThread), m_threadId(threadId)
    {
    }

    ThreadInfo(const ThreadInfo &threadInfo) :
        m_hThread(threadInfo.m_hThread), m_threadId(threadInfo.m_threadId)
    { 
    }

    ThreadInfo &operator=(const ThreadInfo &threadInfo)
    {
        m_hThread = threadInfo.m_hThread;
        m_threadId = threadInfo.m_threadId;
        return *this;
    }

    bool operator<(const ThreadInfo &threadInfo) const
    {
        return m_threadId < threadInfo.m_threadId;
    }

    HANDLE GetThreadHandle() const
    {
        return m_hThread;
    }
};
