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

#include <d3d11.h>

using namespace winrt;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;
using namespace winrt::Windows::Graphics::DirectX;
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

class RealGlassBackdrop : public GlassBackdrop
{
private:
    static PDISPATCHERQUEUECONTROLLER           s_controller;
    static com_ptr<ID3D11Device>                s_d3dDevice;

    CompositionGraphicsDevice m_graphicsDevice = nullptr;
    CompositionDrawingSurface m_contentSurface = nullptr;
    com_ptr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> m_contentSurfaceInterop = nullptr;
    com_ptr<ID3D11Texture2D>  m_contentSurfaceTexture = nullptr;
    SpriteVisual              m_contentVisual = nullptr;
    DesktopWindowTarget       m_target;

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
                nullptr));
        }
    }

    void BuildTarget(HWND hWnd)
    {
        namespace abi = ABI::Windows::UI::Composition;

        Compositor compositor;

        auto desktopInterop = compositor.as<abi::Desktop::ICompositorDesktopInterop>();
        check_hresult(desktopInterop->CreateDesktopWindowTarget(hWnd, false,
            reinterpret_cast<abi::Desktop::IDesktopWindowTarget**>(put_abi(m_target))));

        auto compositorInterop = compositor.as<abi::ICompositorInterop>();
        check_hresult(compositorInterop->CreateGraphicsDevice(s_d3dDevice.get(),
            reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

        auto root = compositor.CreateContainerVisual();
        root.RelativeSizeAdjustment({ 1.0f, 1.0f });

        auto backdrop = compositor.CreateSpriteVisual();
        backdrop.RelativeSizeAdjustment({ 1.0f, 1.0f });
        backdrop.Brush(compositor.CreateColorBrush({ 0x80, 0xEF, 0xE4, 0xB0 }));
        auto visuals = backdrop.Children();
        AddVisual(visuals, 100.0f, 100.0f);
        AddVisual(visuals, 220.0f, 100.0f);
        AddVisual(visuals, 100.0f, 220.0f);
        AddVisual(visuals, 220.0f, 220.0f);

        Size size { 100, 100 };
        m_contentSurface = m_graphicsDevice.CreateDrawingSurface(size,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            DirectXAlphaMode::Premultiplied);
        m_contentSurfaceInterop = m_contentSurface.as<abi::ICompositionDrawingSurfaceInterop>();

        m_contentVisual = compositor.CreateSpriteVisual();
        m_contentVisual.Brush(compositor.CreateSurfaceBrush(m_contentSurface));

        root.Children().InsertAtTop(backdrop);
        root.Children().InsertAtTop(m_contentVisual);

        m_target.Root(root);
    }

public:
    RealGlassBackdrop(HWND hWnd) : m_target(nullptr) {
        EnsureDevices();
        BuildTarget(hWnd);
    }

    virtual ~RealGlassBackdrop() {
    }

    void Begin() override {
        POINT offset;
        m_contentSurfaceInterop->BeginDraw(
            nullptr, // entire surface)
            _uuidof(ID3D11Texture2D),
            (void**)&m_contentSurfaceTexture,
            &offset); // offset
    }

    void End() override {
        m_contentSurfaceInterop->EndDraw();
        m_contentSurfaceTexture = nullptr;
    }
};
PDISPATCHERQUEUECONTROLLER RealGlassBackdrop::s_controller = nullptr;
winrt::com_ptr<ID3D11Device> RealGlassBackdrop::s_d3dDevice = nullptr;

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd) {
    return std::make_shared<RealGlassBackdrop>(hWnd);
}
