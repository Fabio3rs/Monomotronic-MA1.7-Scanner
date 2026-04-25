#pragma once

#include "../utils/Layout.h"
#include "imgui.h"
#include <array>

namespace UI::TableDefinitions {

struct ColumnDef {
    const char *label;
    ImGuiTableColumnFlags flags;
    float width;
};

inline void SetupColumn(const ColumnDef &column) {
    ImGui::TableSetupColumn(column.label, column.flags, column.width);
}

template <std::size_t N>
inline void SetupColumns(const std::array<ColumnDef, N> &columns) {
    for (const auto &column : columns) {
        SetupColumn(column);
    }
}

inline constexpr std::array<ColumnDef, 5> kLiveSensorColumns = {
    ColumnDef{"Status", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::LIVE_STATUS_COLUMN_WIDTH},
    ColumnDef{"Nome", ImGuiTableColumnFlags_WidthStretch, 0.0f},
    ColumnDef{"Valor", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::LIVE_VALUE_COLUMN_WIDTH},
    ColumnDef{"Fixar", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::LIVE_PIN_COLUMN_WIDTH},
    ColumnDef{"Tend\xC3\xAAncia", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::LIVE_TREND_COLUMN_WIDTH},
};

inline constexpr std::array<ColumnDef, 3> kSensorTableBaseColumns = {
    ColumnDef{"Status", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::LIVE_STATUS_COLUMN_WIDTH},
    ColumnDef{"Nome", ImGuiTableColumnFlags_WidthStretch, 0.0f},
    ColumnDef{"Valor", ImGuiTableColumnFlags_WidthFixed,
              Layout::Table::SENSOR_VALUE_COLUMN_WIDTH},
};

inline constexpr ColumnDef kSensorTableTrendColumn = {
    "Trend", ImGuiTableColumnFlags_WidthFixed,
    Layout::Table::SENSOR_TREND_COLUMN_WIDTH};
inline constexpr ColumnDef kSensorTablePinColumn = {
    "Fixar", ImGuiTableColumnFlags_WidthFixed,
    Layout::Table::SENSOR_PIN_COLUMN_WIDTH};

inline int GetSensorTableColumnCount(bool show_sparklines,
                                     bool show_pin_buttons) {
    int count = static_cast<int>(kSensorTableBaseColumns.size());
    if (show_sparklines) {
        count += 1;
    }
    if (show_pin_buttons) {
        count += 1;
    }
    return count;
}

} // namespace UI::TableDefinitions
