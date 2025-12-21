/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates. All rights reserved.
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

#include <DispatcherQueue.h>

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <Windows.Graphics.h>

#include <Dwmapi.h>

#include <d3d11.h>

#include <iostream>

using namespace winrt;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;
using namespace winrt::Windows::Graphics::DirectX;
using namespace Windows::Graphics;
using namespace winrt::Windows::Foundation;

namespace
{
    auto CreateDispatcherQueueController()
    {
        DispatcherQueueOptions options = {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_STA
        };

        PDISPATCHERQUEUECONTROLLER controller = nullptr;
        check_hresult(CreateDispatcherQueueController(options, &controller));
        return controller;
    }

    void AddVisual(VisualCollection const& visuals, float x, float y)
    {
        auto compositor = visuals.Compositor();
        auto visual = compositor.CreateSpriteVisual();

        static Color colors[] = {
            { 0xDC, 0x5B, 0x9B, 0xD5 },
            { 0xDC, 0xFF, 0xC0, 0x00 },
            { 0xDC, 0xED, 0x7D, 0x31 },
            { 0xDC, 0x70, 0xAD, 0x47 }
        };

        static unsigned last = 0;
        unsigned const next = ++last % _countof(colors);
        visual.Brush(compositor.CreateColorBrush(colors[next]));
        visual.Size({ 100.0f, 100.0f });
        visual.Offset({x, y, 0.0f, });
        visuals.InsertAtTop(visual);
    }
}

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
    }

    virtual ~SystemGlassBackdrop() {
        DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_AUTO;
        DwmSetWindowAttribute(m_hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
        BOOL enable = FALSE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &enable, sizeof(enable));

    }

    void Begin() override {
    }

    void End() override {
    }

    HANDLE GetNative() override {
        return NULL;
    }
};

class CustomGlassBackdrop : public GlassBackdrop
{
private:
    inline static PDISPATCHERQUEUECONTROLLER    s_controller = nullptr;
    inline static com_ptr<ID3D11Device>         s_d3dDevice = nullptr;
    inline static com_ptr<ID3D11DeviceContext>  s_d3dDeviceContext = nullptr;
    inline static int                           s_usageCount = 0;

    CompositionGraphicsDevice m_graphicsDevice = nullptr;
    CompositionDrawingSurface m_contentSurface = nullptr;
    SpriteVisual              m_contentVisual = nullptr;

    com_ptr<ID3D11Texture2D>  m_stableD3DTexture = nullptr;
    HANDLE                    m_stableD3DTextureHandle = nullptr;

    com_ptr<ID3D11Texture2D>  m_contentD3DTexture = nullptr;

    HWND                      m_hwnd;
    DesktopWindowTarget       m_target = nullptr;
    SIZE                      m_size;

    SIZE GetSize() {
        RECT rect;
        ::GetClientRect(m_hwnd, &rect);
        return SIZE{rect.right - rect.left, rect.bottom - rect.top};
    }

    void EnsureDevices() {
        if (s_controller == nullptr) {
            s_controller = CreateDispatcherQueueController();
        }

        if (s_d3dDevice == nullptr) {
            unsigned flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            #ifdef _DEBUG
            flags |= D3D11_CREATE_DEVICE_DEBUG;
            #endif

            check_hresult(D3D11CreateDevice(
                nullptr,        // adapter
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,        // module
                flags,
                nullptr, 0,     // feature level
                D3D11_SDK_VERSION,
                s_d3dDevice.put(),
                nullptr,
                s_d3dDeviceContext.put()));
        }
    }

    void BuildTargetTexture(SIZE size) {
        m_stableD3DTexture = nullptr;
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
        check_hresult(s_d3dDevice->CreateTexture2D(&desc, nullptr, m_stableD3DTexture.put()));
    }

    void BuildTarget()
    {
        BOOL enable = TRUE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &enable, sizeof(enable));

        namespace abi = ABI::Windows::UI::Composition;

        Compositor compositor;

        auto desktopInterop = compositor.as<abi::Desktop::ICompositorDesktopInterop>();
        check_hresult(desktopInterop->CreateDesktopWindowTarget(m_hwnd, false,
            reinterpret_cast<abi::Desktop::IDesktopWindowTarget**>(put_abi(m_target))));

        auto compositorInterop = compositor.as<abi::ICompositorInterop>();
        check_hresult(compositorInterop->CreateGraphicsDevice(s_d3dDevice.get(),
            reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

        auto root = compositor.CreateContainerVisual();
        root.RelativeSizeAdjustment({ 1.0f, 1.0f });

        auto backdrop = compositor.CreateSpriteVisual();
        backdrop.RelativeSizeAdjustment({ 1.0f, 1.0f });
        backdrop.Brush(compositor.CreateHostBackdropBrush());
        auto visuals = backdrop.Children();
        AddVisual(visuals, 100.0f, 100.0f);
        AddVisual(visuals, 220.0f, 100.0f);
        AddVisual(visuals, 100.0f, 220.0f);
        AddVisual(visuals, 220.0f, 220.0f);

        m_size = GetSize();
        m_contentSurface = m_graphicsDevice.CreateDrawingSurface2(SizeInt32{m_size.cx, m_size.cy},
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            DirectXAlphaMode::Premultiplied);
        m_contentVisual = compositor.CreateSpriteVisual();
        m_contentVisual.Brush(compositor.CreateSurfaceBrush(m_contentSurface));
        m_contentVisual.Size({float(m_size.cx), float(m_size.cy)});

        BuildTargetTexture(m_size);

        root.Children().InsertAtTop(backdrop);
        root.Children().InsertAtTop(m_contentVisual);

        m_target.Root(root);
    }

public:
    CustomGlassBackdrop(HWND hWnd) : m_hwnd(hWnd) {
        EnsureDevices();
        BuildTarget();
        s_usageCount += 1;
    }

    virtual ~CustomGlassBackdrop() {
        s_usageCount -= 1;
        if (s_usageCount == 0) {
            s_d3dDeviceContext = nullptr;
            s_d3dDevice = nullptr;
        }
    }

    void Begin() override {
        auto newSize = GetSize();
        if (newSize.cx != m_size.cx || newSize.cy != m_size.cy) {
            // The texture is resized immediately. Everything
            // else in the compositor state is resized when
            // drawing is done so we can update the pixels in sync.
            BuildTargetTexture(newSize);
        }

        auto dxgiResource = m_stableD3DTexture.as<IDXGIResource>();
        if (FAILED(dxgiResource->GetSharedHandle(&m_stableD3DTextureHandle))) {
            m_stableD3DTextureHandle = nullptr;
        }
    }

    void End() override {
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
                    m_stableD3DTexture.get(), 0, &sourceRect);
            }
            surfaceInterop->EndDraw();
        };
        m_stableD3DTextureHandle = nullptr;
    }

    HANDLE GetNative() override {
        return m_stableD3DTextureHandle;
    }

};

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd, Style style) {
    return std::make_shared<SystemGlassBackdrop>(hWnd, style);
}
