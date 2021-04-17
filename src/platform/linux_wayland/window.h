#pragma once

#include <wayland-client.h>

namespace platform
{

class Window // : public windowIf
{
public:
    void init();
private:
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_compositor* compositor = nullptr;
    wl_shell* shell = nullptr;
    wl_seat* seat = nullptr;
    wl_pointer* pointer = nullptr;
    wl_keyboard* keyboard = nullptr;
    wl_surface* surface = nullptr;
    wl_shell_surface* shell_surface = nullptr;

    wl_event_queue* queue = nullptr;
    // bool quit = false;
};

