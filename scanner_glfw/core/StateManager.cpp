#include "StateManager.h"
#include "../app_data.h" // For DashboardWidget, WidgetType
#include <cstdlib>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

StateManager::~StateManager() { Shutdown(); }

std::string StateManager::GetDefaultDBPath() {
    // Use ~/.config/ecu_monitor/settings.db
    const char *home = getenv("HOME");
    if (!home) {
        return "./ecu_monitor_settings.db"; // Fallback to local directory
    }

    std::string config_dir = std::string(home) + "/.config/ecu_monitor";

    // Create directory if it doesn't exist
    struct stat st;
    if (stat(config_dir.c_str(), &st) != 0) {
        mkdir(config_dir.c_str(), 0755);
    }

    return config_dir + "/settings.db";
}

bool StateManager::Initialize(const std::string &db_path) {
    if (db_ != nullptr) {
        return true; // Already initialized
    }

    db_path_ = db_path.empty() ? GetDefaultDBPath() : db_path;

    const int result = sqlite3_open(db_path_.c_str(), &db_);
    if (result != SQLITE_OK) {
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    return CreateTables();
}

void StateManager::Shutdown() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool StateManager::CreateTables() {
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS custom_sensors (
            position INTEGER PRIMARY KEY,
            sensor_index INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS dashboard_widgets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            position INTEGER NOT NULL,
            sensor_index INTEGER NOT NULL,
            widget_type TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS pinned_sensors (
            sensor_index INTEGER PRIMARY KEY
        );

        CREATE TABLE IF NOT EXISTS graph_sensors (
            position INTEGER PRIMARY KEY,
            sensor_index INTEGER NOT NULL
        );
    )";

    return ExecuteSQL(sql);
}

bool StateManager::ExecuteSQL(const std::string &sql) {
    if (!db_)
        return false;

    char *err_msg = nullptr;
    const int result =
        sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (result != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return false;
    }

    return true;
}

// Settings (key-value store)

void StateManager::SetSetting(const std::string &key,
                              const std::string &value) {
    if (!db_)
        return;

    const char *sql =
        "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), static_cast<int>(key.size()),
                      SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), static_cast<int>(value.size()),
                      SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string StateManager::GetSetting(const std::string &key,
                                     const std::string &default_val) {
    if (!db_)
        return default_val;

    const char *sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return default_val;
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), static_cast<int>(key.size()),
                      SQLITE_STATIC);

    std::string result = default_val;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *value =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (value) {
            result = value;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

int StateManager::GetSettingInt(const std::string &key, int default_val) {
    const std::string value = GetSetting(key, "");
    if (value.empty())
        return default_val;
    try {
        return std::stoi(value);
    } catch (...) {
        return default_val;
    }
}

float StateManager::GetSettingFloat(const std::string &key, float default_val) {
    const std::string value = GetSetting(key, "");
    if (value.empty())
        return default_val;
    try {
        return std::stof(value);
    } catch (...) {
        return default_val;
    }
}

// Generic helper for clearing a table
static bool ClearTable(sqlite3 *db, const char *table_name) {
    if (!db || !table_name)
        return false;
    std::string sql = "DELETE FROM ";
    sql += table_name;
    sql += ";";
    char *err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (err)
        sqlite3_free(err);
    return rc == SQLITE_OK;
}

// Custom sensors (LIVE screen)

void StateManager::SaveCustomSensors(const std::vector<int> &sensor_indices) {
    if (!db_)
        return;

    ClearTable(db_, "custom_sensors");

    const char *sql =
        "INSERT INTO custom_sensors (position, sensor_index) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    for (size_t i = 0; i < sensor_indices.size(); ++i) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, static_cast<int>(i));
        sqlite3_bind_int(stmt, 2, sensor_indices[i]);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

std::vector<int> StateManager::LoadCustomSensors() {
    std::vector<int> result;
    if (!db_)
        return result;

    const char *sql =
        "SELECT sensor_index FROM custom_sensors ORDER BY position;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int sensor_idx = sqlite3_column_int(stmt, 0);
        result.push_back(sensor_idx);
    }

    sqlite3_finalize(stmt);
    return result;
}

// Dashboard widgets

void StateManager::SaveDashboardWidgets(
    const std::vector<DashboardWidget> &widgets) {
    if (!db_)
        return;

    ClearTable(db_, "dashboard_widgets");

    const char *sql = "INSERT INTO dashboard_widgets (position, sensor_index, "
                      "widget_type) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    for (size_t i = 0; i < widgets.size(); ++i) {
        std::string widget_type_str;
        switch (widgets[i].type) {
        case WidgetType::NUMERIC:
            widget_type_str = "numeric";
            break;
        case WidgetType::GAUGE:
            widget_type_str = "gauge";
            break;
        case WidgetType::GRAPH:
            widget_type_str = "graph";
            break;
        case WidgetType::BARGRAPH:
            widget_type_str = "bargraph";
            break;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, static_cast<int>(i));
        sqlite3_bind_int(stmt, 2, widgets[i].sensor_idx);
        sqlite3_bind_text(stmt, 3, widget_type_str.c_str(),
                          static_cast<int>(widget_type_str.size()),
                          SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

std::vector<DashboardWidget> StateManager::LoadDashboardWidgets() {
    std::vector<DashboardWidget> result;
    if (!db_)
        return result;

    const char *sql = "SELECT sensor_index, widget_type FROM dashboard_widgets "
                      "ORDER BY position;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int sensor_idx = sqlite3_column_int(stmt, 0);
        const char *type_str =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        WidgetType widget_type = WidgetType::NUMERIC;
        if (type_str) {
            const std::string_view type_s(type_str);
            if (type_s == "gauge")
                widget_type = WidgetType::GAUGE;
            else if (type_s == "graph")
                widget_type = WidgetType::GRAPH;
            else if (type_s == "bargraph")
                widget_type = WidgetType::BARGRAPH;
        }

        result.emplace_back(sensor_idx, widget_type);
    }

    sqlite3_finalize(stmt);
    return result;
}

// Pinned sensors

void StateManager::SavePinnedSensors(const std::vector<int> &sensor_indices) {
    if (!db_)
        return;

    ClearTable(db_, "pinned_sensors");

    const char *sql = "INSERT INTO pinned_sensors (sensor_index) VALUES (?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    for (const int idx : sensor_indices) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, idx);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

std::vector<int> StateManager::LoadPinnedSensors() {
    std::vector<int> result;
    if (!db_)
        return result;

    const char *sql = "SELECT sensor_index FROM pinned_sensors;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int sensor_idx = sqlite3_column_int(stmt, 0);
        result.push_back(sensor_idx);
    }

    sqlite3_finalize(stmt);
    return result;
}

// Graph sensors

void StateManager::SaveGraphSensors(const std::vector<int> &sensor_indices) {
    if (!db_)
        return;

    ClearTable(db_, "graph_sensors");

    const char *sql =
        "INSERT INTO graph_sensors (position, sensor_index) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }

    for (size_t i = 0; i < sensor_indices.size(); ++i) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, static_cast<int>(i));
        sqlite3_bind_int(stmt, 2, sensor_indices[i]);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

std::vector<int> StateManager::LoadGraphSensors() {
    std::vector<int> result;
    if (!db_)
        return result;

    const char *sql =
        "SELECT sensor_index FROM graph_sensors ORDER BY position;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int sensor_idx = sqlite3_column_int(stmt, 0);
        result.push_back(sensor_idx);
    }

    sqlite3_finalize(stmt);
    return result;
}
