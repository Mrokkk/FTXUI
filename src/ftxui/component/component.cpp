// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <algorithm>  // for find_if
#include <cassert>    // for assert
#include <cstddef>    // for size_t
#include <iterator>   // for begin, end
#include <memory>     // for unique_ptr, make_unique
#include <utility>    // for move
#include <vector>     // for vector, __alloc_traits<>::value_type

#include "ftxui/component/captured_mouse.hpp"  // for CapturedMouse, CapturedMouseInterface
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"  // for ComponentBase, Components
#include "ftxui/component/event.hpp"           // for Event
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"                  // for text, Element
#include "ftxui/dom/node.hpp"                      // for Node, Elements
#include "ftxui/screen/box.hpp"                    // for Box

namespace ftxui::animation {
class Params;
}  // namespace ftxui::animation

namespace ftxui {

namespace {
class CaptureMouseImpl : public CapturedMouseInterface {};
}  // namespace

ComponentBase::~ComponentBase() {
  DetachAllChildren();
}

/// @brief Return the parent ComponentBase, or nul if any.
/// @see Detach
/// @see Parent
ComponentBase* ComponentBase::Parent() const {
  return parent_;
}

/// @brief Access the child at index `i`.
Component& ComponentBase::ChildAt(size_t i) {
  assert(i < ChildCount());  // NOLINT
  return children_[i];
}

/// @brief Returns the number of children.
size_t ComponentBase::ChildCount() const {
  return children_.size();
}

/// @brief Return index of the component in its parent. -1 if no parent.
int ComponentBase::Index() const {
  if (parent_ == nullptr) {
    return -1;
  }
  int index = 0;
  for (const Component& child : parent_->children_) {
    if (child.get() == this) {
      return index;
    }
    index++;
  }
  return -1;  // Not reached.
}

/// @brief Add a child.
/// @@param child The child to be attached.
void ComponentBase::Add(Component child) {
  child->Detach();
  child->parent_ = this;
  children_.push_back(std::move(child));
}

/// @brief Detach this child from its parent.
/// @see Detach
/// @see Parent
void ComponentBase::Detach() {
  if (parent_ == nullptr) {
    return;
  }
  auto it = std::find_if(std::begin(parent_->children_),  //
                         std::end(parent_->children_),    //
                         [this](const Component& that) {  //
                           return this == that.get();
                         });
  ComponentBase* parent = parent_;
  parent_ = nullptr;
  parent->children_.erase(it);  // Might delete |this|.
}

/// @brief Remove all children.
void ComponentBase::DetachAllChildren() {
  while (!children_.empty()) {
    children_[0]->Detach();
  }
}

/// @brief Draw the component.
/// Build a ftxui::Element to be drawn on the ftxui::Screen representing this
/// ftxui::ComponentBase. Please override OnRender() to modify the rendering.
Element ComponentBase::Render() {
  // Some users might call `ComponentBase::Render()` from
  // `T::OnRender()`. To avoid infinite recursion, we use a flag.
  if (in_render) {
    return ComponentBase::OnRender();
  }

  in_render = true;
  Element element = OnRender();
  in_render = false;

  class Wrapper : public Node {
   public:
    bool active_ = false;

    Wrapper(Element child, bool active)
        : Node({std::move(child)}), active_(active) {}

    void SetBox(Box box) override {
      Node::SetBox(box);
      children_[0]->SetBox(box);
    }

    void ComputeRequirement() override {
      Node::ComputeRequirement();
      requirement_.focused.component_active = active_;
    }
  };

  return std::make_shared<Wrapper>(std::move(element), Active());
}

/// @brief Draw the component.
/// Build a ftxui::Element to be drawn on the ftxi::Screen representing this
/// ftxui::ComponentBase. This function is means to be overridden.
Element ComponentBase::OnRender() {
  if (children_.size() == 1) {
    return children_.front()->Render();
  }

  return text("Not implemented component");
}

/// @brief Called in response to an event.
/// @param event The event.
/// @return True when the event has been handled.
/// The default implementation called OnEvent on every child until one return
/// true. If none returns true, return false.
bool ComponentBase::OnEvent(const Event& event) {  // NOLINT
  for (Component& child : children_) {      // NOLINT
    if (child->OnEvent(event)) {
      return true;
    }
  }
  return false;
}

/// @brief Called in response to an animation event.
/// @param params the parameters of the animation
/// The default implementation dispatch the event to every child.
void ComponentBase::OnAnimation(animation::Params& params) {
  for (const Component& child : children_) {
    child->OnAnimation(params);
  }
}

/// @brief Return the currently Active child.
/// @return the currently Active child.
Component ComponentBase::ActiveChild() {
  for (auto& child : children_) {
    if (child->Focusable()) {
      return child;
    }
  }
  return nullptr;
}

/// @brief Return true when the component contains focusable elements.
/// The non focusable Components will be skipped when navigating using the
/// keyboard.
bool ComponentBase::Focusable() const {
  for (const Component& child : children_) {  // NOLINT
    if (child->Focusable()) {
      return true;
    }
  }
  return false;
}

/// @brief Returns if the element if the currently active child of its parent.
bool ComponentBase::Active() const {
  return parent_ == nullptr || parent_->ActiveChild().get() == this;
}

/// @brief Returns if the elements if focused by the user.
/// True when the ComponentBase is focused by the user. An element is Focused
/// when it is with all its ancestors the ActiveChild() of their parents, and it
/// Focusable().
bool ComponentBase::Focused() const {
  const auto* current = this;
  while (current && current->Active()) {
    current = current->parent_;
  }
  return !current && Focusable();
}

/// @brief Make the |child| to be the "active" one.
/// @param child the child to become active.
void ComponentBase::SetActiveChild([[maybe_unused]] ComponentBase* child) {}

/// @brief Make the |child| to be the "active" one.
/// @param child the child to become active.
void ComponentBase::SetActiveChild(Component child) {  // NOLINT
  SetActiveChild(child.get());
}

/// @brief Configure all the ancestors to give focus to this component.
void ComponentBase::TakeFocus() {
  ComponentBase* child = this;
  while (ComponentBase* parent = child->parent_) {
    parent->SetActiveChild(child);
    child = parent;
  }
}

/// @brief Take the CapturedMouse if available. There is only one component of
/// them. It represents a component taking priority over others.
/// @param event The event
CapturedMouse ComponentBase::CaptureMouse(const Event& event) {  // NOLINT
  if (event.screen_) {
    return event.screen_->CaptureMouse();
  }
  return std::make_unique<CaptureMouseImpl>();
}

}  // namespace ftxui
