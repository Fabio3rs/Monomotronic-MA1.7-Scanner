#include "DashScreen.h"
#include "../app_data.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../core/TouchGestureHandler.h"
#include "../ui/DashboardWidget.h"
#include "../utils/Colors.h"
#include "../utils/ImGuiRAII.h"
#include "../utils/Layout.h"
#include <algorithm>

using namespace Layout::Button;

extern ImFont *font_large;

DashWidget::DashWidget(int idx, DashWidgetType t) : sensor_idx(idx), type(t) {
    switch (type) {
    case DashWidgetType::NUMERIC:
        renderer = std::make_unique<NumericWidgetRenderer>();
        break;
    case DashWidgetType::GAUGE:
        renderer = std::make_unique<GaugeWidgetRenderer>();
        break;
    case DashWidgetType::GRAPH:
        renderer = std::make_unique<GraphWidgetRenderer>();
        break;
    case DashWidgetType::BARGRAPH:
        renderer = std::make_unique<BarGraphWidgetRenderer>();
        break;
    }
}

DashScreen::DashScreen()
    : add_widget_modal_("Add Widget"), remove_confirm_modal_("Confirm Removal"),
      reset_confirm_modal_("Reset Dashboard") {}

void DashScreen::OnEnter() {
    SetActive(true);
    LoadWidgetsFromState();

    if (widgets_.empty()) {
        AutoPopulateDashboard();
    }

    UpdateSubscriptionsFromWidgets();
}

void DashScreen::OnExit() {
    SetActive(false);
    SaveWidgetsToState();
    ECUBackend::Instance().ClearSubscriptions();
}

void DashScreen::Update(float delta_time) {
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("dash_screen", 0.2f));
}

