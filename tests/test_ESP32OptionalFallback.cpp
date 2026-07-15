#define ESP32_OPTIONAL_FORCE_FALLBACK 1
#include "../ESP32/optional.h"
#include <gtest/gtest.h>
#include <optional>
#include <type_traits>
#include <utility>

namespace {
struct NoDefault {
    explicit NoDefault(int value_in) : value(value_in) {}
    int value;
};

struct LifecycleProbe {
    static int destructor_calls;

    explicit LifecycleProbe(int id_in) : id(id_in) {}
    LifecycleProbe(const LifecycleProbe &other) : id(other.id) {}
    LifecycleProbe(LifecycleProbe &&other) : id(other.id) { other.id = -1; }

    LifecycleProbe &operator=(const LifecycleProbe &other) {
        id = other.id;
        return *this;
    }

    LifecycleProbe &operator=(LifecycleProbe &&other) {
        id = other.id;
        other.id = -1;
        return *this;
    }

    ~LifecycleProbe() { ++destructor_calls; }

    int id;
};

int LifecycleProbe::destructor_calls = 0;
} // namespace

TEST(ESP32OptionalFallbackTest, SupportsNonDefaultConstructibleTypes) {
    optional<NoDefault> value;
    EXPECT_FALSE(value.has_value());

    value.emplace(42);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->value, 42);
}

TEST(ESP32OptionalFallbackTest, ResetDestroysContainedValue) {
    LifecycleProbe::destructor_calls = 0;

    {
        optional<LifecycleProbe> value;
        value.emplace(7);
        EXPECT_EQ(LifecycleProbe::destructor_calls, 0);

        value.reset();
        EXPECT_EQ(LifecycleProbe::destructor_calls, 1);
        EXPECT_FALSE(value.has_value());
    }

    EXPECT_EQ(LifecycleProbe::destructor_calls, 1);
}

TEST(ESP32OptionalFallbackTest, AssignmentAndValueOrPreserveState) {
    optional<int> value;
    EXPECT_EQ(value.value_or(99), 99);

    value = 10;
    ASSERT_TRUE(value);
    EXPECT_EQ(*value, 10);
    EXPECT_EQ(value.value_or(99), 10);

    value = nullopt;
    EXPECT_FALSE(value.has_value());
}

TEST(ESP32OptionalFallbackTest, CopyAndMoveWorkForEngagedAndEmptyValues) {
    optional<int> source(15);
    optional<int> copy(source);
    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(copy.value(), 15);

    optional<int> moved(std::move(source));
    ASSERT_TRUE(moved.has_value());
    EXPECT_EQ(moved.value(), 15);

    optional<int> empty;
    copy = empty;
    EXPECT_FALSE(copy.has_value());
}

TEST(ESP32OptionalFallbackTest, ExposesExpectedCustomType) {
    EXPECT_FALSE((std::is_same<optional<int>, std::optional<int>>::value));
}
