#pragma once

#include "../ui/DashboardWidget.h"
#include "../ui/Modal.h"
#include "BaseScreen.h"
#include <memory>
#include <vector>

enum class DashWidgetType { NUMERIC, GAUGE, GRAPH, BARGRAPH };

struct DashWidget {
    int sensor_idx;
    DashWidgetType type;
    std::unique_ptr<DashboardWidgetRenderer> renderer;

    DashWidget(int idx, DashWidgetType t);
};

class DashScreen : public BaseScreen {
  public:
    DashScreen();
    ~DashScreen() override = default;

    // BaseScreen interface
    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

  private:
    // UI state
    bool show_add_widget_modal_ = false;
    bool show_context_menu_ = false;
    bool context_menu_popup_open_ =
        false; // Track if context menu popup is actually open
    bool show_remove_confirmation_ = false;
    bool show_reset_confirmation_ = false;
    int context_menu_widget_idx_ = -1;
    int pending_remove_widget_idx_ = -1;

    // Widgets
    std::vector<DashWidget> widgets_;

    // Temporary state for add widget modal
    int temp_selected_sensor_ = -1;
    DashWidgetType temp_selected_type_ = DashWidgetType::NUMERIC;

    // Components
    Modal add_widget_modal_;
    Modal remove_confirm_modal_;
    Modal reset_confirm_modal_;

    // Helper methods
    void RenderTopControls();
    void RenderWidgetGrid();
    void RenderAddWidgetModal();
    void RenderContextMenu();
    void RenderRemoveConfirmation();
    void RenderResetConfirmation();

    void AddWidget(int sensor_idx, DashWidgetType type);
    void RemoveWidget(int widget_idx);
    void ChangeWidgetType(int widget_idx, DashWidgetType new_type);

    // Auto-population helpers
    void AutoPopulateDashboard();
    DashWidgetType InferWidgetTypeForSensor(int sensor_idx);

    void SaveWidgetsToState();
    void LoadWidgetsFromState();
    void UpdateSubscriptionsFromWidgets();
};
