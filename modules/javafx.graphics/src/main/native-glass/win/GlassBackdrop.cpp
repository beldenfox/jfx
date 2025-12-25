/*
 * Copyright (c) 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include "GlassBackdrop.h"

// For DWM-supplied backdrops
#include <Dwmapi.h>

// For Composition-supplied backdrops
#include <DispatcherQueue.h>
#include <Windows.Graphics.h>
#include <Windows.UI.Composition.Interop.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <d3d11.h>

#include <iostream>

using namespace winrt;
using namespace Windows::Graphics;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;

static bool s_useSystemBackdrop = false;

class SystemGlassBackdrop : public GlassBackdrop
{
private:
    HWND m_hwnd;

public:
    SystemGlassBackdrop(HWND hWnd, Style style) : m_hwnd(hWnd) {
        DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_TRANSIENTWINDOW;
        switch (style) {
            case Window:
                type = DWMSBT_MAINWINDOW;
                break;
            case Tabbed:
                type = DWMSBT_TABBEDWINDOW;
                break;
            case Transient:
                type = DWMSBT_TRANSIENTWINDOW;
                break;
        }
        DwmSetWindowAttribute(m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

        // In case the user asks for the accent color to tint the title bar. We
        // don't want DWM to draw this since it doesn't know the correct height.
        COLORREF captionColor = DWMWA_COLOR_NONE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    }

    virtual ~SystemGlassBackdrop() {
        DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_AUTO;
        DwmSetWindowAttribute(m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

        COLORREF captionColor = DWMWA_COLOR_DEFAULT;
        DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    }
};

class CompositionGlassBackdrop : public GlassBackdrop
{
private:
    HWND m_hwnd;

    inline static PDISPATCHERQUEUECONTROLLER    s_controller = nullptr;
    inline static com_ptr<ID3D11Device>         s_d3dDevice = nullptr;
    inline static com_ptr<ID3D11DeviceContext>  s_d3dDeviceContext = nullptr;
    inline static int                           s_usageCount = 0;

    com_ptr<ID3D11Texture2D>  m_sharedTexture = nullptr;
    HANDLE                    m_sharedTextureHandle = NULL;

    DesktopWindowTarget       m_windowTarget = nullptr;

    SIZE                      m_size;
    CompositionGraphicsDevice m_graphicsDevice = nullptr;
    CompositionDrawingSurface m_contentSurface = nullptr;
    SpriteVisual              m_contentVisual = nullptr;

    SIZE GetSize() {
        RECT rect;
        ::GetClientRect(m_hwnd, &rect);
        return SIZE{rect.right - rect.left, rect.bottom - rect.top};
    }

    void EnsureDispatcherQueueController() {
        if (s_controller != nullptr) {
            return;
        }

        DispatcherQueueOptions options = {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_NONE
        };

        PDISPATCHERQUEUECONTROLLER controller = nullptr;
        CreateDispatcherQueueController(options, &s_controller);
    }

    void EnsureD3DDevice() {
        if (s_d3dDevice != nullptr) {
            return;
        }

        unsigned flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        #ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif

        D3D11CreateDevice(
            nullptr,        // adapter
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,        // module
            flags,
            nullptr, 0,     // feature level
            D3D11_SDK_VERSION,
            s_d3dDevice.put(),
            nullptr,
            s_d3dDeviceContext.put());
    }

    void BuildSharedTexture(SIZE size) {
        if (s_d3dDevice == nullptr) return;
        m_sharedTexture = nullptr;
        D3D11_TEXTURE2D_DESC desc = { 0 };
        desc.Width = size.cx;
        desc.Height = size.cy;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        s_d3dDevice->CreateTexture2D(&desc, nullptr, m_sharedTexture.put());
    }

    void RemoveVisuals() {
    }

    void BuildBackdropVisuals() {
        // This contains all the content that is not reliant on the D3D device.
        namespace abi = ABI::Windows::UI::Composition;

        try {
            Compositor compositor;
            auto desktopInterop = compositor.as<abi::Desktop::ICompositorDesktopInterop>();

            check_hresult(desktopInterop->CreateDesktopWindowTarget(m_hwnd, false,
                reinterpret_cast<abi::Desktop::IDesktopWindowTarget**>(put_abi(m_windowTarget))));

            // The blurred backdrop
             BOOL enable = TRUE;
            DwmSetWindowAttribute(m_hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &enable, sizeof(enable));

            auto backdrop = compositor.CreateSpriteVisual();
            backdrop.RelativeSizeAdjustment({ 1.0f, 1.0f });
            backdrop.Brush(compositor.CreateHostBackdropBrush());

            m_contentVisual = compositor.CreateSpriteVisual();

            auto root = compositor.CreateContainerVisual();
            root.RelativeSizeAdjustment({ 1.0f, 1.0f });

            root.Children().InsertAtTop(backdrop);
            root.Children().InsertAtTop(m_contentVisual);

            m_windowTarget.Root(root);
        } catch (winrt::hresult_error const&) {
            RemoveVisuals();
        }
    }

    void BuildContentSurface() {
        if (s_d3dDevice == nullptr) return;

        namespace abi = ABI::Windows::UI::Composition;

        try {
            Compositor compositor = m_windowTarget.Compositor();

            // The foreground JavaFX content
            auto compositorInterop = compositor.as<abi::ICompositorInterop>();
            check_hresult(compositorInterop->CreateGraphicsDevice(s_d3dDevice.get(),
                reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

            m_size = GetSize();
            m_contentSurface = m_graphicsDevice.CreateDrawingSurface2(SizeInt32{m_size.cx, m_size.cy},
                DirectXPixelFormat::B8G8R8A8UIntNormalized,
                DirectXAlphaMode::Premultiplied);
            m_contentVisual.Brush(compositor.CreateSurfaceBrush(m_contentSurface));
            m_contentVisual.Size({float(m_size.cx), float(m_size.cy)});
        } catch (winrt::hresult_error const&) {
            RemoveVisuals();
        }
    }

public:
    CompositionGlassBackdrop(HWND hWnd, Style style) : m_hwnd(hWnd) {
        EnsureDispatcherQueueController();
        EnsureD3DDevice();
        BuildBackdropVisuals();
        BuildContentSurface();
        s_usageCount += 1;

        DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_TRANSIENTWINDOW;
        switch (style) {
            case Window:
                type = DWMSBT_MAINWINDOW;
                break;
            case Tabbed:
                type = DWMSBT_TABBEDWINDOW;
                break;
            case Transient:
                type = DWMSBT_TRANSIENTWINDOW;
                break;
        }
        DwmSetWindowAttribute(m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

        COLORREF captionColor = DWMWA_COLOR_NONE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    }

    virtual ~CompositionGlassBackdrop() {
        RemoveVisuals();
        if (s_usageCount == 1) {
            s_d3dDeviceContext = nullptr;
            s_d3dDevice = nullptr;
        }
        if (s_usageCount > 0) {
            s_usageCount -= 1;
        }

        DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_AUTO;
        DwmSetWindowAttribute(m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

        COLORREF captionColor = DWMWA_COLOR_DEFAULT;
        DwmSetWindowAttribute(m_hwnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
    }

    void SettingChanged() override {
        try {
            auto settings = UISettings();
            auto color = settings.GetColorValue(UIColorType::Background);
            std::cout << "New color " << (int) color.R << ' ' << (int) color.G << ' ' << (int) color.B << std::endl;
        }
        catch (winrt::hresult_error const&) {
        }
    }

    void BeginPaint() override {
        m_sharedTextureHandle = NULL;
        auto newSize = GetSize();
        if (newSize.cx != m_size.cx || newSize.cy != m_size.cy || m_sharedTexture == nullptr) {
            // The texture is resized immediately. Everything
            // else in the compositor state is resized when
            // drawing is done so we can update the pixels in sync.
            BuildSharedTexture(newSize);
        }

        if (m_sharedTexture == nullptr) {
            return;
        }

        auto dxgiResource = m_sharedTexture.as<IDXGIResource>();
        if (FAILED(dxgiResource->GetSharedHandle(&m_sharedTextureHandle))) {
            m_sharedTextureHandle = nullptr;
        }
    }

    void EndPaint() override {
        m_sharedTextureHandle = nullptr;
        if (m_contentSurface == nullptr || m_contentVisual == nullptr) return;

        namespace abi = ABI::Windows::UI::Composition;
        auto surfaceInterop = m_contentSurface.as<abi::ICompositionDrawingSurfaceInterop>();
        auto newSize = GetSize();
        if (newSize.cx != m_size.cx || newSize.cy != m_size.cy) {
            m_size = newSize;
            surfaceInterop->Resize(m_size);
            m_contentVisual.Size({float(m_size.cx), float(m_size.cy)});
        }

        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = m_size.cx;
        rect.bottom = m_size.cy;
        POINT offset = {};
        com_ptr<IDXGISurface> dxgiSurface;
        if (SUCCEEDED(surfaceInterop->BeginDraw(&rect,
                                        IID_PPV_ARGS(&dxgiSurface),
                                        &offset))) {
            auto target = dxgiSurface.as<ID3D11Texture2D>();
            if (target != nullptr) {
                D3D11_BOX sourceRect;
                sourceRect.left = 0;
                sourceRect.right = m_size.cx;
                sourceRect.top = 0;
                sourceRect.bottom = m_size.cy;
                sourceRect.front = 0;
                sourceRect.back = 1;
                s_d3dDeviceContext->CopySubresourceRegion(target.get(), 0,
                            offset.x, offset.y, 0,
                            m_sharedTexture.get(), 0, &sourceRect);
            }
            surfaceInterop->EndDraw();
        } else {
            std::cout << "BeginDraw failed" << std::endl;
        }
    }

    HANDLE GetNativeFrameBuffer() override {
        return m_sharedTextureHandle;
    }
};


bool GlassBackdrop::Configure(HWND hWnd) {
    DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_AUTO;
    return SUCCEEDED(DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)));
}

bool GlassBackdrop::DrawsEverything() {
    return !s_useSystemBackdrop;
}

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd, Style style) {
    return std::make_shared<CompositionGlassBackdrop>(hWnd, style);
}
