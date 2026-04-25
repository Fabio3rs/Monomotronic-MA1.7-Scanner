#include "DTCScreen.h"
#include "../app_data.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/MockECUBackend.h" // Fallback when ECU is unavailable
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/FontGuard.h" // C.21: RAII for font management
#include "../utils/ImGuiRAII.h"
#include "../utils/Layout.h"
#include <algorithm>
#include <ctime>

using namespace Layout::Button; // Use button constants

extern ImFont *font_large;

namespace {
// Return a color hint based on the DTC code category (P/C/B/U)
ImVec4 GetDTCCategoryColor(const std::string &code, const ThemeManager &theme) {
    if (code.empty())
        return theme.GetTextColor();
    switch (code[0]) {
    case 'P':
    case 'p':
        return Colors::Status::CRITICAL; // Powertrain – red
    case 'C':
    case 'c':
        return Colors::Status::WARN; // Chassis – orange
    case 'B':
    case 'b':
        return theme.GetAccentColor(); // Body – purple
    case 'U':
    case 'u':
        return theme.GetPrimaryColor(); // Network – blue
    default:
        return theme.GetTextColor();
    }
}

// Format current local time as HH:MM:SS
std::string FormatCurrentTime() {
    const std::time_t now = std::time(nullptr);
    const std::tm *tm = std::localtime(&now);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min,
                  tm->tm_sec);
    return std::string(buf);
}
} // namespace

DTCScreen::DTCScreen() : clear_confirm_modal_("Clear DTCs") {}

void DTCScreen::OnEnter() {
    SetActive(true);

    // If temos conexão real, evite mostrar mocks ao entrar
    if (ECUBackend::Instance().IsConnected()) {
        dtc_list_.clear();
        activeDTCs.clear();
        current_page_ = 0;
        cached_page_index_ = -1;
    }

    // Auto-read DTCs on enter
    // ReadDTCs();  // TODO: Enable when backend integrated
}

void DTCScreen::OnExit() { SetActive(false); }

void DTCScreen::Update(float delta_time) {
    // Update fade alpha for smooth transitions
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("dtc_screen", 0.2f));
}

void DTCScreen::Render() {

    // Apply fade
    UI::StyleVarGuard style;
    style.push(ImGuiStyleVar_Alpha, GetFadeAlpha());

    // Content area (cursor already at (0,0) within ContentArea parent)
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("DTCScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderTopControls();
    RenderClearConfirmation();

    ImGui::Spacing();

    // Calculate remaining height for DTC table + pagination
    const float controls_height = ImGui::GetCursorPosY();
    // Only reserve space for pagination if we have more than 1 page
    const float pagination_height = (GetTotalPages() > 1) ? 80.0f : 0.0f;
    const float spacing = (GetTotalPages() > 1) ? Layout::SPACING_MEDIUM * 2
                                                : Layout::SPACING_MEDIUM;
    const float table_height =
        content_height - controls_height - pagination_height - spacing;

    // Render DTC table
    ImGui::BeginChild("DTCTable", ImVec2(0, table_height), true,
                      ImGuiWindowFlags_NoScrollbar);
    RenderDTCTable();
    ImGui::EndChild();

    ImGui::Spacing();

    // Render pagination
    RenderPagination();

    ImGui::EndChild();
}

bool DTCScreen::HandleGesture(const GestureEvent &event) {
    // Handle swipe gestures for screen navigation
    if (event.type == GestureType::SWIPE_RIGHT) {
        SetCurrentScreen(Screen::DASH);
        return true;
    }

    return false;
}

void DTCScreen::RenderTopControls() {
    auto &theme = ThemeManager::Instance();

    // DTC count badge
    const int active_count = static_cast<int>(
        std::count_if(dtc_list_.begin(), dtc_list_.end(),
                      [](const DTCEntry &dtc) { return dtc.is_active; }));

    // Action buttons on the left
    if (ImGui::Button(
            "\uF021 Read DTCs",
            ImVec2(DTC_ACTION_WIDTH, WIDE_HEIGHT))) { // Font Awesome refresh
        ReadDTCs();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Read diagnostic trouble codes from ECU");
    }

    ImGui::SameLine();

    // Disable clear button if no DTCs
    const bool has_dtcs = !dtc_list_.empty();
    if (!has_dtcs) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button(
            "\uF00D Clear All",
            ImVec2(DTC_ACTION_WIDTH, WIDE_HEIGHT))) { // Font Awesome times
        show_clear_confirmation_ = true;
        clear_confirm_modal_.Open();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all diagnostic trouble codes");
    }

    if (!has_dtcs) {
        ImGui::EndDisabled();
        // UX: explain why disabled
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("No DTCs to clear – read DTCs first");
        }
    }

    // DTC count and timestamp on the right side
    ImGui::SameLine();

    // Build count text
    char count_buffer[32];
    const char *count_text = nullptr;
    if (active_count == 0) {
        count_text = "No Active DTCs";
    } else if (active_count == 1) {
        count_text = "1 Active DTC";
    } else {
        std::snprintf(count_buffer, sizeof(count_buffer), "%d Active DTCs",
                      active_count);
        count_text = count_buffer;
    }

    // C.21: Use FontGuard for automatic PopFont (RAII)
    {
        FontGuard font_guard(font_large);
        if (font_guard.IsValid()) {
            const ImVec2 text_size = ImGui::CalcTextSize(count_text);
            const float content_max_x = ImGui::GetWindowContentRegionMax().x -
                                        ImGui::GetStyle().WindowPadding.x;
            const float pos_x =
                std::max(ImGui::GetCursorPosX(), content_max_x - text_size.x);
            ImGui::SetCursorPosX(pos_x);

            if (active_count > 0) {
                ImGui::TextColored(Colors::Status::CRITICAL, "%s", count_text);
            } else {
                ImGui::TextColored(Colors::Status::OK, "%s", count_text);
            }

            // Timestamp on the next line, aligned right
            if (!last_read_time_.empty()) {
                ImGui::SameLine();
                ImGui::SetCursorPosX(pos_x);
                const char *time_label = last_read_time_.c_str();
                const ImVec2 time_size = ImGui::CalcTextSize(time_label);
                ImGui::SetCursorPosX(content_max_x - time_size.x);
                ImGui::TextColored(theme.GetSecondaryColor(), "Updated: %s",
                                   time_label);
            }
        }
    } // PopFont() called automatically
}

