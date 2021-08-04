#ifndef SIMPLE_CLIENT_H
#define SIMPLE_CLIENT_H

#include "include/cef_client.h"

#include <list>

class SimpleClient : public CefClient, public CefDisplayHandler, public CefLifeSpanHandler, public CefLoadHandler
{
public:
    explicit SimpleClient(bool use_views);
    ~SimpleClient();

    static SimpleClient * GetInstance();

    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE
    {
        return this;
    }

    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE
    {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE
    {
        return this;
    }

    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title) OVERRIDE;
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             ErrorCode errorCode,
                             const CefString & errorText,
                             const CefString & failedUrl) OVERRIDE;

    void CloseAllBrowsers(bool force_close); 
    bool IsClosing() const
    {
        return is_closing_;
    }

private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString & title);
    const bool use_views_; 
    // List of existing browser windows. Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    BrowserList browser_list_;

    bool is_closing_;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(SimpleClient);
};

#endif