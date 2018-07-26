#pragma once
class StatusControl : public Window
{
public:
    StatusControl() {}

    BOOL Create(HWND hwndParent, HINSTANCE hInstance, ULONG64 id);

    void SetNumActiveWorkerThreads(LONG64 cThreads);
};

