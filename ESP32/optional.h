/*
Local optional implementation for Arduino/ESP32 builds.

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

#include <utility>

class nulloptional {};

static const nulloptional nullopt{};

template <class T> class optional {
    T val_{};
    bool hasv_{false};

  public:
    optional() = default;
    optional(const nulloptional &) : hasv_(false) {}
    optional(const T &v) : val_(v), hasv_(true) {}
    optional(T &&v) : val_(std::move(v)), hasv_(true) {}
    optional(const optional &) = default;
    optional(optional &&) = default;
    optional &operator=(const optional &) = default;
    optional &operator=(optional &&) = default;

    optional &operator=(const T &v) {
        val_ = v;
        hasv_ = true;
        return *this;
    }

    optional &operator=(T &&v) {
        val_ = std::move(v);
        hasv_ = true;
        return *this;
    }

    bool has_value() const { return hasv_; }
    explicit operator bool() const { return hasv_; }

    T &value() { return val_; }
    const T &value() const { return val_; }

    void reset() { hasv_ = false; }
};

#endif