void DTCScreen::RenderDTCTable() {
    auto &theme = ThemeManager::Instance();

    // Empty state
    if (dtc_list_.empty()) {
        const ImVec2 content_size = ImGui::GetContentRegionAvail();
        const char *empty_msg = "No DTCs found\nPress 'Read DTCs' to scan";
        const ImVec2 text_size = ImGui::CalcTextSize(empty_msg);

        const ImVec2 text_pos = ImVec2((content_size.x - text_size.x) * 0.5f,
                                       (content_size.y - text_size.y) * 0.5f);

        ImGui::SetCursorPos(text_pos);
        ImGui::TextColored(theme.GetSecondaryColor(), "%s", empty_msg);
        return;
    }

    // Table flags
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX;

    if (ImGui::BeginTable("DTCTable", 3, flags)) {
        // Setup columns
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,
                                90.0f);
        ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthFixed,
                                110.0f);
        ImGui::TableSetupColumn("Description",
                                ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        // Per.10: Use cached page reference
        const auto &page_dtcs = GetCurrentPageDTCs();

        bool previous_active = true; // Track group transitions
        for (size_t i = 0; i < page_dtcs.size(); ++i) {
            const auto &dtc = page_dtcs[i];

            // Visual separator when switching from active to stored
            if (i > 0 && dtc.is_active != previous_active) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                const ImVec2 min = ImGui::GetCursorScreenPos();
                const ImVec2 max = ImVec2(
                    min.x + ImGui::GetContentRegionAvail().x, min.y + 1.0f);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    min, max,
                    ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()));
                ImGui::Dummy(ImVec2(0, 4.0f));
            }
            previous_active = dtc.is_active;

            ImGui::TableNextRow();

            // Column 0: Status with icon
            ImGui::TableNextColumn();
            if (dtc.is_active) {
                ImGui::TextColored(Colors::Status::CRITICAL, "\uF06A ACTIVE");
            } else {
                ImGui::TextColored(Colors::Status::OK, "\uF017 Stored");
            }

            // Column 1: Code (with category color)
            ImGui::TableNextColumn();
            {
                FontGuard font_guard(font_large);
                if (font_guard.IsValid()) {
                    ImVec4 code_color = GetDTCCategoryColor(dtc.code, theme);
                    ImGui::TextColored(code_color, "%s", dtc.code.c_str());
                }
            } // PopFont() called automatically

            // Column 2: Description
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", dtc.description.c_str());
        }

        ImGui::EndTable();
    }
}

void DTCScreen::RenderPagination() {

    const int total_pages = GetTotalPages();

    if (total_pages <= 1) {
        return; // No pagination needed
    }

    const float content_width = ImGui::GetContentRegionAvail().x;

    // Center pagination controls
    const float controls_width = 400.0f;
    const float start_x = (content_width - controls_width) * 0.5f;

    ImGui::SetCursorPosX(start_x);

    // Previous button (BUG FIX: Icon ◀)
    if (current_page_ == 0) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("\u25C0 Prev",
                      ImVec2(PAGINATION_WIDTH, PAGINATION_HEIGHT))) { // ◀
        if (current_page_ > 0) {
            current_page_--;
        }
    }

    if (current_page_ == 0) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Page indicator
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
    ImGui::Text("Page %d / %d", current_page_ + 1, total_pages);

    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15.0f);

    // Next button (BUG FIX: Icon ▶)
    if (current_page_ >= total_pages - 1) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Next \u25B6",
                      ImVec2(PAGINATION_WIDTH, PAGINATION_HEIGHT))) { // ▶
        if (current_page_ < total_pages - 1) {
            current_page_++;
        }
    }

    if (current_page_ >= total_pages - 1) {
        ImGui::EndDisabled();
    }
}

