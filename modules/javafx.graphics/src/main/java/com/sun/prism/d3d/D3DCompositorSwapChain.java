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

package com.sun.prism.d3d;

import com.sun.glass.ui.Screen;
import com.sun.javafx.geom.Rectangle;
import com.sun.prism.Graphics;
import com.sun.prism.GraphicsResource;
import com.sun.prism.Presentable;
import com.sun.prism.PresentableState;
import com.sun.prism.impl.PrismSettings;

class D3DCompositorSwapChain implements Presentable, GraphicsResource {

    private final D3DContext pContext;
    private D3DRTTexture stableBackbuffer;
    private final float pixelScaleFactorX;
    private final float pixelScaleFactorY;
    private final int w, h;

    public D3DCompositorSwapChain(D3DContext context, PresentableState state, D3DRTTexture texture) {
        pContext = context;
        stableBackbuffer = texture;

        pixelScaleFactorX = state.getRenderScaleX();
        pixelScaleFactorY = state.getRenderScaleY();
        w = state.getRenderWidth();
        h = state.getRenderHeight();
    }

    @Override
    public boolean lockResources(PresentableState state) {
        if (w != state.getRenderWidth() ||
            h != state.getRenderHeight() ||
            pixelScaleFactorX != state.getRenderScaleX() ||
            pixelScaleFactorY != state.getRenderScaleY()) {
            stableBackbuffer.dispose();
            stableBackbuffer = null;
            return true;
        }
        stableBackbuffer.lock();
        if (stableBackbuffer.isSurfaceLost()) {
            stableBackbuffer.dispose();
            stableBackbuffer = null;
        }
        return stableBackbuffer == null;
    }

    @Override
    public boolean prepare(Rectangle dirtyregion) {
        D3DContext context = getContext();
        context.flushVertexBuffer();
        D3DGraphics g = (D3DGraphics) D3DGraphics.create(stableBackbuffer, context);
        if (g == null) {
            return false;
        }
        stableBackbuffer.unlock();
        return true;
    }

    private static native int nSync(long context);

    public D3DContext getContext() {
        return pContext;
    }

    @Override
    public boolean present() {
        D3DContext context = getContext();
        if (context.isDisposed()) {
            return false;
        }
        int res = nSync(context.getContextHandle());
        return context.validatePresent(res);
    }

    @Override
    public float getPixelScaleFactorX() {
        return pixelScaleFactorX;
    }

    @Override
    public float getPixelScaleFactorY() {
        return pixelScaleFactorY;
    }

    @Override
    public Screen getAssociatedScreen() {
        return getContext().getAssociatedScreen();
    }

    @Override
    public Graphics createGraphics() {
        Graphics g = D3DGraphics.create(stableBackbuffer, getContext());
        if (g == null) {
            return null;
        }
        g.scale(pixelScaleFactorX, pixelScaleFactorY);
        return g;
    }

    @Override
    public boolean isOpaque() {
        return false;
    }

    @Override
    public void setOpaque(boolean opaque) {
    }

    @Override
    public boolean isMSAA() {
        return false;
    }

    @Override
    public int getPhysicalWidth() {
        return w;
    }

    @Override
    public int getPhysicalHeight() {
        return h;
    }

    @Override
    public int getContentX() {
        return 0;
    }

    @Override
    public int getContentY() {
        return 0;
    }

    @Override
    public int getContentWidth() {
        return w;
    }

    @Override
    public int getContentHeight() {
        return h;
    }

    @Override
    public void dispose() {
        if (stableBackbuffer != null) {
            stableBackbuffer.dispose();
            stableBackbuffer = null;
        }
    }
}
