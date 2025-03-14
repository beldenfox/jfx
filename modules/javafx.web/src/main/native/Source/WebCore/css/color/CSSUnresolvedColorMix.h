/*
 * Copyright (C) 2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ColorInterpolationMethod.h"
#include "StyleColor.h"
#include <variant>
#include <wtf/Forward.h>

namespace WebCore {

namespace Style {
enum class ForVisitedLink : bool;
}

class CSSUnresolvedColor;
class Document;
class RenderStyle;

struct CSSUnresolvedColorResolutionContext;

struct CSSUnresolvedColorMix {
    struct Component {
        bool operator==(const Component&) const;

        using Percentage = std::variant<PercentRaw, UnevaluatedCalc<PercentRaw>>;

        UniqueRef<CSSUnresolvedColor> color;
        std::optional<Percentage> percentage;
    };

    bool operator==(const CSSUnresolvedColorMix&) const = default;

    ColorInterpolationMethod colorInterpolationMethod;
    Component mixComponents1;
    Component mixComponents2;
};

PercentRaw resolveComponentPercentage(const CSSUnresolvedColorMix::Component::Percentage&);

void serializationForCSS(StringBuilder&, const CSSUnresolvedColorMix&);
String serializationForCSS(const CSSUnresolvedColorMix&);

StyleColor createStyleColor(const CSSUnresolvedColorMix&, const Document&, RenderStyle&, Style::ForVisitedLink);
Color createColor(const CSSUnresolvedColorMix&, const CSSUnresolvedColorResolutionContext&);

bool containsCurrentColor(const CSSUnresolvedColorMix&);
bool containsColorSchemeDependentColor(const CSSUnresolvedColorMix&);

} // namespace WebCore
