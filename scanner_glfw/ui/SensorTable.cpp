#include "SensorTable.h"
#include "../core/AnimationSystem.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/Layout.h"
#include "../utils/StringUtils.h" // ES.5: Case-insensitive string utilities
#include "TableDefinitions.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

extern ImFont *font_large;

int SensorTable::Render(const std::vector<int> &sensor_indices,
                        const std::vector<SensorState> &all_sensors,
                        float table_height) {
    auto &theme = ThemeManager::Instance();
    int detail_sensor_idx = -1;

    // Table flags
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const int num_columns = UI::TableDefinitions::GetSensorTableColumnCount(
        show_sparklines_, show_pin_buttons_);

    if (ImGui::BeginTable("SensorTable", num_columns, flags,
                          ImVec2(0, table_height))) {
        UI::TableDefinitions::SetupColumns(
            UI::TableDefinitions::kSensorTableBaseColumns);
        if (show_sparklines_) {
            UI::TableDefinitions::SetupColumn(
                UI::TableDefinitions::kSensorTableTrendColumn);
        }
        if (show_pin_buttons_) {
            UI::TableDefinitions::SetupColumn(
                UI::TableDefinitions::kSensorTablePinColumn);
        }

        ImGui::TableSetupScrollFreeze(0, 1); // Freeze header row
        ImGui::TableHeadersRow();

        // Render rows
        int visible_count = 0;

        for (const int sensor_idx : sensor_indices) {
            if (sensor_idx < 0 ||
                sensor_idx >= static_cast<int>(all_sensors.size())) {
                continue;
            }

            const auto &sensor = all_sensors[sensor_idx];

            // Filter check
            if (!MatchesFilter(sensor)) {
                continue;
            }

            visible_count++;

            ImGui::TableNextRow(ImGuiTableRowFlags_None,
                                Layout::SENSOR_ROW_HEIGHT);

            // Column 0: Status circle
            ImGui::TableNextColumn();
            RenderStatusCircle(sensor.status);

            // Column 1: Sensor name (clickable for details)
            ImGui::TableNextColumn();

            // Check for critical status pulsing background
            if (sensor.status == SensorStatus::CRITICAL) {
                auto &anim = AnimationSystem::Instance();
                const float pulse = anim.GetPulse(2.0f); // 2 Hz
                const float alpha = 0.3f + 0.2f * pulse; // 0.3 - 0.5

                ImVec4 bg_color = Colors::Status::CRITICAL;
                bg_color.w = alpha;

                ImGui::TableSetBgColor(
                    ImGuiTableBgTarget_CellBg,
                    ImGui::ColorConvertFloat4ToU32(bg_color));
            }

            // Make entire row selectable for detail view
            char selectable_id[64];
            snprintf(selectable_id, sizeof(selectable_id), "##sensor_%d",
                     sensor_idx);

            if (ImGui::Selectable(selectable_id, false,
                                  ImGuiSelectableFlags_SpanAllColumns |
                                      ImGuiSelectableFlags_AllowItemOverlap,
                                  ImVec2(0, Layout::SENSOR_ROW_HEIGHT))) {
                detail_sensor_idx = sensor_idx;
            }

            ImGui::SameLine();
            ImGui::Text("%s", sensor.name.c_str());
            ImGui::SameLine();
            const bool is_collection =
                sensor.pollMode == SensorPollMode::COLLECTION;
            const ImVec4 badge_color =
                is_collection ? ThemeManager::Instance().GetSecondaryColor()
                              : Colors::Status::STALE;
            ImGui::TextColored(badge_color, is_collection ? "[COL]" : "[IND]");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(
                    is_collection
                        ? "Coletado via requestSensorCollection (frame F4)"
                        : "Lido individualmente via readECUMemory");
            }

            // Column 2: Value + Unit
            ImGui::TableNextColumn();

            char value_str[128];
            if (sensor.status == SensorStatus::STALE) {
                snprintf(value_str, sizeof(value_str), "--- %s",
                         sensor.unit.c_str());
                ImGui::TextColored(Colors::Status::STALE, "%s", value_str);
            } else {
                snprintf(value_str, sizeof(value_str), "%.1f %s",
                         sensor.lastValue, sensor.unit.c_str());

                ImVec4 value_color = theme.GetTextColor();

                // Check sensor status first
                if (sensor.status == SensorStatus::WARN)
                    value_color = Colors::Status::WARN;
                else if (sensor.status == SensorStatus::CRITICAL)
                    value_color = Colors::Status::CRITICAL;

                // Additional heuristic: highlight obviously abnormal values
                // (negative temps, extreme RPM, etc.)
                else if ((sensor.unit == "\u00B0C" || sensor.unit == "C") &&
                         sensor.lastValue < -50.0f) {
                    value_color = Colors::Status::WARN; // Abnormally low temp
                } else if (sensor.unit == "rpm" && sensor.lastValue > 8000.0f) {
                    value_color = Colors::Status::CRITICAL; // Extreme RPM
                }

                ImGui::TextColored(value_color, "%s", value_str);
            }

            // Column 3: Sparkline (optional)
            if (show_sparklines_) {
                ImGui::TableNextColumn();
                RenderSparkline(sensor, Layout::Table::SENSOR_SPARKLINE_WIDTH,
                                Layout::Table::SENSOR_SPARKLINE_HEIGHT);
            }

            // Column 4: Pin button (optional)
            if (show_pin_buttons_) {
                ImGui::TableNextColumn();

                const bool is_pinned = is_pinned_callback_
                                           ? is_pinned_callback_(sensor_idx)
                                           : false;

                // Pin/Unpin button with icons
                const char *pin_icon =
                    is_pinned ? "\xF0\x9F\x93\x8C" : "\xE2\x97\x8B"; // 📌 or ○
                char pin_btn_id[64];
                snprintf(pin_btn_id, sizeof(pin_btn_id), "%s##pin_%d", pin_icon,
                         sensor_idx);

                if (ImGui::Button(pin_btn_id,
                                  ImVec2(Layout::Button::PIN_TABLE_SIZE,
                                         Layout::Button::PIN_TABLE_SIZE))) {
                    if (pin_callback_) {
                        pin_callback_(sensor_idx, !is_pinned);
                    }
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(is_pinned ? "Desafixar"
                                                : "Fixar no dashboard");
                }
            }
        }

        // Show "no results" message if nothing visible
        if (visible_count == 0) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableSetColumnIndex(1);

            ImGui::TextColored(theme.GetSecondaryColor(),
                               "Nenhum sensor encontrado");
        }

        ImGui::EndTable();
    }

    return detail_sensor_idx;
}