void DTCScreen::RenderClearConfirmation() {
    if (!show_clear_confirmation_) {
        return;
    }

    bool confirmed = false;
    clear_confirm_modal_.RenderWithButtons(
        []() {
            auto &theme = ThemeManager::Instance();

            ImGui::Text("Clear all DTCs?");
            ImGui::Spacing();
            ImGui::TextColored(
                theme.GetSecondaryColor(),
                "This will erase all stored diagnostic trouble codes.");
            ImGui::Spacing();
            ImGui::TextColored(Colors::Status::WARN,
                               "Warning: DTCs may return if underlying issues "
                               "are not resolved.");
        },
        &confirmed);

    if (confirmed) {
        ClearDTCs();
        show_clear_confirmation_ = false;
    }

    if (!clear_confirm_modal_.IsOpen()) {
        show_clear_confirmation_ = false;
    }
}

void DTCScreen::ReadDTCs() {
    auto &backend = ECUBackend::Instance();
    ECUBackend::DTCResult result{false, {}, "ECU not connected"};
    const bool use_mock = !backend.IsConnected();

    if (!use_mock) {
        result = backend.ReadDTCs();
    }

    if (use_mock) {
        auto &mock = MockECUBackend::Instance();
        const auto mock_result = mock.ReadDTCs();
        result.success = mock_result.success;
        result.dtcs = mock_result.dtcs;
        result.error_message = mock_result.error_message;
    }

    if (result.success) {
        // Convert SimulatedDTC to DTCEntry
        dtc_list_.clear();
        activeDTCs.clear();
        for (const auto &simulated_dtc : result.dtcs) {
            DTCEntry entry;
            entry.code = simulated_dtc.code;
            entry.description = simulated_dtc.description;
            entry.is_active = simulated_dtc.active;
            dtc_list_.push_back(entry);
            activeDTCs.push_back(simulated_dtc);
        }

        // Sort: active first, then by code
        std::sort(dtc_list_.begin(), dtc_list_.end(),
                  [](const DTCEntry &a, const DTCEntry &b) {
                      if (a.is_active != b.is_active)
                          return a.is_active > b.is_active;
                      return a.code < b.code;
                  });

        last_read_time_ = FormatCurrentTime();
        current_page_ = 0;
        cached_page_index_ = -1; // Invalidate cache so table refreshes
    } else {
        // E.2: Log error without crashing
        fprintf(stderr, "ERROR: Failed to read DTCs: %s\n",
                result.error_message.c_str());
        // Keep existing DTCs on error
    }
}

void DTCScreen::ClearDTCs() {
    auto &backend = ECUBackend::Instance();
    ECUBackend::DTCResult result{false, {}, "ECU not connected"};
    const bool use_mock = !backend.IsConnected();

    if (!use_mock) {
        result = backend.ClearDTCs();
    }

    if (use_mock) {
        auto &mock = MockECUBackend::Instance();
        const auto mock_result = mock.ClearDTCs();
        result.success = mock_result.success;
        result.error_message = mock_result.error_message;
    }

    if (result.success) {
        dtc_list_.clear();
        activeDTCs.clear();
        last_read_time_.clear();
        current_page_ = 0;
        cached_page_index_ = -1;
    } else {
        // E.2: Log error without crashing
        fprintf(stderr, "ERROR: Failed to clear DTCs: %s\n",
                result.error_message.c_str());
        // Keep existing DTCs on error
    }
}

int DTCScreen::GetTotalPages() const {
    if (dtc_list_.empty()) {
        return 0;
    }
    return (static_cast<int>(dtc_list_.size()) + DTCS_PER_PAGE - 1) /
           DTCS_PER_PAGE;
}

const std::vector<DTCEntry> &DTCScreen::GetCurrentPageDTCs() {
    // Per.10: Use cache to avoid recreating vector every frame
    if (cached_page_index_ != current_page_) {
        const int start_idx = current_page_ * DTCS_PER_PAGE;
        const int end_idx = std::min(start_idx + DTCS_PER_PAGE,
                                     static_cast<int>(dtc_list_.size()));

        if (start_idx >= static_cast<int>(dtc_list_.size())) {
            cached_page_dtcs_.clear();
        } else {
            cached_page_dtcs_.assign(dtc_list_.begin() + start_idx,
                                     dtc_list_.begin() + end_idx);
        }

        cached_page_index_ = current_page_;
    }

    return cached_page_dtcs_;
}
