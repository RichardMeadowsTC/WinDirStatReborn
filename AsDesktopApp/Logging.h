#pragma once
class Logging
{
public:
    Logging() {}
    ~Logging() {}

    static void GetSystemMessage(DWORD dwError, std::wstring &sMsg);
    static void DebugSystemErrorMsg(LPCWSTR sz, DWORD dwErr);
};

