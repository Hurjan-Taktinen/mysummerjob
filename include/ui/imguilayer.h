#pragma once

#include <string>
#include <string_view>

namespace ui
{

class UiLayer
{
public:
    // All per-frame ImGui update commands must come between begin and end
    auto begin() -> void;
    auto end() -> void;
    auto dockspace() -> void;
    auto openFileButton(std::string_view caption) const -> std::string;

private:
};

} // namespace ui
