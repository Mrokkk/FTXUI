#pragma once

#include <memory>
#include <vector>

namespace ftxui
{

class ComponentBase;
using Component = std::shared_ptr<ComponentBase>;
using Components = std::vector<Component>;

class Node;
using Element = std::shared_ptr<Node>;

}  // namespace ftxui
