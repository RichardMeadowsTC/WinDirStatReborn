#pragma once

INT_PTR CALLBACK Select(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

class SelectDirectories : public Window
{
    HINSTANCE m_hInstance;
    HWND m_hwndParent;
    HWND m_hwndList;
    std::set<std::wstring> m_searchDirectories;
    typedef std::set<std::wstring>::const_iterator DirItor;
public:
    SelectDirectories() : m_hInstance(NULL), m_hwndParent(NULL)
    {

    }

    INT_PTR DoModal(HINSTANCE hInstance, HWND hwndParent);

    void Initialize();

    void OnAddFolder();
    void UpdateSelection(LPCWSTR szFolder, bool bAdd);
    std::set<std::wstring> *GetFolderListPtr()
    {
        return &m_searchDirectories;
    }
};

