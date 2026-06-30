#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

struct SensorCatalogEntry {
    const char *key;
    const char *display_name;
    const char *unit;
    uint8_t id;
    uint8_t subcommand;
    uint8_t length;
    uint8_t collection_table;
    float (*decode)(int raw);
};

constexpr size_t kCollectionSlots = 10;

using KlineEntry = std::pair<uint8_t, uint8_t>;

extern const std::array<KlineEntry, kCollectionSlots> kCollectionTable1;
extern const std::array<KlineEntry, kCollectionSlots> kCollectionTable2;

const SensorCatalogEntry *GetSensorCatalog();
size_t GetSensorCatalogCount();
const SensorCatalogEntry *FindSensorCatalogEntry(uint8_t subcommand,
                                                 uint8_t id);
const std::array<KlineEntry, kCollectionSlots> *
GetCollectionTable(uint8_t table_id);
