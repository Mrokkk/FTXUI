// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <algorithm>  // for min
#include <memory>     // for make_shared
#include <string>   // for string, wstring
#include <utility>  // for move

#include "ftxui/dom/deprecated.hpp"   // for text, vtext
#include "ftxui/dom/elements.hpp"     // for Element, text, vtext
#include "ftxui/dom/node.hpp"         // for Node
#include "ftxui/dom/requirement.hpp"  // for Requirement
#include "ftxui/dom/selection.hpp"    // for Selection
#include "ftxui/screen/box.hpp"       // for Box
#include "ftxui/screen/colored_string.hpp"
#include "ftxui/screen/screen.hpp"    // for Pixel, Screen
#include "ftxui/screen/string.hpp"  // for string_width, Utf8ToGlyphs, to_string

namespace ftxui {

namespace {
using ftxui::Screen;

class Text : public Node {
 public:
  explicit Text(std::string text) : textOwn_(std::move(text)), textPtr_(&textOwn_) {}
  explicit Text(const std::string* text) : textPtr_(text) {}

  void ComputeRequirement() override {
    requirement_.min_x = string_width(*textPtr_);
    requirement_.min_y = 1;
    has_selection = false;
  }

  void Select(Selection& selection) override {
    if (Box::Intersection(selection.GetBox(), box_).IsEmpty()) {
      return;
    }

    const Selection selection_saturated = selection.SaturateHorizontal(box_);

    has_selection = true;
    selection_start_ = selection_saturated.GetBox().x_min;
    selection_end_ = selection_saturated.GetBox().x_max;

    std::stringstream ss;
    int x = box_.x_min;
    for (const auto& cell : Utf8ToGlyphs(*textPtr_)) {
      if (cell[0] == '\n') {
        continue;
      }
      if (GlyphIsColorSet(cell) || GlyphIsColorReset(cell)) {
          continue;
      }
      if (selection_start_ <= x && x <= selection_end_) {
        ss << cell;
      }
      x++;
    }
    selection.AddPart(ss.str(), box_.y_min, selection_start_, selection_end_);
  }

  void Render(Screen& screen) override {
    int x = box_.x_min;
    const int y = box_.y_min;

    if (y > box_.y_max) {
      return;
    }

    const Color* color = nullptr;

    for (const auto& cell : Utf8ToGlyphs(*textPtr_)) {
      if (x > box_.x_max) {
        break;
      }
      if (cell[0] == '\n') {
        continue;
      }

      if (GlyphIsColorSet(cell)) {
        color = GlyphToColor(cell);
        continue;
      }
      else if (GlyphIsColorReset(cell)) {
        color = nullptr;
        continue;
      }

      PutGlyph(screen, x++, y, cell, color);
    }
  }

 private:
  void PutGlyph(Screen& screen, int x, int y, const std::string& cell, const Color* color) const {
      auto& pixel = screen.PixelAt(x, y);
      pixel.character = cell;
      if (color)
      {
        pixel.foreground_color = *color;
      }

      if (has_selection) {
        auto selectionTransform = screen.GetSelectionStyle();
        if ((x >= selection_start_) && (x <= selection_end_)) {
          selectionTransform(screen.PixelAt(x, y));
        }
      }
  }

  const std::string textOwn_;
  const std::string* textPtr_;
  bool has_selection = false;
  int selection_start_ = 0;
  int selection_end_ = -1;
};

class VText : public Node {
 public:
  explicit VText(std::string text)
      : text_(std::move(text)), width_{std::min(string_width(text_), 1)} {}

  void ComputeRequirement() override {
    requirement_.min_x = width_;
    requirement_.min_y = string_width(text_);
  }

  void Render(Screen& screen) override {
    const int x = box_.x_min;
    int y = box_.y_min;
    if (x + width_ - 1 > box_.x_max) {
      return;
    }
    for (const auto& it : Utf8ToGlyphs(text_)) {
      if (y > box_.y_max) {
        return;
      }
      screen.PixelAt(x, y).character = it;
      y += 1;
    }
  }

 private:
  std::string text_;
  int width_ = 1;
};

}  // namespace

/// @brief Display a piece of UTF8 encoded unicode text.
/// @ingroup dom
/// @see ftxui::to_wstring
///
/// ### Example
///
/// ```cpp
/// Element document = text("Hello world!");
/// ```
///
/// ### Output
///
/// ```bash
/// Hello world!
/// ```
Element text(std::string text) {
  return std::make_shared<Text>(std::move(text));
}

Element text(const std::string* text) {
  return std::make_shared<Text>(text);
}

/// @brief Display a piece of unicode text.
/// @ingroup dom
/// @see ftxui::to_wstring
///
/// ### Example
///
/// ```cpp
/// Element document = text(L"Hello world!");
/// ```
///
/// ### Output
///
/// ```bash
/// Hello world!
/// ```
Element text(std::wstring text) {  // NOLINT
  return std::make_shared<Text>(to_string(text));
}

/// @brief Display a piece of unicode text vertically.
/// @ingroup dom
/// @see ftxui::to_wstring
///
/// ### Example
///
/// ```cpp
/// Element document = vtext("Hello world!");
/// ```
///
/// ### Output
///
/// ```bash
/// H
/// e
/// l
/// l
/// o
///
/// w
/// o
/// r
/// l
/// d
/// !
/// ```
Element vtext(std::string text) {
  return std::make_shared<VText>(std::move(text));
}

/// @brief Display a piece unicode text vertically.
/// @ingroup dom
/// @see ftxui::to_wstring
///
/// ### Example
///
/// ```cpp
/// Element document = vtext(L"Hello world!");
/// ```
///
/// ### Output
///
/// ```bash
/// H
/// e
/// l
/// l
/// o
///
/// w
/// o
/// r
/// l
/// d
/// !
/// ```
Element vtext(std::wstring text) {  // NOLINT
  return std::make_shared<VText>(to_string(text));
}

}  // namespace ftxui

// vim: set expandtab tabstop=2 shiftwidth=2 softtabstop=2 :
