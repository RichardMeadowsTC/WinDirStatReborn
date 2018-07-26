#include "stdafx.h"
#include "Logging.h"

using namespace std;

void Logging::GetSystemMessage(DWORD dwError, wstring &sMsg)
{
    DWORD cChars;
    LPVOID lpMsgBuf = NULL;
    cChars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR)&lpMsgBuf, 0, NULL);
    if (cChars && lpMsgBuf)
    {
        sMsg = (LPWSTR)lpMsgBuf;
    }

    if (lpMsgBuf)
    {
        LocalFree(lpMsgBuf);
    }

    return;
}

void Logging::DebugSystemErrorMsg(LPCWSTR sz, DWORD dwError)
{
    HRESULT hr;
    wstring systemMsg;
    Logging::GetSystemMessage(dwError, systemMsg);
    WCHAR szCode[256];
    hr = StringCbPrintf(szCode, sizeof(szCode), TEXT(" - Error Code: 0x%08X or %d\n"), dwError, dwError);
    if (FAILED(hr))
    {
        throw hr;
    }

    wstring msg;
    msg = sz;
    msg += TEXT("  ") +systemMsg + szCode;
    OutputDebugString(msg.c_str());
    return;
}
