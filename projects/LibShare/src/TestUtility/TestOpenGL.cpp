#include "targetver.h"
#include "TestUtility/TestOpenGL.h"
#include <atlbase.h>
#include <atlconv.h>
#include <dwrite.h>
#include "Log/FileLog.h"
#pragma comment(lib, "DWrite.lib")

SHARELIB_BEGIN_NAMESPACE

void PrintPixelFormat(HWND hWnd)
{
    HDC hDc = ::GetDC(hWnd);
    if (hDc)
    {
        FileLog log(NULL, NULL);
        PIXELFORMATDESCRIPTOR pfd{sizeof(pfd)};
        int n = ::DescribePixelFormat(hDc, 1, 0, 0);
        for (int i = 1; i <= n; ++i)
        {
            ::DescribePixelFormat(hDc, i, sizeof(pfd), &pfd);
            logoutex(&log).Printf(
                "%d,\n"
                "PFD_DRAW_TO_WINDOW=%d,\n"
                "PFD_DRAW_TO_BITMAP=%d,\n"
                "PFD_SUPPORT_GDI=%d,\n"
                "PFD_SUPPORT_OPENGL=%d,\n"
                "PFD_GENERIC_ACCELERATED=%d,\n"
                "PFD_GENERIC_FORMAT=%d,\n"
                "PFD_NEED_PALETTE=%d,\n"
                "PFD_NEED_SYSTEM_PALETTE=%d,\n"
                "PFD_DOUBLEBUFFER=%d,\n"
                "PFD_DOUBLEBUFFER_DONTCARE=%d,\n"
                "PFD_STEREO=%d,\n"
                "PFD_STEREO_DONTCARE=%d,\n"
                "PFD_SWAP_LAYER_BUFFERS=%d,\n"
                "PFD_DEPTH_DONTCARE=%d,\n"
                "PFD_SWAP_COPY=%d,\n"
                "PFD_SWAP_EXCHANGE=%d,\n"
                "PFD_TYPE=%s,\n"
                "cColorBits=%d,\n"
                "cAccumBits=%d,\n"
                "cDepthBits=%d,\n"
                "cStencilBits=%d,\n"
                "bReserved=%d,\n"
                "dwVisibleMask=%u,\n",
                i,
                (pfd.dwFlags & PFD_DRAW_TO_WINDOW ? 1 : 0),
                (pfd.dwFlags & PFD_DRAW_TO_BITMAP ? 1 : 0),
                (pfd.dwFlags & PFD_SUPPORT_GDI ? 1 : 0),
                (pfd.dwFlags & PFD_SUPPORT_OPENGL ? 1 : 0),
                (pfd.dwFlags & PFD_GENERIC_ACCELERATED ? 1 : 0),
                (pfd.dwFlags & PFD_GENERIC_FORMAT ? 1 : 0),
                (pfd.dwFlags & PFD_NEED_PALETTE ? 1 : 0),
                (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE ? 1 : 0),
                (pfd.dwFlags & PFD_DOUBLEBUFFER ? 1 : 0),
                (pfd.dwFlags & PFD_DOUBLEBUFFER_DONTCARE ? 1 : 0),
                (pfd.dwFlags & PFD_STEREO ? 1 : 0),
                (pfd.dwFlags & PFD_STEREO_DONTCARE ? 1 : 0),
                (pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS ? 1 : 0),
                (pfd.dwFlags & PFD_DEPTH_DONTCARE ? 1 : 0),
                (pfd.dwFlags & PFD_SWAP_COPY ? 1 : 0),
                (pfd.dwFlags & PFD_SWAP_EXCHANGE ? 1 : 0),
                (pfd.iPixelType ? "COLORINDEX" : "RGBA"),
                pfd.cColorBits,
                pfd.cAccumBits,
                pfd.cDepthBits,
                pfd.cStencilBits,
                pfd.bReserved,
                pfd.dwVisibleMask);
        }

        ::ReleaseDC(hWnd, hDc);
    }
}

void PrintSysFontFamily()
{
    ::CoInitialize(NULL);
    FileLog log(NULL, "SysFontFamily");
    {
        ATL::CComPtr<IDWriteFactory> spFactory;
        HRESULT hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE::DWRITE_FACTORY_TYPE_SHARED,
                                           __uuidof(IDWriteFactory),
                                           (IUnknown **)&spFactory);
        if (FAILED(hr))
        {
            goto ERROR_END;
        }
        ATL::CComPtr<IDWriteFontCollection> spDWrieFontCollection;
        hr = spFactory->GetSystemFontCollection(&spDWrieFontCollection);
        if (FAILED(hr))
        {
            goto ERROR_END;
        }
        UINT nFamilyCout = spDWrieFontCollection->GetFontFamilyCount();
        for (UINT i = 0; i < nFamilyCout; ++i)
        {
            ATL::CComPtr<IDWriteFontFamily> spDWriteFontFamily;
            spDWrieFontCollection->GetFontFamily(i, &spDWriteFontFamily);

            ATL::CComPtr<IDWriteLocalizedStrings> spString;
            spDWriteFontFamily->GetFamilyNames(&spString);
            UINT nNameCount = spString->GetCount();
            for (UINT j = 0; j < nNameCount; ++j)
            {
                wchar_t szName[4096] = {0};
                wchar_t szLocaleName[4096] = {0};
                spString->GetString(j, szName, 4096);
                spString->GetLocaleName(j, szLocaleName, 4096);
                try
                {
                    logout(&log) << szName << ":" << szLocaleName << "\n";
                }
                catch (...)
                {
                    logout(&log) << "未知字体:" << szLocaleName << "\n";
                }
            }
        }
    }

ERROR_END:
    ::CoUninitialize();
}

SHARELIB_END_NAMESPACE
