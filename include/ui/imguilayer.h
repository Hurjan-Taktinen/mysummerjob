#pragma once

#include "event/sub.h"
#include "event/updateevents.h"

// #include "entt/dispatcher.h"

#include <string>
#include <string_view>


namespace ui
{

class UiLayer
{
public:
    UiLayer(entt::dispatcher& disp);
    // All per-frame ImGui update commands must come between begin and end
    auto begin() -> void;
    auto end() -> void;
    auto setDebugTab() -> void;
    auto dockspace() -> void;
    auto openFileButton(std::string_view caption) const -> std::string;

    void onEvent(event::UiCameraUpdate const& event);

private:
    event::Subs<UiLayer> _conn;

    struct Events {
        event::UiCameraUpdate cameraUpdate;

    }_events;
};

} // namespace ui
