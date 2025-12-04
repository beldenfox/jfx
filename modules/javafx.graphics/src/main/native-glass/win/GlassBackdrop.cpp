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

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

using namespace winrt;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Desktop;

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

    auto CreateDesktopWindowTarget(const Compositor &compositor, HWND hWnd)
    {
        namespace abi = ABI::Windows::UI::Composition::Desktop;
        auto interop = compositor.as<abi::ICompositorDesktopInterop>();
        DesktopWindowTarget target = nullptr;
        check_hresult(interop->CreateDesktopWindowTarget(hWnd, false,
            reinterpret_cast<abi::IDesktopWindowTarget**>(put_abi(target))));
        return target;
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

    DesktopWindowTarget PrepareVisuals(HWND hWnd)
    {
        Compositor compositor;
        auto target = CreateDesktopWindowTarget(compositor, hWnd);
        auto root = compositor.CreateSpriteVisual();
        root.RelativeSizeAdjustment({ 1.0f, 1.0f });
        root.Brush(compositor.CreateColorBrush({ 0x80, 0xEF, 0xE4, 0xB0 }));
        target.Root(root);
        auto visuals = root.Children();
        AddVisual(visuals, 100.0f, 100.0f);
        AddVisual(visuals, 220.0f, 100.0f);
        AddVisual(visuals, 100.0f, 220.0f);
        AddVisual(visuals, 220.0f, 220.0f);
        return target;
    }
}

class RealGlassBackdrop : public GlassBackdrop
{
public:
    RealGlassBackdrop(HWND hWnd) : m_target(nullptr) {
        if (s_controller == nullptr) {
            s_controller = CreateDispatcherQueueController();
        }

        m_target = PrepareVisuals(hWnd);
    }

    virtual ~RealGlassBackdrop() {
    }

private:
    static PDISPATCHERQUEUECONTROLLER s_controller;
    DesktopWindowTarget m_target;
};
PDISPATCHERQUEUECONTROLLER RealGlassBackdrop::s_controller = nullptr;

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd) {
    return std::make_shared<RealGlassBackdrop>(hWnd);
}
