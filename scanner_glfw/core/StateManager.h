#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

// Forward declarations to avoid circular dependencies
enum class WidgetType;
struct DashboardWidget;

class StateManager {
  public:
    // Singleton access
    static StateManager &Instance() {
        static StateManager instance;
        return instance;
    }

    // Initialization / Shutdown
    bool Initialize(const std::string &db_path = "");
    void Shutdown();
    bool IsInitialized() const { return db_ != nullptr; }

    // Generic settings (key-value store)
    void SetSetting(const std::string &key, const std::string &value);
    std::string GetSetting(const std::string &key,
                           const std::string &default_val = "");
    int GetSettingInt(const std::string &key, int default_val = 0);
    float GetSettingFloat(const std::string &key, float default_val = 0.0f);

    // Custom sensors (LIVE screen)
    void SaveCustomSensors(const std::vector<int> &sensor_indices);
    std::vector<int> LoadCustomSensors();

    // Dashboard widgets
    void SaveDashboardWidgets(const std::vector<DashboardWidget> &widgets);
    std::vector<DashboardWidget> LoadDashboardWidgets();

    // Pinned sensors
    void SavePinnedSensors(const std::vector<int> &sensor_indices);
    std::vector<int> LoadPinnedSensors();

    // Graph sensors
    void SaveGraphSensors(const std::vector<int> &sensor_indices);
    std::vector<int> LoadGraphSensors();

    // Delete copy/move constructors (singleton)
    StateManager(const StateManager &) = delete;
    StateManager &operator=(const StateManager &) = delete;
    StateManager(StateManager &&) = delete;
    StateManager &operator=(StateManager &&) = delete;

  private:
    StateManager() = default;
    ~StateManager();

    sqlite3 *db_ = nullptr;
    std::string db_path_;

    // Helper functions
    bool ExecuteSQL(const std::string &sql);
    bool CreateTables();
    std::string GetDefaultDBPath();
};
