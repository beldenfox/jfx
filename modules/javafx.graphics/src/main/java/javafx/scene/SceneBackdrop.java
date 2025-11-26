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

package javafx.scene;

/**
 * Defines the backdrop of the scene. The scene's fill is composited on top of
 * this backdrop so an opaque fill will completely obscure the backdrop. To
 * see the backdrop the scene's fill and the background of the scene's root
 * node should be set to null or a non-opaque paint.
 *
 * @since 27
 */
public enum SceneBackdrop {
    /**
     * The default (legacy) backdrop which varies based on the StageStyle.
     */
    DEFAULT,

    /**
     * A platform-specific backdrop. This may include advanced visual effects
     * like translucency or it may be opaque. It is always a suitable
     * background for drawing text in the default text color. The platform
     * backdrop responds to platform preferences such as the color scheme and
     * whether the user has asked to reduce transparency. Its appearance may
     * change based on the focused state of the window.
     */
    PLATFORM
}
