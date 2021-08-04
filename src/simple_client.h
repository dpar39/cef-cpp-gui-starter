#ifndef SIMPLE_CLIENT_H
#define SIMPLE_CLIENT_H

#include "include/cef_client.h"
#include <list>

class SimpleClient final : public CefClient, public CefDisplayHandler, public CefLifeSpanHandler, public CefLoadHandler
{
public:
    explicit SimpleClient(bool useViews);

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;

    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString & errorText,
                     const CefString & failedUrl) override;
    void OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString> & icon_urls) override;

    void CloseAllBrowsers(bool forceClose);
    bool IsClosing() const;

private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title);
    const bool _useViews;
    // List of existing browser windows. Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    BrowserList _browserList;
    bool _isClosing;
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(SimpleClient);
};

#endif