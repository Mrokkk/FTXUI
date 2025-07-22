// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef FTXUI_SCREEN_STRING_HPP
#define FTXUI_SCREEN_STRING_HPP

#include <stdint.h> // for uint8_t
#include <ostream>  // for ostream
#include <string>   // for string, wstring, to_string
#include <vector>   // for vector
#include "color.hpp"

namespace ftxui {

class Color;

std::string to_string(const std::wstring& s);
std::wstring to_wstring(const std::string& s);

template <typename T>
std::wstring to_wstring(T s) {
  return to_wstring(std::to_string(s));
}

enum SpecialMarkers : uint8_t {
  ColorSet = 0xff,
  ColorReset = 0xfe,
};

static inline bool GlyphIsColorSet(const std::string& glyph) {
  return static_cast<uint8_t>(glyph[0]) == SpecialMarkers::ColorSet;
}

static inline bool GlyphIsColorReset(const std::string& glyph) {
  return static_cast<uint8_t>(glyph[0]) == SpecialMarkers::ColorReset;
}

template <typename T>
struct ColorWrappedImpl {
  const T& value;
  Color color;
};

template <typename T>
ColorWrappedImpl<T> ColorWrapped(const T& value, const Color& color) {
  return ColorWrappedImpl<T>{.value = value, .color = color};
}

const Color* GlyphToColor(const std::string& glyph);

template <typename T>
std::ostream& operator<<(std::ostream& os, const ColorWrappedImpl<T>& wrapped)
{
  os.write("\xff", 1);
  os.write(reinterpret_cast<const char*>(&wrapped.color), sizeof(wrapped.color));
  os << wrapped.value;
  os.write("\xfe", 1);
  return os;
}

int string_width(const std::string&);

// Split the string into a its glyphs. An empty one is inserted ater fullwidth
// ones.
std::vector<std::string> Utf8ToGlyphs(const std::string& input);

// Map every cells drawn by |input| to their corresponding Glyphs. Half-size
// Glyphs takes one cell, full-size Glyphs take two cells.
std::vector<int> CellToGlyphIndex(const std::string& input);

}  // namespace ftxui

#endif /* end of include guard: FTXUI_SCREEN_STRING_HPP */

// vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 :