bool SensorTable::MatchesFilter(const SensorState &sensor) const {
    if (filter_text_.empty()) {
        return true;
    }

    // ES.5: Case-insensitive search in name and description
    return StringUtils::ContainsIgnoreCase(sensor.name, filter_text_) ||
           StringUtils::ContainsIgnoreCase(sensor.description, filter_text_);
}

void SensorTable::RenderStatusCircle(SensorStatus status) {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const ImVec2 circle_center = ImVec2(
        cursor_pos.x + 30.0f,                           // Center in column
        cursor_pos.y + Layout::SENSOR_ROW_HEIGHT * 0.5f // Center vertically
    );

    ImVec4 status_color;
    switch (status) {
    case SensorStatus::OK:
        status_color = Colors::Status::OK;
        break;
    case SensorStatus::WARN:
        status_color = Colors::Status::WARN;
        break;
    case SensorStatus::CRITICAL:
        status_color = Colors::Status::CRITICAL;
        break;
    case SensorStatus::STALE:
        status_color = Colors::Status::STALE;
        break;
    }

    draw_list->AddCircleFilled(circle_center,
                               Layout::SENSOR_STATUS_CIRCLE_RADIUS,
                               ImGui::ColorConvertFloat4ToU32(status_color));
}

void SensorTable::RenderSparkline(const SensorState &sensor, float width,
                                  float height) {
    if (sensor.history.empty()) {
        return;
    }

    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const ImVec2 sparkline_min = cursor_pos;
    const ImVec2 sparkline_max =
        ImVec2(cursor_pos.x + width, cursor_pos.y + height);

    // Find min/max for Y-axis scaling (prefer display range, then alert range)
    float min_val = 0.0f;
    float max_val = 0.0f;
    if (!std::isnan(sensor.displayMin) && !std::isnan(sensor.displayMax)) {
        min_val = static_cast<float>(sensor.displayMin);
        max_val = static_cast<float>(sensor.displayMax);
    } else if (!std::isnan(sensor.alertMin) && !std::isnan(sensor.alertMax)) {
        min_val = static_cast<float>(sensor.alertMin);
        max_val = static_cast<float>(sensor.alertMax);
    } else {
        const auto [min_it, max_it] =
            std::minmax_element(sensor.history.begin(), sensor.history.end());
        min_val = *min_it;
        max_val = *max_it;
    }

    const float range = max_val - min_val;

    // ES.102: Prevent division by zero (range and history.size()-1)
    if (range > 0.001f && sensor.history.size() >= 2) {
        // Draw sparkline
        for (size_t i = 1; i < sensor.history.size(); ++i) {
            const float x0 =
                sparkline_min.x + (i - 1) * width / (sensor.history.size() - 1);
            const float x1 =
                sparkline_min.x + i * width / (sensor.history.size() - 1);

            const float y0 =
                sparkline_max.y -
                ((sensor.history[i - 1] - min_val) / range) * height;
            const float y1 = sparkline_max.y -
                             ((sensor.history[i] - min_val) / range) * height;

            draw_list->AddLine(
                ImVec2(x0, y0), ImVec2(x1, y1),
                ImGui::ColorConvertFloat4ToU32(theme.GetPrimaryColor()), 1.5f);
        }
    }

    ImGui::Dummy(ImVec2(width, height));
}
