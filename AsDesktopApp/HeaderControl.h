#pragma once
class HeaderControl : public Window
{
public:
    HeaderControl() {}

    BOOL CalcPosition(const RECT &rectParent, RECT &rect);
    BOOL Create(HWND hwndParent, HINSTANCE hInstance, ULONG64 id, int folderWidth, int percentWidth, int sizeWidth);

    void NotifyHandler(LPNMHDR pmnhdr);  // Handle WM_NOTIFY messages that the control sent to its parent.
};

