#pragma once

#include "../ui/Modal.h"
#include "BaseScreen.h"
#include <string>
#include <vector>

struct DTCEntry {
    std::string code;
    std::string description;
    bool is_active;
};

class DTCScreen : public BaseScreen {
  public:
    DTCScreen();
    ~DTCScreen() override = default;

    // BaseScreen interface
    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

  private:
    // UI state
    bool show_clear_confirmation_ = false;
    int current_page_ = 0;

    // DTC data
    std::vector<DTCEntry> dtc_list_;

    // Per.10: Cache for performance optimization
    std::vector<DTCEntry> cached_page_dtcs_;
    int cached_page_index_ = -1;

    // Timestamp of last successful read
    std::string last_read_time_;

    // Components
    Modal clear_confirm_modal_;

    // Constants
    static constexpr int DTCS_PER_PAGE = 10;

    // Helper methods
    void RenderTopControls();
    void RenderDTCTable();
    void RenderPagination();
    void RenderClearConfirmation();

    void ReadDTCs();
    void ClearDTCs();

    int GetTotalPages() const;
    const std::vector<DTCEntry> &
    GetCurrentPageDTCs(); // Per.10: Returns cached reference
};
