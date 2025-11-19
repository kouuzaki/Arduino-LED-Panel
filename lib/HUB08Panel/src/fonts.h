/**
 * @file fonts.h
 * @brief Type-safe font namespace for HUB08Panel library
 *
 * This header provides all available custom fonts through a namespace for clean,
 * auto-complete friendly access. Fonts are stored in PROGMEM to save RAM.
 *
 * Available fonts via HUB08Fonts namespace:
 *   - HUB08Fonts::Roboto_6       (Roboto 6pt - tiny)
 *   - HUB08Fonts::Roboto_Bold_12 (Roboto Bold 12pt - small-medium)
 *   - HUB08Fonts::Roboto_Bold_13 (Roboto Bold 13pt - medium)
 *   - HUB08Fonts::Roboto_Bold_14 (Roboto Bold 14pt - medium-large)
 *   - HUB08Fonts::Roboto_Bold_15 (Roboto Bold 15pt - large, professional)
 *
 * Usage example:
 *   #include "HUB08Panel.h"
 *   #include "fonts.h"
 *
 *   display.setFont(&HUB08Fonts::Roboto_Bold_15);
 */

#ifndef HUB08_FONTS_H
#define HUB08_FONTS_H

#include <Adafruit_GFX.h>

// Include actual font definitions from fonts/ subdirectory (one level up)
#include "../fonts/Roboto_6.h"
#include "../fonts/Roboto_Bold_12.h"
#include "../fonts/Roboto_Bold_13.h"
#include "../fonts/Roboto_Bold_14.h"
#include "../fonts/Roboto_Bold_15.h"

/**
 * @namespace HUB08Fonts
 * @brief Type-safe font namespace providing IDE auto-complete
 *
 * Provides clean, professional access to all available fonts:
 *   display.setFont(&HUB08Fonts::Roboto_Bold_15);
 */
namespace HUB08Fonts
{
    /// Roboto 6pt font - Tiny, compact text for displays
    constexpr const GFXfont *const Roboto_6 = &::Roboto_6;

    /// Roboto Bold 12pt font - Small-medium text
    constexpr const GFXfont *const Roboto_Bold_12 = &::Roboto_Bold_12;

    /// Roboto Bold 13pt font - Medium text
    constexpr const GFXfont *const Roboto_Bold_13 = &::Roboto_Bold_13;

    /// Roboto Bold 14pt font - Medium-large text
    constexpr const GFXfont *const Roboto_Bold_14 = &::Roboto_Bold_14;

    /// Roboto Bold 15pt font - Large, professional appearance
    constexpr const GFXfont *const Roboto_Bold_15 = &::Roboto_Bold_15;
} // namespace HUB08Fonts

#endif // HUB08_FONTS_H
