#include "targetver.h"
#include "Other/TraversalTree.h"
#include <cassert>
#include <vector>

using namespace std;

SHARELIB_BEGIN_NAMESPACE

static LSTATUS PrepareRegBuffer(HKEY hKey, vector<char> &buffer)
{
    DWORD dwSubKeyLen = 0;    //单位：Unicode characters
    DWORD dwClassLen = 0;     //单位：Unicode characters
    DWORD dwValueNameLen = 0; //单位：Unicode characters
    DWORD dwValueLen = 0;     //单位：bytes
    LSTATUS res = ::RegQueryInfoKeyW(hKey,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &dwSubKeyLen,
                                     &dwClassLen,
                                     NULL,
                                     &dwValueNameLen,
                                     &dwValueLen,
                                     NULL,
                                     NULL);
    if (res != ERROR_SUCCESS) {
        return res;
    } else {
        dwValueLen = dwValueLen / 2 + 1;
        DWORD dwLen = max(max(max(dwSubKeyLen, dwClassLen), dwValueNameLen), dwValueLen);
        buffer.assign((dwLen + 2) * 2, '\0');
        return res;
    }
}

LSTATUS ForEachItemInTheRegKey(const wstring &path,
                               HKEY hKey,
                               unsigned int nMaxDepth,
                               PFProcessKeyFunc pfProcessKey,
                               PFProcessValueFunc pfProcessValue)
{
    if (nMaxDepth == 0) {
        return ERROR_SUCCESS;
    }
    //准备存储空间
    vector<char> buffer;
    DWORD dwBufferLenth = 0;
    LSTATUS result = PrepareRegBuffer(hKey, buffer);
    if (result != ERROR_SUCCESS) {
        return result;
    }

    {
        //下面遍历值
        vector<char> valueData(buffer);
        DWORD dwDataBytes = 0;
        DWORD dwDataType = 0;
        result = ERROR_SUCCESS;
        for (DWORD dwIndex = 0; result == ERROR_SUCCESS; ++dwIndex) {
            dwBufferLenth = static_cast<DWORD>(buffer.size() / 2);
            dwDataBytes = static_cast<DWORD>(valueData.size());
            dwDataType = 0;
            result = ::RegEnumValueW(hKey,
                                     dwIndex,
                                     (wchar_t *)(&buffer[0]),
                                     &dwBufferLenth,
                                     NULL,
                                     &dwDataType,
                                     (LPBYTE)(&valueData[0]),
                                     &dwDataBytes);
            if (result == ERROR_NO_MORE_ITEMS) {
                break;
            }
#ifdef _DEBUG
            else if (result == ERROR_MORE_DATA) {
                ::MessageBoxW(NULL, L"ERROR_MORE_DATA, PrepareRegBuffer实现有错误!", NULL, MB_OK);
                return result;
            }
#endif
            assert(result == ERROR_SUCCESS);

            //处理末尾字符
            buffer[dwBufferLenth * 2] = '\0';
            buffer[(dwBufferLenth + 1) * 2] = '\0';
            valueData[dwDataBytes] = '\0';
            valueData[dwDataBytes + 1] = '\0';

            //(wchar_t*)&buffer[0] //值的名字
            //dwDataType// 值的类型
            //(wchar_t *)&valueData[0] //值的数据
            //dwDataBytes //数据长度
            pfProcessValue(
                path, (wchar_t *)&buffer[0], dwDataType, (wchar_t *)&valueData[0], dwDataBytes);
        }
        if (result != ERROR_NO_MORE_ITEMS) {
            assert(result == ERROR_NO_MORE_ITEMS);
            return result;
        }
    }

    //下面遍历子键
    result = ERROR_SUCCESS;
    for (DWORD dwIndex = 0; result == ERROR_SUCCESS; ++dwIndex) {
        dwBufferLenth = static_cast<DWORD>(buffer.size() / 2);
        result = ::RegEnumKeyExW(
            hKey, dwIndex, (wchar_t *)(&buffer[0]), &dwBufferLenth, NULL, NULL, NULL, NULL);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
#ifdef _DEBUG
        else if (result == ERROR_MORE_DATA) {
            ::MessageBoxW(NULL, L"ERROR_MORE_DATA, PrepareRegBuffer实现有错误!", NULL, MB_OK);
            return result;
        }
#endif
        assert(result == ERROR_SUCCESS);

        //处理末尾字符
        buffer[dwBufferLenth * 2] = '\0';
        buffer[(dwBufferLenth + 1) * 2] = '\0';
        //(wchar_t*)&buffer[0]) 键的名称
        REGSAM samDesired = KEY_READ;
        if (nMaxDepth > 0 && pfProcessKey(path, (wchar_t *)&buffer[0], samDesired)) //处理并过滤键
        {
            //递归处理子键
            HKEY hSubKey = NULL;
            if (ERROR_SUCCESS ==
                ::RegOpenKeyExW(hKey, (wchar_t *)&buffer[0], 0, samDesired, &hSubKey)) {
                ForEachItemInTheRegKey(path + L"\\" + (wchar_t *)&buffer[0],
                                       hSubKey,
                                       nMaxDepth - 1,
                                       pfProcessKey,
                                       pfProcessValue);
                ::RegCloseKey(hSubKey);
            }
        }
    }
    if (result != ERROR_NO_MORE_ITEMS) {
        assert(result == ERROR_NO_MORE_ITEMS);
        return result;
    }

    return ERROR_SUCCESS;
}

SHARELIB_END_NAMESPACE
