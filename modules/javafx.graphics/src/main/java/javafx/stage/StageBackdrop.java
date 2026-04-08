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

package javafx.stage;

import javafx.application.ConditionalFeature;
import javafx.application.Platform;

import java.util.List;

import com.sun.javafx.tk.Toolkit;

/**
 * The backdrop of a {@code Stage}. Backdrops are visual effects drawn across
 * the entire stage behind the Scene's fill and background. The specific
 * effects vary but in general the backdrop will track the window's color
 * scheme.
 *
 * Platforms which support backdrops will always provide two default
 * materials. The "Window" material is appropriate for stages where the
 * backdrop effect will be visible across the window. The "Partial" material
 * is appropriate for stages where the backdrop will only be partially
 * visible.
 *
 * @since 27
 */
@Deprecated(since = "27")
public final class StageBackdrop {
    private String material;

    private StageBackdrop(String material) {
        this.material = material;
    }

    /**
     * Gets the backdrop's material
     * @return The material of the backdrop
     */
    public String getMaterial() {
        return material;
    }

    /**
     * Gets all the backdrop materials supported on this system.
     * For systems where backdrops are not supported this will
     * be an empty list.
     * @return The list of all the supported materials.
     */
    public static List<String> getMaterials() {
        return Toolkit.getToolkit().getBackdropMaterials();
    }

    /**
     * Constructs a backdrop using the specified material.
     * @param material The material to use for the backdrop.
     * @return The backdrop if supported. Otherwise null.
     */
    public static StageBackdrop backdrop(String material) {
        if (getMaterials().contains(material)) {
            return new StageBackdrop(material);
        }
        return null;
    }
}

