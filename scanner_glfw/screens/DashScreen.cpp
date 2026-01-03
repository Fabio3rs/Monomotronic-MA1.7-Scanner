#include "DashScreen.h"
#include "../app_data.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../core/TouchGestureHandler.h"
#include "../ui/DashboardWidget.h"
#include "../utils/Colors.h"
#include "../utils/Layout.h"
#include <algorithm>

using namespace Layout::Button; // Use button constants

extern ImFont *font_large;

DashWidget::DashWidget(int idx, DashWidgetType t) : sensor_idx(idx), type(t) {
    // Create appropriate renderer based on type
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

    // Auto-populate on first launch (no saved widgets)
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
    // Update fade alpha for smooth transitions
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("dash_screen", 0.2f));
}

void DashScreen::Render() {
    // Apply fade
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, GetFadeAlpha());

    // Content area (cursor already at (0,0) within ContentArea parent)
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("DashScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderTopControls();

    // Calculate remaining height for widget grid
    const float controls_height = ImGui::GetCursorPosY();
    const float grid_height =
        content_height - controls_height - Layout::SPACING_SMALL;

    // Render widget grid
    ImGui::BeginChild("WidgetGrid", ImVec2(0, grid_height), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderWidgetGrid();
    ImGui::EndChild();

    ImGui::EndChild();

    // Render modals
    RenderAddWidgetModal();
    RenderContextMenu();
    RenderRemoveConfirmation();
    RenderResetConfirmation();

    ImGui::PopStyleVar();
}

bool DashScreen::HandleGesture(const GestureEvent &event) {
    // Handle swipe gestures for screen navigation
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

    // Add Widget button
    if (ImGui::Button("\uF067 Add Widget",
                      ImVec2(150, PRIMARY_HEIGHT))) { // FA plus
        show_add_widget_modal_ = true;
        // OpenPopup will be called in RenderAddWidgetModal() in the correct
        // scope

        // Reset temp selection
        temp_selected_sensor_ = -1;
        temp_selected_type_ = DashWidgetType::NUMERIC;
    }

    ImGui::SameLine(0.0f, Layout::Button::SPACING);

    // Reset button
    if (ImGui::Button("\uF0E2 Reset", ImVec2(WIDE_WIDTH, PRIMARY_HEIGHT))) {
        show_reset_confirmation_ = true;
    }

    ImGui::SameLine();

    // Info text
    ImGui::TextColored(theme.GetSecondaryColor(),
                       "Long-press widget for options");
}

void DashScreen::RenderWidgetGrid() {
    auto &theme = ThemeManager::Instance();

    // Empty state
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

    // ES.10: Responsive grid layout
    const float grid_width = ImGui::GetContentRegionAvail().x;
    const float grid_height = ImGui::GetContentRegionAvail().y;

    constexpr float grid_spacing = 12.0f;
    constexpr float min_widget_width = 240.0f;
    constexpr float min_widget_height =
        110.0f; // Reduced from 150px (sparkline now at 52%, not 95%)
    constexpr float max_widget_height =
        150.0f; // Prevent stretching beyond this

    // Calculate responsive number of columns
    int num_cols = 2; // Default
    if (grid_width < (min_widget_width * 2 + grid_spacing)) {
        num_cols = 1; // Narrow screen: 1 column
    } else if (grid_width >= (min_widget_width * 3 + grid_spacing * 2)) {
        num_cols = 3; // Wide screen: 3 columns
    }

    const float widget_width =
        (grid_width - grid_spacing * (num_cols - 1)) / num_cols;
    const int num_rows =
        (static_cast<int>(widgets_.size()) + num_cols - 1) / num_cols;
    const float widget_height =
        (grid_height - grid_spacing * (num_rows - 1)) / num_rows;

    // ES.10: Apply minimum and maximum size constraints consistently for
    // positioning
    const float effective_width = std::max(widget_width, min_widget_width);
    const float effective_height =
        std::min(std::max(widget_height, min_widget_height), max_widget_height);
    const ImVec2 widget_size(effective_width, effective_height);

    auto &gesture_handler = TouchGestureHandler::Instance();

    // Calculate total grid height (needed for scrolling)
    const float total_grid_height =
        num_rows * (effective_height + grid_spacing) - grid_spacing;

    // Create scrollable container if content exceeds viewport
    const bool needs_scroll = total_grid_height > grid_height;
    if (needs_scroll) {
        ImGui::BeginChild("DashScrollContainer",
                          ImVec2(grid_width, grid_height),
                          false,                  // No border
                          ImGuiWindowFlags_None); // Allow vertical scroll
    }

    for (size_t i = 0; i < widgets_.size(); ++i) {
        auto &widget = widgets_[i];

        // Calculate responsive grid position using EFFECTIVE size (after min
        // constraints)
        const int row = static_cast<int>(i) / num_cols;
        const int col = static_cast<int>(i) % num_cols;

        const float pos_x = col * (effective_width + grid_spacing);
        const float pos_y = row * (effective_height + grid_spacing);

        ImGui::SetCursorPos(ImVec2(pos_x, pos_y));

        // Widget container
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginChild("WidgetContainer", widget_size, true,
                          ImGuiWindowFlags_NoScrollbar);

        // Check for valid sensor index
        if (widget.sensor_idx >= 0 &&
            widget.sensor_idx < static_cast<int>(simulatedSensors.size())) {
            const auto &sensor = simulatedSensors[widget.sensor_idx];

            // Render widget using appropriate renderer
            if (widget.renderer) {
                widget.renderer->Render(sensor, widget_size);
            }

            // ES.10: Context menu icon (gear) - top-right corner with correct
            // positioning
            const ImVec2 widget_content_min =
                ImGui::GetWindowContentRegionMin();
            const ImVec2 widget_content_max =
                ImGui::GetWindowContentRegionMax();
            const ImVec2 window_pos = ImGui::GetWindowPos();

            constexpr float gear_margin = 10.0f;

            const ImVec2 gear_pos = ImVec2(
                window_pos.x + widget_content_max.x - ICON_SIZE - gear_margin,
                window_pos.y + widget_content_min.y + gear_margin);

            ImGui::SetCursorScreenPos(gear_pos);
            if (ImGui::Button("\uF013",
                              ImVec2(ICON_SIZE, ICON_SIZE))) { // FA cog
                context_menu_widget_idx_ = static_cast<int>(i);
                show_context_menu_ = true;
            }
        }

        ImGui::EndChild();

        // Detect long-press on widget for context menu
        if (ImGui::IsItemHovered() &&
            gesture_handler.IsGesture(GestureType::LONG_PRESS)) {
            context_menu_widget_idx_ = static_cast<int>(i);
            show_context_menu_ = true;
        }

        ImGui::PopID();
    }

    // End scrollable container if it was created
    if (needs_scroll) {
        ImGui::EndChild();
    }
}

void DashScreen::RenderAddWidgetModal() {
    if (!show_add_widget_modal_) {
        return;
    }

    // Call OpenPopup if modal should be shown but isn't open yet
    // This ensures OpenPopup is called in the correct scope (outside child
    // windows)
    if (!add_widget_modal_.IsOpen()) {
        add_widget_modal_.Open();
    }

    bool confirmed = false;
    add_widget_modal_.RenderWithButtons(
        [this]() {
            auto &theme = ThemeManager::Instance();

            ImGui::Text("Select sensor:");
            ImGui::Spacing();

            // Sensor selection (dropdown)
            const char *preview =
                temp_selected_sensor_ >= 0 &&
                        temp_selected_sensor_ <
                            static_cast<int>(simulatedSensors.size())
                    ? simulatedSensors[temp_selected_sensor_].name.c_str()
                    : "Choose a sensor...";

            if (ImGui::BeginCombo("##sensor", preview)) {
                for (size_t i = 0; i < simulatedSensors.size(); ++i) {
                    const bool is_selected =
                        (temp_selected_sensor_ == static_cast<int>(i));
                    if (ImGui::Selectable(simulatedSensors[i].name.c_str(),
                                          is_selected)) {
                        temp_selected_sensor_ = static_cast<int>(i);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Select widget type:");
            ImGui::Spacing();

            // Widget type selection (radio buttons)
            if (ImGui::RadioButton("Numeric", temp_selected_type_ ==
                                                  DashWidgetType::NUMERIC)) {
                temp_selected_type_ = DashWidgetType::NUMERIC;
            }
            ImGui::SameLine();
            ImGui::TextColored(theme.GetSecondaryColor(), "Value + Progress");

            ImGui::Spacing();

            if (ImGui::RadioButton("Gauge", temp_selected_type_ ==
                                                DashWidgetType::GAUGE)) {
                temp_selected_type_ = DashWidgetType::GAUGE;
            }
            ImGui::SameLine();
            ImGui::TextColored(theme.GetSecondaryColor(), "Arc Dial");

            ImGui::Spacing();

            if (ImGui::RadioButton("Graph", temp_selected_type_ ==
                                                DashWidgetType::GRAPH)) {
                temp_selected_type_ = DashWidgetType::GRAPH;
            }
            ImGui::SameLine();
            ImGui::TextColored(theme.GetSecondaryColor(), "Line Chart");

            ImGui::Spacing();

            if (ImGui::RadioButton("BarGraph", temp_selected_type_ ==
                                                   DashWidgetType::BARGRAPH)) {
                temp_selected_type_ = DashWidgetType::BARGRAPH;
            }
            ImGui::SameLine();
            ImGui::TextColored(theme.GetSecondaryColor(), "Retro 80s/90s Bars");
        },
        &confirmed);

    if (confirmed) {
        // Validate selection
        constexpr size_t MAX_WIDGETS = 12; // Smaller widgets allow more items
        if (temp_selected_sensor_ >= 0 && widgets_.size() < MAX_WIDGETS) {
            AddWidget(temp_selected_sensor_, temp_selected_type_);
        }
        show_add_widget_modal_ = false;
    }

    if (!add_widget_modal_.IsOpen()) {
        show_add_widget_modal_ = false;
    }
}

void DashScreen::RenderContextMenu() {
    if (!show_context_menu_) {
        return;
    }

    // Guard check: only open popup if not already open
    if (!context_menu_popup_open_) {
        ImGui::OpenPopup("WidgetContextMenu");
        context_menu_popup_open_ = true;
    }

    if (ImGui::BeginPopup("WidgetContextMenu")) {
        // Detect click outside popup to close it
        if (ImGui::IsMouseClicked(0) &&
            !ImGui::IsWindowHovered(
                ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        ImGui::Text("Widget Options");
        ImGui::Separator();

        if (ImGui::MenuItem("Change to Numeric")) {
            ChangeWidgetType(context_menu_widget_idx_, DashWidgetType::NUMERIC);
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        if (ImGui::MenuItem("Change to Gauge")) {
            ChangeWidgetType(context_menu_widget_idx_, DashWidgetType::GAUGE);
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        if (ImGui::MenuItem("Change to Graph")) {
            ChangeWidgetType(context_menu_widget_idx_, DashWidgetType::GRAPH);
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        if (ImGui::MenuItem("Change to BarGraph")) {
            ChangeWidgetType(context_menu_widget_idx_,
                             DashWidgetType::BARGRAPH);
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Remove Widget")) {
            pending_remove_widget_idx_ = context_menu_widget_idx_;
            show_remove_confirmation_ = true;
            remove_confirm_modal_.Open();
            ImGui::CloseCurrentPopup();
            context_menu_popup_open_ = false;
            show_context_menu_ = false;
        }

        ImGui::EndPopup();
    } else {
        // Popup was closed by ImGui (ESC or other reason)
        show_context_menu_ = false;
        context_menu_popup_open_ = false;
    }
}

void DashScreen::RenderRemoveConfirmation() {
    if (!show_remove_confirmation_) {
        return;
    }

    // Call OpenPopup if modal should be shown but isn't open yet
    // This ensures OpenPopup is called in the correct scope (outside other
    // popups)
    if (!remove_confirm_modal_.IsOpen()) {
        remove_confirm_modal_.Open();
    }

    bool confirmed = false;
    remove_confirm_modal_.RenderWithButtons(
        [this]() {
            auto &theme = ThemeManager::Instance();

            if (pending_remove_widget_idx_ >= 0 &&
                pending_remove_widget_idx_ <
                    static_cast<int>(widgets_.size())) {
                const auto &widget = widgets_[pending_remove_widget_idx_];

                if (widget.sensor_idx >= 0 &&
                    widget.sensor_idx <
                        static_cast<int>(simulatedSensors.size())) {
                    const auto &sensor = simulatedSensors[widget.sensor_idx];

                    ImGui::Text("Remove widget for sensor:");
                    ImGui::Spacing();
                    ImGui::PushFont(font_large);
                    ImGui::TextColored(theme.GetAccentColor(), "%s",
                                       sensor.name.c_str());
                    ImGui::PopFont();
                    ImGui::Spacing();
                    ImGui::TextColored(theme.GetSecondaryColor(),
                                       "This action cannot be undone.");
                }
            }
        },
        &confirmed);

    if (confirmed) {
        RemoveWidget(pending_remove_widget_idx_);
        pending_remove_widget_idx_ = -1;
        show_remove_confirmation_ = false;
    }

    if (!remove_confirm_modal_.IsOpen()) {
        show_remove_confirmation_ = false;
        pending_remove_widget_idx_ = -1;
    }
}

void DashScreen::RenderResetConfirmation() {
    if (!show_reset_confirmation_) {
        return;
    }

    // Open modal if not already open
    if (!reset_confirm_modal_.IsOpen()) {
        reset_confirm_modal_.Open();
    }

    bool confirmed = false;
    reset_confirm_modal_.RenderWithButtons(
        [this]() {
            auto &theme = ThemeManager::Instance();

            ImGui::Text("Reset dashboard to default layout?");
            ImGui::Spacing();

            ImGui::TextColored(
                theme.GetSecondaryColor(),
                "This will remove all custom widgets and restore\n"
                "the essential sensors:");

            ImGui::Spacing();
            ImGui::BulletText("Water Temperature");
            ImGui::BulletText("RPM");
            ImGui::BulletText("Throttle Position (TPS)");
            ImGui::BulletText("Air Temperature");

            ImGui::Spacing();
            ImGui::TextColored(Colors::Status::WARN,
                               "This action cannot be undone.");
        },
        &confirmed);

    if (confirmed) {
        // CRITICAL: Clear existing widgets FIRST
        widgets_.clear();

        // Then populate with defaults
        AutoPopulateDashboard();

        // Close modal
        show_reset_confirmation_ = false;
    }

    // Cleanup if modal was closed without confirmation
    if (!reset_confirm_modal_.IsOpen()) {
        show_reset_confirmation_ = false;
    }
}

void DashScreen::AddWidget(int sensor_idx, DashWidgetType type) {
    // Limit to 12 widgets (up to 6 rows × 2 cols with scrolling)
    // With max_widget_height=150px, this allows comfortable viewing on 1024x600
    // screen
    constexpr size_t MAX_WIDGETS = 12;
    if (widgets_.size() >= MAX_WIDGETS) {
        return;
    }

    widgets_.emplace_back(sensor_idx, type);
    SaveWidgetsToState();
    UpdateSubscriptionsFromWidgets();
}

void DashScreen::RemoveWidget(int widget_idx) {
    if (widget_idx < 0 || widget_idx >= static_cast<int>(widgets_.size())) {
        return;
    }

    // BUG FIX #8: Use erase with iterator (safe for vector)
    widgets_.erase(widgets_.begin() + widget_idx);

    // Reset pending index if needed
    if (pending_remove_widget_idx_ == widget_idx) {
        pending_remove_widget_idx_ = -1;
    }

    SaveWidgetsToState();
    UpdateSubscriptionsFromWidgets();
}

void DashScreen::ChangeWidgetType(int widget_idx, DashWidgetType new_type) {
    if (widget_idx < 0 || widget_idx >= static_cast<int>(widgets_.size())) {
        return;
    }

    auto &widget = widgets_[widget_idx];

    // Create new renderer
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

    // Try to get active sensor collection from ECU
    auto &backend = ECUBackend::Instance();
    if (backend.IsConnected()) {
        auto ecu_sensors = backend.GetActiveSensorCollection();
        if (ecu_sensors.has_value()) {
            // Map ECU sensor IDs to simulatedSensors indices
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

    // Fallback: default collection table (10 sensors)
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

    // Add widgets with smart type inference
    for (int sensor_idx : sensor_indices_to_add) {
        DashWidgetType widget_type = InferWidgetTypeForSensor(sensor_idx);
        AddWidget(sensor_idx, widget_type);
    }

    // Persist to StateManager
    SaveWidgetsToState();
}

DashWidgetType DashScreen::InferWidgetTypeForSensor(int sensor_idx) {
    if (sensor_idx < 0 ||
        sensor_idx >= static_cast<int>(simulatedSensors.size())) {
        return DashWidgetType::NUMERIC; // Safe default
    }

    const auto &sensor = simulatedSensors[sensor_idx];
    const std::string &name = sensor.name;

    // Gauge: Rotational/percentage values (RPM, TPS, throttle, actuators)
    if (name.find("RPM") != std::string::npos ||
        name.find("TPS") != std::string::npos ||
        name.find("Throttle") != std::string::npos ||
        name.find("Actuator") != std::string::npos) {
        return DashWidgetType::GAUGE;
    }

    // Graph: Values that benefit from trend analysis
    if (name.find("Lambda") != std::string::npos ||
        name.find("MAP") != std::string::npos ||
        name.find("Injection") != std::string::npos ||
        name.find("adaptation") != std::string::npos ||
        name.find("integrator") != std::string::npos) {
        return DashWidgetType::GRAPH;
    }

    // Numeric: Temperature, voltage, default
    return DashWidgetType::NUMERIC;
}

void DashScreen::SaveWidgetsToState() {
    auto &state = StateManager::Instance();
    if (!state.IsInitialized()) {
        return;
    }

    // Convert local DashWidget to app_data.h DashboardWidget for persistence
    std::vector<::DashboardWidget> state_widgets;
    for (const auto &widget : widgets_) {
        // Map DashWidgetType to WidgetType
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
            // Map WidgetType to DashWidgetType
            DashWidgetType dwt = DashWidgetType::NUMERIC; // default

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
