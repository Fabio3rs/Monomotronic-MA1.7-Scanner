/*
Local optional adapter for Arduino/ESP32 builds.

MIT License

Copyright (c) 2019 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef ESP32_OPTIONAL_H
#define ESP32_OPTIONAL_H

#include <cassert>
#include <new>
#include <type_traits>
#include <utility>

#if !defined(ESP32_OPTIONAL_FORCE_FALLBACK) && defined(__has_include)
#if __has_include(<optional>)
#include <optional>
#if defined(__cpp_lib_optional) && (__cpp_lib_optional >= 201606L)
#define ESP32_OPTIONAL_USE_STD 1
#endif
#endif
#endif

#ifdef ESP32_OPTIONAL_USE_STD

using nulloptional = std::nullopt_t;
constexpr nulloptional nullopt = std::nullopt;

template <class T> using optional = std::optional<T>;

#else

struct nulloptional {
    explicit constexpr nulloptional(int) {}
};

constexpr nulloptional nullopt{0};

template <class T> class optional {
    static_assert(!std::is_reference<T>::value,
                  "optional<T&> is not supported");

    alignas(T) unsigned char storage_[sizeof(T)];
    bool hasv_{false};

    T *ptr() { return reinterpret_cast<T *>(storage_); }
    const T *ptr() const { return reinterpret_cast<const T *>(storage_); }

    template <class... Args> void construct(Args &&...args) {
        new (storage_) T(std::forward<Args>(args)...);
        hasv_ = true;
    }

  public:
    optional() = default;
    optional(const nulloptional &) {}

    optional(const T &v) { construct(v); }
    optional(T &&v) { construct(std::move(v)); }

    optional(const optional &other) {
        if (other.hasv_) {
            construct(*other.ptr());
        }
    }

    optional(optional &&other) {
        if (other.hasv_) {
            construct(std::move(*other.ptr()));
        }
    }

    ~optional() { reset(); }

    optional &operator=(const nulloptional &) {
        reset();
        return *this;
    }

    optional &operator=(const optional &other) {
        if (this == &other) {
            return *this;
        }

        if (other.hasv_) {
            if (hasv_) {
                *ptr() = *other.ptr();
            } else {
                construct(*other.ptr());
            }
        } else {
            reset();
        }

        return *this;
    }

    optional &operator=(optional &&other) {
        if (this == &other) {
            return *this;
        }

        if (other.hasv_) {
            if (hasv_) {
                *ptr() = std::move(*other.ptr());
            } else {
                construct(std::move(*other.ptr()));
            }
        } else {
            reset();
        }

        return *this;
    }

    optional &operator=(const T &v) {
        if (hasv_) {
            *ptr() = v;
        } else {
            construct(v);
        }
        return *this;
    }

    optional &operator=(T &&v) {
        if (hasv_) {
            *ptr() = std::move(v);
        } else {
            construct(std::move(v));
        }
        return *this;
    }

    bool has_value() const { return hasv_; }
    explicit operator bool() const { return hasv_; }

    T &value() {
        assert(hasv_);
        return *ptr();
    }

    const T &value() const {
        assert(hasv_);
        return *ptr();
    }

    T &operator*() { return value(); }
    const T &operator*() const { return value(); }

    T *operator->() { return &value(); }
    const T *operator->() const { return &value(); }

    template <class... Args> T &emplace(Args &&...args) {
        reset();
        construct(std::forward<Args>(args)...);
        return *ptr();
    }

    template <class U> T value_or(U &&default_value) const {
        if (hasv_) {
            return *ptr();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    void reset() {
        if (hasv_) {
            ptr()->~T();
            hasv_ = false;
        }
    }
};

#endif

#endif
