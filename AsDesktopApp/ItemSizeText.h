#pragma once

namespace ItemSizePrivateData {
    const int bufferLen = 256;
}

// Formats folder sizes as text.
class ItemSizeText
{
    DisplayUnits m_displayUnits;
    WCHAR m_szSize[ItemSizePrivateData::bufferLen];
    WCHAR m_szPercent[ItemSizePrivateData::bufferLen];
    int m_cchSize;
    int m_cchPercent;
public:

    ItemSizeText(const DirectoryDrawTextItems &item, const size_t totalDiskSize, const DisplayUnits displayUnits) :
        m_displayUnits(displayUnits)
    {
        double dblPercent = (item.totalSize / (double)totalDiskSize) * 100;

        switch (displayUnits)
        {
        case DisplayUnits::Bytes:
            StringCbPrintf(m_szSize, sizeof(m_szSize), TEXT("%llu bytes"), item.totalSize);
            if (item.bSizeKnown)
            {
                StringCbPrintf(m_szPercent, sizeof(m_szPercent), TEXT("%0.2f%%"), dblPercent);
            }
            else
            {
                if (item.totalSize > 0)
                {
                    StringCbPrintf(m_szPercent, sizeof(m_szPercent), TEXT("%0.2f%% - Computing"), dblPercent);
                }
                else if (NOERROR != item.dwFindFileError)
                {
                    std::wstring sErrMsg;
                    Logging::GetSystemMessage(item.dwFindFileError, sErrMsg);
                    StringCbPrintf(m_szPercent, sizeof(m_szPercent), TEXT("%s"), sErrMsg.c_str());
                }
                else
                {
                    StringCbPrintf(m_szPercent, sizeof(m_szPercent), TEXT("Waiting to be computed"));
                }
            }
            break;
        default:
            throw "Unsupported disaplyUnits";
        }

        size_t cchSize;
        HRESULT hr = StringCchLength(m_szSize, ItemSizePrivateData::bufferLen, &cchSize);
        m_cchSize = static_cast<int>(cchSize);
        if (FAILED(hr))
        {
            throw hr;
        }

        hr = StringCchLength(m_szPercent, ItemSizePrivateData::bufferLen, &cchSize);
        m_cchPercent = static_cast<int>(cchSize);
        if (FAILED(hr))
        {
            throw hr;
        }
    }

    LPCWSTR GetSizeText()
    {
        return m_szSize;
    }

    int GetSizeTextLen()
    {
        return m_cchSize;
    }

    LPCWSTR GetPercentText()
    {
        return m_szPercent;
    }

    int GetPercentTextLen()
    {
        return m_cchPercent;
    }
};