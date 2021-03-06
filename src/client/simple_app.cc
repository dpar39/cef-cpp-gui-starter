#include "simple_app.h"
#include "simple_client.h"

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include "Server.h"

namespace
{
// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate
{
public:
    explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
    : browser_view_(browser_view)
    {
    }

    void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE
    {
        // Add the browser view and show the window.
        window->AddChildView(browser_view_);
        window->Show();

        // Give keyboard focus to the browser view.
        browser_view_->RequestFocus();
    }

    void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE
    {
        browser_view_ = nullptr;
    }

    bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE
    {
        // Allow the window to close if the browser says it's OK.
        CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
        if (browser)
            return browser->GetHost()->TryCloseBrowser();
        return true;
    }

    CefSize GetPreferredSize(CefRefPtr<CefView> view) OVERRIDE
    {
        return CefSize(800, 600);
    }

private:
    CefRefPtr<CefBrowserView> browser_view_;

    IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
    DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

class SimpleBrowserViewDelegate : public CefBrowserViewDelegate
{
public:
    SimpleBrowserViewDelegate()
    {
    }

    bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
                                   CefRefPtr<CefBrowserView> popup_browser_view,
                                   bool is_devtools) OVERRIDE
    {
        // Create a new top-level Window for the popup. It will show itself after
        // creation.
        CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(popup_browser_view));

        // We created the Window.
        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(SimpleBrowserViewDelegate);
    DISALLOW_COPY_AND_ASSIGN(SimpleBrowserViewDelegate);
};
} // namespace

std::string GetApplicationDir()
{
    HMODULE hModule = GetModuleHandleW(NULL);
    WCHAR wpath[MAX_PATH];

    GetModuleFileNameW(hModule, wpath, MAX_PATH);
    std::wstring wide(wpath);

    std::string path = CefString(wide);
    path = path.substr(0, path.find_last_of("\\/"));
    return path;
}

SimpleApp::SimpleApp()
{
    _server = std::make_shared<Server>(GetApplicationDir() + "/../../gui");
}

CefRefPtr<CefBrowserProcessHandler> SimpleApp::GetBrowserProcessHandler()
{
    return this;
}

void SimpleApp::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();

    auto commandLine = CefCommandLine::GetGlobalCommandLine();

    const bool enableChromeRuntime = commandLine->HasSwitch("enable-chrome-runtime");

#if defined(OS_WIN) || defined(OS_LINUX)
    // Create the browser using the Views framework if "--use-views" is specified
    // via the command-line. Otherwise, create the browser using the native
    // platform framework. The Views framework is currently only supported on
    // Windows and Linux.
    const bool useViews = commandLine->HasSwitch("use-views") || true;
#else
    const bool use_views = false;
#endif

    // SimpleHandler implements browser-level callbacks.
    CefRefPtr<SimpleClient> handler(new SimpleClient(useViews));

    // Specify CEF browser settings here.
    const CefBrowserSettings browserSettings;

    std::string url;

    auto port = _server->getTcpPort();

    url = std::string("http://127.0.0.1:") + std::to_string(port) + "/index.html";

    if (useViews && !enableChromeRuntime)
    {
        // Create the BrowserView.

        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(handler,
                                                                                   url,
                                                                                   browserSettings,
                                                                                   nullptr,
                                                                                   nullptr,
                                                                                   new SimpleBrowserViewDelegate());

        // Create the Window. It will show itself after creation.
        CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
    }
    else
    {
        // Information used when creating the native window.
        CefWindowInfo window_info;
        window_info.width = 800;
        window_info.height = 600;

#if defined(OS_WIN)
        // On Windows we need to specify certain flags that will be passed to
        // CreateWindowEx().
        window_info.SetAsPopup(NULL, "CEF Gui by dpar39");
#endif

        // Create the first browser window.
        CefBrowserHost::CreateBrowser(window_info, handler, url, browserSettings, nullptr, nullptr);
    }
}
