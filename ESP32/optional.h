/*
Implementation of std::optional by Fabio3rs to use in ESP32

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
#ifndef ESP32_OPTIONAL
#define ESP32_OPTIONAL

namespace std {
class nullopt_t {
    bool a;

  public:
};

template <class T> class optional {
    T val;
    bool hasv;

  public:
    T &value() const { return val; };
    bool has_value() const { return hasv; };

    operator bool() const { return hasv; }

    optional &operator=(const T &v) {
        val = v;
        hasv = true;

        return *this;
    }

    optional &operator=(const T &&v) {
        val = std::move(v);
        hasv = true;

        return *this;
    }

    optional(T &&v) : hasv(true), val(std::move(v)) {}
    optional(const T &v) : hasv(true), val(v) {}
    optional() : hasv(false) {}
};
} // namespace std

#endif
