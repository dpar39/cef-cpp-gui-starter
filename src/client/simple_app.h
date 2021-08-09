#pragma once

class ServerApp;

#include "include/cef_app.h"

// Implement application-level callbacks for the browser process.
class SimpleApp final : public CefApp, public CefBrowserProcessHandler
{
public:
    SimpleApp();

    // CefApp methods:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;

    // CefBrowserProcessHandler methods:
    void OnContextInitialized() override;

private:
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(SimpleApp);

private:
    std::shared_ptr<ServerApp> _server;
};
