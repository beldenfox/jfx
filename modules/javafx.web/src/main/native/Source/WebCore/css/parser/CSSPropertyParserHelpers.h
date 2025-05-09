// Copyright 2016 The Chromium Authors. All rights reserved.
// Copyright (C) 2016-2023 Apple Inc. All rights reserved.
// Copyright (C) 2024 Samuel Weinig <sam@webkit.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "CSSFunctionValue.h"
#include "CSSParserContext.h"
#include "CSSParserTokenRange.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyParserConsumer+Primitives.h"
#include "CSSShadowValue.h"
#include "CSSValuePool.h"
#include "GridArea.h"
#include "Length.h"
#include "SystemFontDatabase.h"
#include <variant>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSGridLineNamesValue;

namespace WebKitFontFamilyNames {
enum class FamilyNamesIndex;
}

enum class FontTechnology : uint8_t;

// When these functions are successful, they will consume all the relevant
// tokens from the range and also consume any whitespace which follows. When
// the start of the range doesn't match the type we're looking for, the range
// will not be modified.
namespace CSSPropertyParserHelpers {

RefPtr<CSSPrimitiveValue> consumeFontWeightNumber(CSSParserTokenRange&);

enum class AllowedFilterFunctions {
    PixelFilters,
    ColorFilters
};

RefPtr<CSSValue> consumeFilter(CSSParserTokenRange&, const CSSParserContext&, AllowedFilterFunctions);
RefPtr<CSSShadowValue> consumeSingleShadow(CSSParserTokenRange&, const CSSParserContext&, bool allowInset, bool allowSpread, bool isWebkitBoxShadow = false);

struct FontStyleRaw {
    CSSValueID style;
    std::optional<AngleRaw> angle;
};
using FontWeightRaw = std::variant<CSSValueID, double>;
using FontSizeRaw = std::variant<CSSValueID, LengthOrPercentRaw>;
using LineHeightRaw = std::variant<CSSValueID, double, LengthOrPercentRaw>;
using FontFamilyRaw = std::variant<CSSValueID, AtomString>;

struct FontRaw {
    std::optional<FontStyleRaw> style;
    std::optional<CSSValueID> variantCaps;
    std::optional<FontWeightRaw> weight;
    std::optional<CSSValueID> stretch;
    FontSizeRaw size;
    std::optional<LineHeightRaw> lineHeight;
    Vector<FontFamilyRaw> family;
};

RefPtr<CSSPrimitiveValue> consumeCounterStyleName(CSSParserTokenRange&);
AtomString consumeCounterStyleNameInPrelude(CSSParserTokenRange&, CSSParserMode = CSSParserMode::HTMLStandardMode);
RefPtr<CSSPrimitiveValue> consumeSingleContainerName(CSSParserTokenRange&);

std::optional<CSSValueID> consumeFontStretchKeywordValueRaw(CSSParserTokenRange&);
AtomString concatenateFamilyName(CSSParserTokenRange&);
AtomString consumeFamilyNameRaw(CSSParserTokenRange&);
// https://drafts.csswg.org/css-fonts-4/#family-name-value
Vector<AtomString> consumeFamilyNameListRaw(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFamilyNameList(CSSParserTokenRange&);
std::optional<FontRaw> consumeFontRaw(CSSParserTokenRange&, CSSParserMode);
const AtomString& genericFontFamily(CSSValueID);
WebKitFontFamilyNames::FamilyNamesIndex genericFontFamilyIndex(CSSValueID);

bool isFontStyleAngleInRange(double angleInDegrees);

RefPtr<CSSValue> consumeAspectRatio(CSSParserTokenRange&);

using IsPositionKeyword = bool (*)(CSSValueID);
bool isFlexBasisIdent(CSSValueID);
bool isBaselineKeyword(CSSValueID);
bool isContentPositionKeyword(CSSValueID);
bool isContentPositionOrLeftOrRightKeyword(CSSValueID);
bool isSelfPositionKeyword(CSSValueID);
bool isSelfPositionOrLeftOrRightKeyword(CSSValueID);
bool isGridBreadthIdent(CSSValueID);

RefPtr<CSSValue> consumeDisplay(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeWillChange(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeQuotes(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontSizeAdjust(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontVariantLigatures(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontVariantEastAsian(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontVariantAlternates(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontVariantNumeric(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontWeight(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFamilyName(CSSParserTokenRange&);
RefPtr<CSSValue> consumeFontFamily(CSSParserTokenRange&);
RefPtr<CSSValue> consumeCounterIncrement(CSSParserTokenRange&);
RefPtr<CSSValue> consumeCounterReset(CSSParserTokenRange&);
RefPtr<CSSValue> consumeCounterSet(CSSParserTokenRange&);
RefPtr<CSSValue> consumeSize(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeTextIndent(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeTextTransform(CSSParserTokenRange&);
RefPtr<CSSValue> consumeMarginSide(CSSParserTokenRange&, CSSPropertyID currentShorthand, CSSParserMode);
RefPtr<CSSValue> consumeMarginTrim(CSSParserTokenRange&);
RefPtr<CSSValue> consumeSide(CSSParserTokenRange&, CSSPropertyID currentShorthand, const CSSParserContext&);
RefPtr<CSSValue> consumeInsetLogicalStartEnd(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeClip(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeTouchAction(CSSParserTokenRange&);
RefPtr<CSSValue> consumeKeyframesName(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSingleTransitionPropertyOrNone(CSSParserTokenRange&);
RefPtr<CSSValue> consumeSingleTransitionProperty(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeTimingFunction(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeTextShadow(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeBoxShadow(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeWebkitBoxShadow(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeTextDecorationLine(CSSParserTokenRange&);
RefPtr<CSSValue> consumeTextEmphasisStyle(CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderWidth(CSSParserTokenRange&, CSSPropertyID currentShorthand, const CSSParserContext&);
RefPtr<CSSValue> consumeBorderColor(CSSParserTokenRange&, CSSPropertyID currentShorthand, const CSSParserContext&);
RefPtr<CSSValue> consumeTransform(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeTransformFunction(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeTranslate(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeScale(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeRotate(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeRepeatStyle(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumePaintStroke(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeListStyleType(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumePaintOrder(CSSParserTokenRange&);
RefPtr<CSSValue> consumeStrokeDasharray(CSSParserTokenRange&);
RefPtr<CSSValue> consumeCursor(CSSParserTokenRange&, const CSSParserContext&, bool inQuirksMode);
RefPtr<CSSValue> consumeAttr(CSSParserTokenRange args, const CSSParserContext&);
RefPtr<CSSValue> consumeContent(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeScrollSnapAlign(CSSParserTokenRange&);
RefPtr<CSSValue> consumeScrollSnapType(CSSParserTokenRange&);
RefPtr<CSSValue> consumeScrollbarColor(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeScrollbarGutter(CSSParserTokenRange&);
RefPtr<CSSValue> consumeTextEdge(CSSPropertyID, CSSParserTokenRange&);
RefPtr<CSSValue> consumeViewTransitionClass(CSSParserTokenRange&);
RefPtr<CSSValue> consumeViewTransitionName(CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderRadiusCorner(CSSParserTokenRange&, CSSParserMode);
bool consumeRadii(std::array<RefPtr<CSSValue>, 4>& horizontalRadii, std::array<RefPtr<CSSValue>, 4>& verticalRadii, CSSParserTokenRange&, CSSParserMode, bool useLegacyParsing);
enum PathParsingOption : uint8_t { RejectRay = 1 << 0, RejectFillRule = 1 << 1 };
RefPtr<CSSValue> consumePathOperation(CSSParserTokenRange&, const CSSParserContext&, OptionSet<PathParsingOption>);
RefPtr<CSSValue> consumePath(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeShapeOutside(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeContentDistributionOverflowPosition(CSSParserTokenRange&, IsPositionKeyword);
RefPtr<CSSValue> consumeJustifyContent(CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderImageRepeat(CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderImageSlice(CSSPropertyID, CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderImageOutset(CSSParserTokenRange&);
RefPtr<CSSValue> consumeBorderImageWidth(CSSPropertyID, CSSParserTokenRange&);
bool consumeBorderImageComponents(CSSPropertyID, CSSParserTokenRange&, const CSSParserContext&, RefPtr<CSSValue>&, RefPtr<CSSValue>&, RefPtr<CSSValue>&, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
RefPtr<CSSValue> consumeWebkitBorderImage(CSSPropertyID, CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeReflect(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSingleBackgroundClip(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeBackgroundClip(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeBackgroundSize(CSSPropertyID, CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeGridAutoFlow(CSSParserTokenRange&);
RefPtr<CSSValueList> consumeMasonryAutoFlow(CSSParserTokenRange&);
RefPtr<CSSValue> consumeSingleBackgroundSize(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSingleMaskSize(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSingleWebkitBackgroundSize(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSelfPositionOverflowPosition(CSSParserTokenRange&, IsPositionKeyword);
RefPtr<CSSValue> consumeAlignItems(CSSParserTokenRange&);
RefPtr<CSSValue> consumeJustifyItems(CSSParserTokenRange&);
RefPtr<CSSValue> consumeGridLine(CSSParserTokenRange&);
bool parseGridTemplateAreasRow(StringView gridRowNames, NamedGridAreaMap&, const size_t rowCount, size_t& columnCount);
RefPtr<CSSValue> consumeGridTrackSize(CSSParserTokenRange&, CSSParserMode);
enum class AllowEmpty : bool { No, Yes };
RefPtr<CSSGridLineNamesValue> consumeGridLineNames(CSSParserTokenRange&, AllowEmpty = AllowEmpty::No);
enum TrackListType : uint8_t { GridTemplate, GridTemplateNoRepeat, GridAuto };
RefPtr<CSSValue> consumeGridTrackList(CSSParserTokenRange&, const CSSParserContext&, TrackListType);
RefPtr<CSSValue> consumeGridTemplatesRowsOrColumns(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeGridTemplateAreas(CSSParserTokenRange&);
RefPtr<CSSValue> consumeLineBoxContain(CSSParserTokenRange&);
RefPtr<CSSValue> consumeContainerName(CSSParserTokenRange&);
RefPtr<CSSValue> consumeWebkitInitialLetter(CSSParserTokenRange&);
RefPtr<CSSValue> consumeSpeakAs(CSSParserTokenRange&);
RefPtr<CSSValue> consumeHangingPunctuation(CSSParserTokenRange&);
RefPtr<CSSValue> consumeContain(CSSParserTokenRange&);
RefPtr<CSSValue> consumeContainIntrinsicSize(CSSParserTokenRange&);
RefPtr<CSSValue> consumeTextEmphasisPosition(CSSParserTokenRange&);
#if ENABLE(DARK_MODE_CSS)
RefPtr<CSSValue> consumeColorScheme(CSSParserTokenRange&);
#endif
RefPtr<CSSValue> consumeOffsetRotate(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeTextSpacingTrim(CSSParserTokenRange&);
RefPtr<CSSValue> consumeTextAutospace(CSSParserTokenRange&);
RefPtr<CSSValue> consumeTextUnderlinePosition(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeAnimationTimeline(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeSingleAnimationTimeline(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeAnimationTimelineScroll(CSSParserTokenRange&);
RefPtr<CSSValue> consumeAnimationTimelineView(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeViewTimelineInsetListItem(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeViewTimelineInset(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSPrimitiveValue> consumeAnchor(CSSParserTokenRange&, CSSParserMode);
RefPtr<CSSValue> consumeViewTransitionTypes(CSSParserTokenRange&);

RefPtr<CSSValue> consumeDeclarationValue(CSSParserTokenRange&, const CSSParserContext&);

// @font-face descriptor consumers:

RefPtr<CSSValue> consumeFontFaceFontFamily(CSSParserTokenRange&);
Vector<FontTechnology> consumeFontTech(CSSParserTokenRange&, bool singleValue = false);
String consumeFontFormat(CSSParserTokenRange&, bool rejectStringValues = false);

// @font-palette-values descriptor consumers:

RefPtr<CSSValue> consumeFontPaletteValuesOverrideColors(CSSParserTokenRange&, const CSSParserContext&);

// @counter-style descriptor consumers:

RefPtr<CSSValue> consumeCounterStyleSystem(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleSymbol(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleNegative(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleRange(CSSParserTokenRange&);
RefPtr<CSSValue> consumeCounterStylePad(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleSymbols(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleAdditiveSymbols(CSSParserTokenRange&, const CSSParserContext&);
RefPtr<CSSValue> consumeCounterStyleSpeakAs(CSSParserTokenRange&);

// Template and inline implementations are at the bottom of the file for readability.

inline bool isFontStyleAngleInRange(double angleInDegrees)
{
    return angleInDegrees >= -90 && angleInDegrees <= 90;
}

inline bool isSystemFontShorthand(CSSValueID valueID)
{
    // This needs to stay in sync with SystemFontDatabase::FontShorthand.
    static_assert(CSSValueStatusBar - CSSValueCaption == static_cast<SystemFontDatabase::FontShorthandUnderlyingType>(SystemFontDatabase::FontShorthand::StatusBar));
    return valueID >= CSSValueCaption && valueID <= CSSValueStatusBar;
}

inline SystemFontDatabase::FontShorthand lowerFontShorthand(CSSValueID valueID)
{
    // This needs to stay in sync with SystemFontDatabase::FontShorthand.
    ASSERT(isSystemFontShorthand(valueID));
    return static_cast<SystemFontDatabase::FontShorthand>(valueID - CSSValueCaption);
}

} // namespace CSSPropertyParserHelpers

} // namespace WebCore
