#ifndef FTXUI_SCREEN_COLORED_STRING_HPP
#define FTXUI_SCREEN_COLORED_STRING_HPP

#include <ostream>  // for ostream
#include "color.hpp"

namespace ftxui {

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
std::ostream& operator<<(std::ostream& os, const ColorWrappedImpl<T>& wrapped) {
  os.write("\xff", 1);
  os.write(reinterpret_cast<const char*>(&wrapped.color), sizeof(wrapped.color));
  os << wrapped.value;
  os.write("\xfe", 1);
  return os;
}

}  // namespace ftxui

#endif /* end of include guard: FTXUI_SCREEN_COLORED_STRING_HPP */
