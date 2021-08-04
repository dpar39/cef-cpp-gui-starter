#include "simple_client.h"

#include <string>
#include <windows.h>


#include "include/cef_browser.h"

void SimpleClient::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title)
{

    CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
    if (hwnd)
        SetWindowText(hwnd, std::wstring(title).c_str());
}