void DashScreen::Render() {
    UI::StyleVarGuard style;
    style.push(ImGuiStyleVar_Alpha, GetFadeAlpha());

    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("DashScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderTopControls();

    const float controls_height = ImGui::GetCursorPosY();
    const float grid_height =
        content_height - controls_height - Layout::SPACING_SMALL;

    ImGui::BeginChild("WidgetGrid", ImVec2(0, grid_height), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderWidgetGrid();
    ImGui::EndChild();

    ImGui::EndChild();

    RenderAddWidgetModal();
    RenderContextMenu();
    RenderRemoveConfirmation();
    RenderResetConfirmation();
}

bool DashScreen::HandleGesture(const GestureEvent &event) {
    if (event.type == GestureType::SWIPE_RIGHT) {
        SetCurrentScreen(Screen::GRAPH);
        return true;
    }
    if (event.type == GestureType::SWIPE_LEFT) {
        SetCurrentScreen(Screen::DTC);
        return true;
    }
    return false;
}

void DashScreen::RenderTopControls() {
    auto &theme = ThemeManager::Instance();

    if (ImGui::Button("\uF067 Add Widget", ImVec2(150, PRIMARY_HEIGHT))) {
        show_add_widget_modal_ = true;
        temp_selected_sensor_ = -1;
        temp_selected_type_ = DashWidgetType::NUMERIC;
    }

    ImGui::SameLine(0.0f, Layout::Button::SPACING);

    if (ImGui::Button("\uF0E2 Reset", ImVec2(WIDE_WIDTH, PRIMARY_HEIGHT))) {
        show_reset_confirmation_ = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all dashboard widgets");
    }

    ImGui::SameLine();
    ImGui::TextColored(theme.GetSecondaryColor(),
                       "Long-press widget for options");
}

void DashScreen::RenderWidgetGrid() {
    auto &theme = ThemeManager::Instance();

    if (widgets_.empty()) {
        const ImVec2 content_size = ImGui::GetContentRegionAvail();
        const char *empty_msg =
            "No widgets added\nPress '+ Add Widget' to get started";
        const ImVec2 text_size = ImGui::CalcTextSize(empty_msg);

        const ImVec2 text_pos = ImVec2((content_size.x - text_size.x) * 0.5f,
                                       (content_size.y - text_size.y) * 0.5f);

        ImGui::SetCursorPos(text_pos);
        ImGui::TextColored(theme.GetSecondaryColor(), "%s", empty_msg);
        return;
    }

    const float grid_width = ImGui::GetContentRegionAvail().x;
    const float grid_height = ImGui::GetContentRegionAvail().y;

    constexpr float grid_spacing = 12.0f;
    constexpr float min_widget_width = 240.0f;
    constexpr float min_widget_height = 110.0f;
    constexpr float max_widget_height = 150.0f;

    int num_cols = 2;
    if (grid_width < (min_widget_width * 2 + grid_spacing)) {
        num_cols = 1;
    } else if (grid_width >= (min_widget_width * 3 + grid_spacing * 2)) {
        num_cols = 3;
    }

    const float widget_width =
        (grid_width - grid_spacing * (num_cols - 1)) / num_cols;
    const int num_rows =
        (static_cast<int>(widgets_.size()) + num_cols - 1) / num_cols;

    float widget_height =
        (grid_height - grid_spacing * (num_rows - 1)) / num_rows;
    widget_height =
        std::clamp(widget_height, min_widget_height, max_widget_height);

    int widget_idx = 0;
    for (int row = 0;
         row < num_rows && widget_idx < static_cast<int>(widgets_.size());
         ++row) {
        for (int col = 0;
             col < num_cols && widget_idx < static_cast<int>(widgets_.size());
             ++col) {
            if (col > 0) {
                ImGui::SameLine(0, grid_spacing);
            }

            const float x = col * (widget_width + grid_spacing);
            const float y = row * (widget_height + grid_spacing);
            ImGui::SetCursorPos(ImVec2(x, y));

            RenderWidget(widget_idx, widget_width, widget_height);
            ++widget_idx;
        }
    }
}

void DashScreen::RenderWidget(int widget_idx, float width, float height) {
    if (widget_idx < 0 || widget_idx >= static_cast<int>(widgets_.size())) {
        return;
    }

    auto &widget = widgets_[widget_idx];
    if (widget.sensor_idx < 0 ||
        widget.sensor_idx >= static_cast<int>(simulatedSensors.size())) {
        return;
    }

    auto &sensor = simulatedSensors[widget.sensor_idx];

    // Use a child window as the widget container; this gives us natural
    // hit-testing, clipping, and isolates cursor movement from the renderer.
    char child_id[32];
    snprintf(child_id, sizeof(child_id), "##widget_%d", widget_idx);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild(child_id, ImVec2(width, height), false,
                      ImGuiWindowFlags_NoScrollbar);
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            const ImVec2 min_pos = ImGui::GetWindowPos();
            const ImVec2 max_pos =
                ImVec2(min_pos.x + width, min_pos.y + height);
            draw_list->AddRect(min_pos, max_pos,
                               ImGui::ColorConvertFloat4ToU32(
                                   ThemeManager::Instance().GetPrimaryColor()),
                               6.0f, 0, 2.0f);
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1)) {
            show_context_menu_ = true;
            context_menu_widget_idx_ = widget_idx;
        }

        if (widget.renderer) {
            widget.renderer->Render(sensor, ImVec2(width, height));
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void DashScreen::RenderAddWidgetModal() {
    if (!show_add_widget_modal_)
        return;

    bool confirmed = false;
    add_widget_modal_.RenderWithButtons(
        [this]() {
            auto &theme = ThemeManager::Instance();

            ImGui::Text("Select a sensor to add:");
            ImGui::Spacing();

            if (ImGui::BeginCombo(
                    "Sensor",
                    temp_selected_sensor_ >= 0
                        ? simulatedSensors[temp_selected_sensor_].name.c_str()
                        : "Select...")) {
                for (int i = 0; i < static_cast<int>(simulatedSensors.size());
                     ++i) {
                    const bool is_selected = (temp_selected_sensor_ == i);
                    if (ImGui::Selectable(simulatedSensors[i].name.c_str(),
                                          is_selected)) {
                        temp_selected_sensor_ = i;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("Widget Type:");

            const char *type_names[] = {"Numeric", "Gauge", "Graph",
                                        "Bar Graph"};
            const DashWidgetType types[] = {
                DashWidgetType::NUMERIC, DashWidgetType::GAUGE,
                DashWidgetType::GRAPH, DashWidgetType::BARGRAPH};

            for (int i = 0; i < 4; ++i) {
                if (i > 0)
                    ImGui::SameLine();
                bool selected = (temp_selected_type_ == types[i]);
                if (selected) {
                    ImGui::PushStyleColor(ImGuiCol_Button,
                                          theme.GetPrimaryColor());
                }
                if (ImGui::Button(type_names[i], ImVec2(100, 40))) {
                    temp_selected_type_ = types[i];
                }
                if (selected) {
                    ImGui::PopStyleColor();
                }
            }
        },
        &confirmed);

    if (confirmed && temp_selected_sensor_ >= 0) {
        AddWidget(temp_selected_sensor_, temp_selected_type_);
        show_add_widget_modal_ = false;
    }

    if (!add_widget_modal_.IsOpen()) {
        show_add_widget_modal_ = false;
    }
}

void DashScreen::RenderContextMenu() {
    if (!show_context_menu_ || context_menu_widget_idx_ < 0 ||
        context_menu_widget_idx_ >= static_cast<int>(widgets_.size())) {
        return;
    }

    ImGui::OpenPopup("WidgetContextMenu");
    show_context_menu_ = false;
    context_menu_popup_open_ = true;

    if (ImGui::BeginPopup("WidgetContextMenu")) {
        auto &widget = widgets_[context_menu_widget_idx_];

        if (ImGui::MenuItem("Remove Widget")) {
            pending_remove_widget_idx_ = context_menu_widget_idx_;
            show_remove_confirmation_ = true;
        }

        if (ImGui::BeginMenu("Change Type")) {
            const char *type_names[] = {"Numeric", "Gauge", "Graph",
                                        "Bar Graph"};
            const DashWidgetType types[] = {
                DashWidgetType::NUMERIC, DashWidgetType::GAUGE,
                DashWidgetType::GRAPH, DashWidgetType::BARGRAPH};

            for (int i = 0; i < 4; ++i) {
                bool selected = (widget.type == types[i]);
                if (ImGui::MenuItem(type_names[i], nullptr, selected)) {
                    ChangeWidgetType(context_menu_widget_idx_, types[i]);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    } else {
        context_menu_popup_open_ = false;
        context_menu_widget_idx_ = -1;
    }
}

void DashScreen::RenderRemoveConfirmation() {
    if (!show_remove_confirmation_)
        return;

    bool confirmed = false;
    remove_confirm_modal_.RenderWithButtons(
        [this]() {
            auto &theme = ThemeManager::Instance();
            ImGui::Text("Remove this widget?");
            ImGui::Spacing();
            if (pending_remove_widget_idx_ >= 0 &&
                pending_remove_widget_idx_ <
                    static_cast<int>(widgets_.size())) {
                const auto &widget = widgets_[pending_remove_widget_idx_];
                const char *name =
                    (widget.sensor_idx >= 0 &&
                     widget.sensor_idx <
                         static_cast<int>(simulatedSensors.size()))
                        ? simulatedSensors[widget.sensor_idx].name.c_str()
                        : "Unknown";
                ImGui::TextColored(theme.GetSecondaryColor(), "%s", name);
            }
        },
        &confirmed);

    if (confirmed) {
        RemoveWidget(pending_remove_widget_idx_);
        show_remove_confirmation_ = false;
        pending_remove_widget_idx_ = -1;
    }

    if (!remove_confirm_modal_.IsOpen()) {
        show_remove_confirmation_ = false;
        pending_remove_widget_idx_ = -1;
    }
}

void DashScreen::RenderResetConfirmation() {
    if (!show_reset_confirmation_)
        return;

    bool confirmed = false;
    reset_confirm_modal_.RenderWithButtons(
        []() {
            auto &theme = ThemeManager::Instance();
            ImGui::Text("Reset dashboard?");
            ImGui::Spacing();
            ImGui::TextColored(theme.GetSecondaryColor(),
                               "This will remove all widgets.");
        },
        &confirmed);

    if (confirmed) {
        widgets_.clear();
        SaveWidgetsToState();
        show_reset_confirmation_ = false;
    }

    if (!reset_confirm_modal_.IsOpen()) {
        show_reset_confirmation_ = false;
    }
}

void DashScreen::AddWidget(int sensor_idx, DashWidgetType type) {
    widgets_.emplace_back(sensor_idx, type);
    SaveWidgetsToState();
    UpdateSubscriptionsFromWidgets();
}

void DashScreen::RemoveWidget(int widget_idx) {
    if (widget_idx < 0 || widget_idx >= static_cast<int>(widgets_.size())) {
        return;
    }
    widgets_.erase(widgets_.begin() + widget_idx);
    SaveWidgetsToState();
    UpdateSubscriptionsFromWidgets();
}

void DashScreen::ChangeWidgetType(int widget_idx, DashWidgetType new_type) {
    if (widget_idx < 0 || widget_idx >= static_cast<int>(widgets_.size())) {
        return;
    }

    auto &widget = widgets_[widget_idx];
    widget.type = new_type;
    switch (new_type) {
    case DashWidgetType::NUMERIC:
        widget.renderer = std::make_unique<NumericWidgetRenderer>();
        break;
    case DashWidgetType::GAUGE:
        widget.renderer = std::make_unique<GaugeWidgetRenderer>();
        break;
    case DashWidgetType::GRAPH:
        widget.renderer = std::make_unique<GraphWidgetRenderer>();
        break;
    case DashWidgetType::BARGRAPH:
        widget.renderer = std::make_unique<BarGraphWidgetRenderer>();
        break;
    }

    SaveWidgetsToState();
}

void DashScreen::AutoPopulateDashboard() {
    std::vector<int> sensor_indices_to_add;

    auto &backend = ECUBackend::Instance();
    if (backend.IsConnected()) {
        auto ecu_sensors = backend.GetActiveSensorCollection();
        if (ecu_sensors.has_value()) {
            for (const auto &[ecu_id, ecu_subcmd] : ecu_sensors.value()) {
                for (size_t i = 0; i < simulatedSensors.size(); ++i) {
                    if (simulatedSensors[i].id == ecu_id &&
                        simulatedSensors[i].subcmd == ecu_subcmd) {
                        sensor_indices_to_add.push_back(static_cast<int>(i));
                        break;
                    }
                }
            }
        }
    }

    if (sensor_indices_to_add.empty()) {
        constexpr std::array<std::pair<int, int>, 10> kDefaultCollection{{
            {0x63, 0x00},
            {0x66, 0x00},
            {0x47, 0x00},
            {0x62, 0x00},
            {0x0C, 0xF8},
            {0x6C, 0xF8},
            {0x6F, 0xF8},
            {0x3D, 0xF8},
            {0x61, 0x00},
            {0x69, 0xF8},
        }};

        for (const auto &[id, subcmd] : kDefaultCollection) {
            for (size_t i = 0; i < simulatedSensors.size(); ++i) {
                const auto &sensor = simulatedSensors[i];
                if (sensor.id == id && sensor.subcmd == subcmd) {
                    sensor_indices_to_add.push_back(static_cast<int>(i));
                    break;
                }
            }
        }
    }

    for (int sensor_idx : sensor_indices_to_add) {
        DashWidgetType widget_type = InferWidgetTypeForSensor(sensor_idx);
        AddWidget(sensor_idx, widget_type);
    }

    SaveWidgetsToState();
}

DashWidgetType DashScreen::InferWidgetTypeForSensor(int sensor_idx) {
    if (sensor_idx < 0 ||
        sensor_idx >= static_cast<int>(simulatedSensors.size())) {
        return DashWidgetType::NUMERIC;
    }

    const auto &sensor = simulatedSensors[sensor_idx];
    const std::string &name = sensor.name;

    if (name.find("RPM") != std::string::npos ||
        name.find("TPS") != std::string::npos ||
        name.find("Throttle") != std::string::npos ||
        name.find("Actuator") != std::string::npos) {
        return DashWidgetType::GAUGE;
    }

    if (name.find("Lambda") != std::string::npos ||
        name.find("MAP") != std::string::npos ||
        name.find("Injection") != std::string::npos ||
        name.find("adaptation") != std::string::npos ||
        name.find("integrator") != std::string::npos) {
        return DashWidgetType::GRAPH;
    }

    return DashWidgetType::NUMERIC;
}

void DashScreen::SaveWidgetsToState() {
    auto &state = StateManager::Instance();
    if (!state.IsInitialized()) {
        return;
    }

    std::vector<::DashboardWidget> state_widgets;
    for (const auto &widget : widgets_) {
        WidgetType wt;
        switch (widget.type) {
        case DashWidgetType::NUMERIC:
            wt = WidgetType::NUMERIC;
            break;
        case DashWidgetType::GAUGE:
            wt = WidgetType::GAUGE;
            break;
        case DashWidgetType::GRAPH:
            wt = WidgetType::GRAPH;
            break;
        case DashWidgetType::BARGRAPH:
            wt = WidgetType::BARGRAPH;
            break;
        }
        state_widgets.push_back(::DashboardWidget{widget.sensor_idx, wt});
    }

    state.SaveDashboardWidgets(state_widgets);
}

void DashScreen::LoadWidgetsFromState() {
    auto &state = StateManager::Instance();
    if (!state.IsInitialized()) {
        return;
    }

    const auto state_widgets = state.LoadDashboardWidgets();

    widgets_.clear();
    for (const auto &sw : state_widgets) {
        if (sw.sensor_idx >= 0 &&
            sw.sensor_idx < static_cast<int>(simulatedSensors.size())) {
            DashWidgetType dwt = DashWidgetType::NUMERIC;
            if (sw.type == WidgetType::NUMERIC) {
                dwt = DashWidgetType::NUMERIC;
            } else if (sw.type == WidgetType::GAUGE) {
                dwt = DashWidgetType::GAUGE;
            } else if (sw.type == WidgetType::GRAPH) {
                dwt = DashWidgetType::GRAPH;
            } else if (sw.type == WidgetType::BARGRAPH) {
                dwt = DashWidgetType::BARGRAPH;
            }
            widgets_.emplace_back(sw.sensor_idx, dwt);
        }
    }
}

void DashScreen::UpdateSubscriptionsFromWidgets() {
    std::vector<int> active_sensors;
    active_sensors.reserve(widgets_.size());

    for (const auto &widget : widgets_) {
        if (widget.sensor_idx >= 0 &&
            widget.sensor_idx < static_cast<int>(simulatedSensors.size())) {
            active_sensors.push_back(widget.sensor_idx);
        }
    }

    ECUBackend::Instance().SubscribeSensors(active_sensors);
}
