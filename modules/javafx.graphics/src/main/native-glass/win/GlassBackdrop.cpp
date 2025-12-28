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

  One approach would be to use SystemGlassBackdrop whenever possible and treat
  CompositionGlassBackdrop as a fallback eventually phasing it out entirely.
  But there are differences in the behavior as outlined in the issues below
  so that becomes complicated.

  Known issues

  Device synchronization (possible showstopper) - CompositionGlassBackdrop
  uses the existing begin/end/getNativeFrameBuffer protocol to redirect
  Prism's output to the correct composition layer. It uses D3D11 to create a
  shared texture which is passed to Prism to draw on. Prism is currently
  based on D3D9 and so uses a different D3D device than the Glass platform
  code. It looks like we're encountering problems flushing the D3D9 GPU
  commands to the texture before the D3D11 device can pull pixels from it.
  This is particularly noticable if you drag a window corner to rapidly
  resize it.

  (There's work in progress to add a D3D12 backend to Prism. That won't allow
  us to share the D3D device since Windows.UI.Composition only works with
  D3D11. There is the possibility that D3D12 can create a composition swap
  chain directly and we can bypass the begin/end/getNativeFramebuffer
  protocol entirely.)

  Title bars - SystemGlassBackdrop extends the effect into the title bar area.
  CompositionGlassBackdrop does not and only produces a satisfying effect for
  stages like EXTENDED that don't contain platform title bars.

  Dark mode - SystemGlassBackdrop tracks the per-window DWM immersive dark
  mode setting (both dark mode and backdrops are DWM features). I have not
  found a way for CompositionGlassBackdrop to retrieve that color. At the
  moment CompositionGlassBackdrop tracks the global window background color
  and SystemGlassBackdrop tracks the local window dark mode setting.
  Backdrops on macOS always track the local window dark mode setting so
  CompositionGlassBackdrop is the odd man out.

  Transparent windows - Currently on Windows if a user clicks on a fully
  transparent pixel in a TRANSPARENT stage it doesn't hit test and the click
  passes through to the window below. SystemGlassBackdrop maintains that
  behavior but CustomGlassBackdrop does not, TRANSPARENT stages register hits
  across the entire window. This is how macOS and Linux work so this may not
  be an issue.

  VSync - Not sure how to enforce vsync with CompositionGlassBackdrop.

  MSAA - CompositionGlassBackdrop does not support MSAA yet.

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

    // This always contains the most up-to-date pixels we have. It might lag
    // behind the window's size since it's only resized when new pixels are
    // delivered by the rendering thread or by uploading pixels.
    com_ptr<ID3D11Texture2D>  m_sharedTexture = nullptr;
    HANDLE                    m_sharedTextureHandle = NULL;

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

    void BuildSharedTexture() {
        if (s_d3dDevice == nullptr) return;
        auto size = GetSurfaceSize();
        if (m_sharedTexture != nullptr) {
            D3D11_TEXTURE2D_DESC desc = { 0 };
            m_sharedTexture->GetDesc(&desc);
            if (desc.Width == size.Width && desc.Height == size.Height) {
                return;
            }
        }

        D3D11_TEXTURE2D_DESC desc = { 0 };
        desc.Width = size.Width;
        desc.Height = size.Height;
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

        auto dxgiResource = m_sharedTexture.as<IDXGIResource>();
        if (FAILED(dxgiResource->GetSharedHandle(&m_sharedTextureHandle))) {
            m_sharedTextureHandle = nullptr;
        }
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

    void CopyTextureToSurface()
    {
        if (m_contentSurface == nullptr || m_contentVisual == nullptr) return;
        if (m_sharedTexture == nullptr) return;

        D3D11_TEXTURE2D_DESC desc;
        m_sharedTexture->GetDesc(&desc);
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
                            m_sharedTexture.get(), 0, &sourceRect);
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
            CopyTextureToSurface();
            m_deviceMutex.unlock();
        }
    }

    void BeginPaint() override {
        m_deviceMutex.lock();
        BuildSharedTexture();
        if (m_sharedTexture == nullptr) {
            return;
        }
    }

    void EndPaint() override {
        if (m_sharedTextureHandle != nullptr) {
            CopyTextureToSurface();
        }
        m_deviceMutex.unlock();
    }

    void UploadPixels(Pixels& p) override {
        auto size = GetSurfaceSize();
        if (m_sharedTexture && p.GetWidth() == size.Width && p.GetHeight() == size.Height) {
            D3D11_BOX destBox;
            destBox.left = 0;
            destBox.right = size.Width;
            destBox.top = 0;
            destBox.bottom = size.Height;
            destBox.front = 0;
            destBox.back = 1;
            s_d3dDeviceContext->UpdateSubresource(m_sharedTexture.get(), 0,
                &destBox, p.GetBits(), size.Width * 4, 0);
        } else {
            std::cout << "Upload failed due to size mismatch" << std::endl;
            std::cout << "Upload size " << p.GetWidth() << ' ' << p.GetHeight() << std::endl;
            std::cout << "Surface size " << size << std::endl;
        }
    }

    HANDLE GetNativeFrameBuffer() override {
        return m_sharedTextureHandle;
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
