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

#include <common.h>

#include "GlassBackdrop.h"

// For Composition-supplied backdrops
#include <DispatcherQueue.h>
#include <Windows.Graphics.h>
#include <Windows.UI.Composition.Interop.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <d3d11.h>
#include <mutex>

#include <iostream>

using namespace winrt;
using namespace Windows::Graphics;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;

/*
  Two different implementations of stage backdrops. SystemGlassBackdrop uses
  DwmSetWindowAttribute to invoke the system backdrops on Windows 11.
  CompositionGlassBackdrop uses Windows.UI.Composition to create an entirely
  custom backdrop implementation. The current composition backdrop visuals
  are placeholders and probably only work on Windows 11.

  This code defaults to composition backdrops unless the JFXSYSBACKDROP
  environment variable is set (value doesn't matter).

  Known issues

  Title bars - SystemGlassBackdrop extends the effect into the title bar area.
  CompositionGlassBackdrop does not and so only produces a satisfying effect
  for stages that don't contain platform title bars like Extended.

  Dark mode - SystemGlassBackdrop tracks the per-window DWM immersive dark
  mode setting (and there's no way to change this). This is also the way
  backdrops on macOS work (and there's no way to change this).
  CompositionGlassBackdrop tracks the global window background color
  preference and not the window's local dark mode setting.

  Transparent windows - Currently on Windows if a user clicks on a fully
  transparent pixel in a TRANSPARENT stage it doesn't hit test and the click
  passes through to the windows below. SystemGlassBackdrop maintains that
  behavior but CustomGlassBackdrop does not so hits register across the
  entire window. Hits register across the entire window on macOS and Linux
  also so this may not be an issue.

  VSync - Not sure how to enforce vsync with CompositionGlassBackdrop.

  Undecorated window bug - SystemGlassBackdrop draw the wrong backdrop for
  UNDECORATED stages. If you alter the window's dark mode setting after it's
  shown the backdrop corrects itself.
*/

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

static std::ostream& operator<<(std::ostream& strm, const SizeInt32& sz) {
    strm << sz.Width << ' ' << sz.Height;
    return strm;
}

class CompositionGlassBackdrop : public GlassBackdrop
{
private:
    HWND  m_hwnd;
    Style m_style;

    inline static PDISPATCHERQUEUECONTROLLER    s_controller = nullptr;
    inline static com_ptr<ID3D11Device>         s_d3dDevice = nullptr;
    inline static com_ptr<ID3D11DeviceContext>  s_d3dDeviceContext = nullptr;
    inline static int                           s_usageCount = 0;

    DesktopWindowTarget       m_windowTarget = nullptr;

    SpriteVisual              m_backdropColorOverlay = nullptr;
    Color                     m_backdropColor;

    std::mutex                m_deviceMutex;
    CompositionGraphicsDevice m_graphicsDevice = nullptr;
    CompositionDrawingSurface m_contentSurface = nullptr;
    SpriteVisual              m_contentVisual = nullptr;

    SizeInt32 GetClientSize() {
        RECT rect;
        ::GetClientRect(m_hwnd, &rect);
        return SizeInt32({rect.right - rect.left, rect.bottom - rect.top});
    }

    SizeInt32 GetSurfaceSize() {
        if (m_contentSurface) {
            return m_contentSurface.SizeInt32();
        }
        return SizeInt32({0, 0});
    }

    Color GetBackdropColor() {
        try {
            auto settings = UISettings();
            auto baseColor = settings.GetColorValue(UIColorType::Background);
            switch (m_style) {
                case Window:
                    baseColor.A = 0xD0;
                    break;
                case Tabbed:
                    baseColor.A = 0xA0;
                    break;
                case Transient:
                    baseColor.A = 0x80;
                    break;
            }
            return baseColor;
        } catch (winrt::hresult_error const&) {
        }

        return Color({0xFF, 0xFF, 0xFF, 0xFF});
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

    void RemoveVisuals() {
        // ToDo
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

            auto desktop = compositor.CreateSpriteVisual();
            desktop.RelativeSizeAdjustment({ 1.0f, 1.0f });
            desktop.Brush(compositor.CreateHostBackdropBrush());

            m_backdropColorOverlay = compositor.CreateSpriteVisual();
            m_backdropColorOverlay.RelativeSizeAdjustment({ 1.0f, 1.0f });
            m_backdropColor = GetBackdropColor();
            auto colorBrush = compositor.CreateColorBrush(m_backdropColor);
            m_backdropColorOverlay.Brush(colorBrush);

            auto backdropContainer = compositor.CreateContainerVisual();
            backdropContainer.RelativeSizeAdjustment({ 1.0f, 1.0f });
            backdropContainer.Children().InsertAtBottom(desktop);
            backdropContainer.Children().InsertAtTop(m_backdropColorOverlay);

            m_contentVisual = compositor.CreateSpriteVisual();

            auto root = compositor.CreateContainerVisual();
            root.RelativeSizeAdjustment({ 1.0f, 1.0f });

            root.Children().InsertAtTop(backdropContainer);
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

            // ::GetClientRect initially returns bogus values.
            m_contentSurface = m_graphicsDevice.CreateDrawingSurface2(SizeInt32{100, 100},
                DirectXPixelFormat::B8G8R8A8UIntNormalized,
                DirectXAlphaMode::Premultiplied);
            m_contentVisual.Brush(compositor.CreateSurfaceBrush(m_contentSurface));
            m_contentVisual.Size({100, 100});
        } catch (winrt::hresult_error const&) {
            RemoveVisuals();
        }
    }

