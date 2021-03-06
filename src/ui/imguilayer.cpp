#include "ui/imguilayer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"

#include <fmt/printf.h>

namespace ui
{
UiLayer::UiLayer(entt::dispatcher& disp) : _conn(disp, this)
{
    _conn.attach<event::UiCameraUpdate>();
}

void UiLayer::begin()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    dockspace();
    setDebugTab();
}

void UiLayer::end()
{
    ImGui::Render();
}

void UiLayer::setDebugTab()
{
    const auto& cameraPos = _events.cameraUpdate.cameraPos;
    const auto& lookdir = _events.cameraUpdate.lookDir;

    const auto& fov = _events.cameraUpdate.fov;

    ImGui::Begin("Camera");
    ImGui::Text("Field of view (%.2f) degrees", fov);
    ImGui::Text(
            "Position (%.2f, %.2f, %.2f)",
            cameraPos.x,
            cameraPos.y,
            cameraPos.z);
    ImGui::Text(
            "Direction (%.2f, %.2f, %.2f)",
            lookdir.x,
            lookdir.y,
            lookdir.z);
    ImGui::End();
}

void UiLayer::dockspace()
{
    bool p_open = true;

    {
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags =
                ImGuiDockNodeFlags_None
                | ImGuiDockNodeFlags_PassthruCentralNode;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the
        // parent window not dockable into, because it would be
        // confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags =
                ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if(opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar
                            | ImGuiWindowFlags_NoResize
                            | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus
                            | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode,
        // DockSpace() will render our background and handle the
        // pass-thru hole, so we ask Begin() to not render a background.
        if(dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false
        // (aka window is collapsed). This is because we want to keep
        // our DockSpace() active. If a DockSpace() is inactive, all
        // active windows docked into it will lose their parent and
        // become undocked. We cannot preserve the docking relationship
        // between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being
        // stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("###DockSpace", &p_open, window_flags);
        ImGui::PopStyleVar();

        if(opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        else
        {
            // ShowDockingDisabledMessage();
        }

        ImGui::End();
    }
}

auto UiLayer::openFileButton(std::string_view caption) const -> std::string
{
    bool b = ImGui::Button(caption.data());
    if(!b)
        return "";

    std::string filename;
    char buffer[1024];
    FILE* file =
            popen("zenity --title=\"Select a obj model file to load\" "
                  "--file-filter=\"OBJ files | *.obj\" "
                  "--file-selection",
                  "r");
    if(file)
    {
        while(fgets(buffer, sizeof(buffer), file))
        {
            filename += buffer;
        };
        filename.erase(
                std::remove(filename.begin(), filename.end(), '\n'),
                filename.end());

        pclose(file);
    }

    return filename;
}

void UiLayer::onEvent(event::UiCameraUpdate const& event)
{
    _events.cameraUpdate = event;
}

} // namespace ui
