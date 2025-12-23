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

#include <Dwmapi.h>

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
};

std::shared_ptr<GlassBackdrop> GlassBackdrop::create(HWND hWnd, Style style) {
    return std::make_shared<CustomGlassBackdrop>(hWnd, style);
}