    void CopyTextureToSurface(com_ptr<ID3D11Texture2D> texture)
    {
        if (m_contentSurface == nullptr || m_contentVisual == nullptr) return;
        if (texture == nullptr) return;

        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        auto surfaceSize = GetSurfaceSize();
        UINT srcWidth = (desc.Width < (UINT) surfaceSize.Width ? desc.Width : (UINT) surfaceSize.Width);
        UINT srcHeight = (desc.Height < (UINT) surfaceSize.Height ? desc.Height : (UINT) surfaceSize.Height);

        namespace abi = ABI::Windows::UI::Composition;
        auto surfaceInterop = m_contentSurface.as<abi::ICompositionDrawingSurfaceInterop>();

        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = surfaceSize.Width;
        rect.bottom = surfaceSize.Height;
        POINT offset = {};
        com_ptr<IDXGISurface> dxgiSurface;
        if (SUCCEEDED(surfaceInterop->BeginDraw(&rect,
                                        IID_PPV_ARGS(&dxgiSurface),
                                        &offset))) {
            auto target = dxgiSurface.as<ID3D11Texture2D>();
            if (target != nullptr) {
                D3D11_BOX sourceRect;
                sourceRect.left = 0;
                sourceRect.right = srcWidth;
                sourceRect.top = 0;
                sourceRect.bottom = srcHeight;
                sourceRect.front = 0;
                sourceRect.back = 1;
                s_d3dDeviceContext->CopySubresourceRegion(target.get(), 0,
                            offset.x, offset.y, 0,
                            texture.get(), 0, &sourceRect);
            }
            surfaceInterop->EndDraw();
        } else {
            std::cout << "BeginDraw failed" << std::endl;
            std::cout << "Asked for rect " << rect.right << ' ' << rect.bottom << std::endl;
            std::cout << "Surface size " << surfaceSize << std::endl;
        }
    }

public:
    CompositionGlassBackdrop(HWND hWnd, Style style) : m_hwnd(hWnd), m_style(style) {
        EnsureDispatcherQueueController();
        EnsureD3DDevice();
        BuildBackdropVisuals();
        BuildContentSurface();
        s_usageCount += 1;
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
    }

    void SettingChanged() override {
        if (m_backdropColorOverlay == nullptr) return;

        try {
            auto color = GetBackdropColor();
            if (color != m_backdropColor) {
                m_backdropColor = color;
                auto compositor = m_backdropColorOverlay.Compositor();
                auto animation = compositor.CreateColorKeyFrameAnimation();
                animation.InsertKeyFrame(1.0f, color);
                animation.Duration(std::chrono::seconds(1));
                m_backdropColorOverlay.Brush().StartAnimation(L"Color", animation);
            }
        }
        catch (winrt::hresult_error const&) {
        }
    }

    void Resize() override {
        if (::IsIconic(m_hwnd)) return;
        auto newSize = GetClientSize();
        auto oldSize = GetSurfaceSize();
        if (newSize != oldSize) {
            namespace abi = ABI::Windows::UI::Composition;
            m_deviceMutex.lock();
            auto surfaceInterop = m_contentSurface.as<abi::ICompositionDrawingSurfaceInterop>();
            m_contentVisual.Size({float(newSize.Width), float(newSize.Height)});
            SIZE s;
            s.cx = newSize.Width;
            s.cy = newSize.Height;
            surfaceInterop->Resize(s);
            m_deviceMutex.unlock();
        }
    }

    void BeginPaint() override {
        m_deviceMutex.lock();
    }

    void EndPaint() override {
        m_deviceMutex.unlock();
    }

    BOOL WantsTextureUpload() {
        return TRUE;
    }

    BOOL UploadTexture(HANDLE handle, UINT width, UINT height) {
        if (s_d3dDevice) {
            com_ptr<ID3D11Resource> resource;
            if (SUCCEEDED(s_d3dDevice->OpenSharedResource(handle, IID_PPV_ARGS(&resource)))) {
                auto texture = resource.as<ID3D11Texture2D>();
                if (texture != nullptr) {
                    CopyTextureToSurface(texture);
                }
            }
            return TRUE;
        }
        return FALSE;
    }
};

// Returns 'true' if backdrops are supported at all.
bool GlassBackdrop::Configure(HWND hWnd) {
    // At this point we should test to see if we're going to use
    // the system backdrop or not. For debug purposes this is
    // now hard-coded.
    DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_AUTO;
    BOOL canUseSystem = SUCCEEDED(DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type)));

    BOOL enable = TRUE;
    BOOL canUseHostBackdrop = DwmSetWindowAttribute(hWnd, DWMWA_USE_HOSTBACKDROPBRUSH, &enable, sizeof(enable));

    // At this point we should choose which we want. But instead
    // it's hard-coded to use the composition brush unless an
    // environment variable is set.
    s_useSystemBackdrop = false;

    constexpr DWORD envLen = 50;
    TCHAR buffer[envLen];

    if (::GetEnvironmentVariable(TEXT("JFXSYSBACKDROP"), buffer, envLen) != 0) {
        s_useSystemBackdrop = true;
    }

    return canUseSystem || canUseHostBackdrop;
}

bool GlassBackdrop::DrawsEverything() {
    return !s_useSystemBackdrop;
}

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd, Style style) {
    if (s_useSystemBackdrop) {
        return std::make_shared<SystemGlassBackdrop>(hWnd, style);
    }
    return std::make_shared<CompositionGlassBackdrop>(hWnd, style);
}
