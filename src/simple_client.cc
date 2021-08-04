#include "simple_client.h"

#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/internal/cef_ptr.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace
{

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string & data, const std::string & mime_type)
{
    return "data:" + mime_type + ";base64," + CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}
} // namespace

SimpleClient::SimpleClient(bool useViews)
: _useViews(useViews)
, _isClosing(false)
{
    /* DCHECK(!g_instance);
     g_instance = this;*/
}

CefRefPtr<CefDisplayHandler> SimpleClient::GetDisplayHandler()
{
    return this;
}

CefRefPtr<CefLifeSpanHandler> SimpleClient::GetLifeSpanHandler()
{
    return this;
}

CefRefPtr<CefLoadHandler> SimpleClient::GetLoadHandler()
{
    return this;
}

class ClientDownloadImageCallback : public CefDownloadImageCallback
{
public:
    explicit ClientDownloadImageCallback(CefRefPtr<CefWindow> window)
    : _window(window)
    {
    }

    void OnDownloadImageFinished(const CefString & image_url, int http_status_code, CefRefPtr<CefImage> image) OVERRIDE
    {
        if (image)
            _window->SetWindowAppIcon(image);
    }

private:
    CefRefPtr<CefWindow> _window;

    IMPLEMENT_REFCOUNTING(ClientDownloadImageCallback);
    DISALLOW_COPY_AND_ASSIGN(ClientDownloadImageCallback);
};

void SimpleClient::OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString> & icon_urls)
{
    CEF_REQUIRE_UI_THREAD()

    if (_useViews)
    {
        if (const auto browserView = CefBrowserView::GetForBrowser(browser))
        {
            if (const auto window = browserView->GetWindow(); window && !icon_urls.empty())
            {
                browser->GetHost()->DownloadImage(icon_urls[0], true, 16, false, new ClientDownloadImageCallback(window));
            }
        }
    }
    else
    {
        // TODO: figure out how to set it using Win32 API
    }
}

void SimpleClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title)
{
    CEF_REQUIRE_UI_THREAD()

    if (_useViews)
    {
        if (const auto browserView = CefBrowserView::GetForBrowser(browser))
        {
            if (const auto window = browserView->GetWindow())
                window->SetTitle(title);
        }
    }
    else
    {
        PlatformTitleChange(browser, title);
    }
}

void SimpleClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD()

    // Add to the list of existing browsers.
    _browserList.push_back(browser);
}

bool SimpleClient::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD()

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed description of this
    // process.
    if (_browserList.size() == 1)
    {
        // Set a flag to indicate that the window close should be allowed.
        _isClosing = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void SimpleClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Remove from the list of existing browsers.
    auto bit = _browserList.begin();
    for (; bit != _browserList.end(); ++bit)
    {
        if ((*bit)->IsSame(browser))
        {
            _browserList.erase(bit);
            break;
        }
    }

    if (_browserList.empty())
    {
        // All browser windows have closed. Quit the application message loop.
        CefQuitMessageLoop();
    }
}

void SimpleClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefFrame> frame,
                               ErrorCode errorCode,
                               const CefString & errorText,
                               const CefString & failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText) << " (" << errorCode
       << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void SimpleClient::CloseAllBrowsers(bool forceClose)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&SimpleClient::CloseAllBrowsers, this, forceClose));
        return;
    }
    for (const auto & b : _browserList)
        b->GetHost()->CloseBrowser(forceClose);
}

bool SimpleClient::IsClosing() const
{
    return _isClosing;
}
