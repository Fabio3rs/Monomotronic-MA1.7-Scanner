# Team C++ Coding Standard (C++20/23)

## 1. Code & Safety

* Write **modern C++20/23** using the **C++ Core Guidelines** as the default reference. ([ISO C++][1])
* Prefer **RAII and strong types** (`std::unique_ptr`, containers, value types); no raw owning `new/delete` in normal code. ([ISO C++][1])
* Apply **NASA Power of 10–style rules**: simple control flow (no `goto` / recursion), bounded loops, no dynamic allocation after initialization in safety-critical paths, short functions, and high assertion density. ([Wikipedia][2])
* Minimize globals and shared state; declare data at the **smallest possible scope**. ([Wikipedia][2])
* All builds use **max warnings + warnings-as-errors** and must be **clean**. Run static analyzers (e.g., clang-tidy with `cppcoreguidelines-*`) and fix all findings. ([ISO C++][1])

## 2. Design for Testability

* Use **Dependency Injection** (pass collaborators via constructor/params; no hidden singletons).
* Separate **policy from mechanism** (Strategy/policy classes, small interfaces).
* Wrap OS/3rd-party APIs in small **facades/adapters** instead of calling them directly. ([ISO C++][1])
* Prefer small, **immutable value types** for configuration and domain data.

## 3. Tests (GoogleTest)

* Every feature must have **automated tests** using GoogleTest/GoogleMock. ([Google GitHub][3])
* Structure: one suite per class/module; **one behavior per test**, clear names (`ParsesValidHeader`, `RejectsBadChecksum`, etc.).
* Use fixtures for shared setup; use `EXPECT_*` for non-fatal checks, `ASSERT_*` when the rest of the test depends on it. ([Google GitHub][3])
* Unit tests are **isolated and deterministic**: no real network, hardware, or filesystem; use mocks/fakes.

## 4. Definition of DONE (“PRONTO”)

A feature is **PRONTO / DONE** only if:

1. Code follows this standard (Core Guidelines + NASA-style safety where applicable).
2. **All tests pass** (unit + relevant integration).
3. Build is **warning-free** and **static analysis is clean**.

> If any test, build warning, or analysis check is red, the feature is **not done**.

[1]: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines?utm_source=chatgpt.com "C++ Core Guidelines - GitHub Pages"
[2]: https://en.wikipedia.org/wiki/The_Power_of_10%3A_Rules_for_Developing_Safety-Critical_Code?utm_source=chatgpt.com "The Power of 10: Rules for Developing Safety-Critical Code"
[3]: https://google.github.io/googletest/primer.html?utm_source=chatgpt.com "GoogleTest Primer"
