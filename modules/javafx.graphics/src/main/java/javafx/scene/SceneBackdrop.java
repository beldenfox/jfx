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

import javafx.beans.NamedArg;

/**
 * Defines the backdrop of the scene. The scene's fill is composited on top of
 * this backdrop so an opaque fill will completely obscure the backdrop. To
 * see the backdrop the scene's fill and the background of the scene's root
 * node should be set to null or a non-opaque paint.
 *
 * @since 27
 */
public final class SceneBackdrop {
    /**
     * The corner radius.
     *
     * @return the corner radius.
     */
    public final double getCornerRadius() { return cornerRadius; }
    private final double cornerRadius;


    /**
     * Whether the backdrop should show a drop shadow or not.
     *
     * @return the drop shadow setting
     */
    public final boolean getUseDropShadow() { return useDropShadow; }
    private final boolean useDropShadow;

    /**
     * Creates a new backdrop with the specified corner radius and
     * drop shadow settings.
     *
     * @param cornerRadius blah
     * @param useDropShadow blah
     */
    public SceneBackdrop(
        @NamedArg("cornerRadius") double cornerRadius,
        @NamedArg("useDropShadow") boolean useDropShadow) {
        this.cornerRadius = (cornerRadius >= 0.0) ? cornerRadius : 0;
        this.useDropShadow = useDropShadow;
    }

    /**
     * Creates a new backdrop with the square corners and no
     * drop shadow.
     */
    public SceneBackdrop() {
        this(0.0, false);
    }

    /**
     * Creates a new backdrop with the specified corner radius and
     * no drop shadow.
     *
     * @param cornerRadius blah
     */
    public SceneBackdrop(@NamedArg("cornerRadius") double cornerRadius) {
        this(cornerRadius, false);
    }
};

