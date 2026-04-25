#pragma once

#include "imgui.h"
#include <cstdarg>

namespace UI::Colors {

// Extracts a single 8-bit channel from an ImU32 color.
inline unsigned char GetColorChannel(ImU32 color, int shift) {
    return static_cast<unsigned char>((color >> shift) & 0xFF);
}

// Normalizes a 0-255 channel into a 0.0-1.0 float.
inline float NormalizeChannel(unsigned char channel) {
    return static_cast<float>(channel) / 255.0f;
}

// Converts ImU32 RGBA into ImVec4.
inline ImVec4 ImU32ToImVec4(ImU32 color) {
    return ImVec4(NormalizeChannel(GetColorChannel(color, IM_COL32_R_SHIFT)),
                  NormalizeChannel(GetColorChannel(color, IM_COL32_G_SHIFT)),
                  NormalizeChannel(GetColorChannel(color, IM_COL32_B_SHIFT)),
                  NormalizeChannel(GetColorChannel(color, IM_COL32_A_SHIFT)));
}

// Convenience wrapper for ImGui::TextColored with ImU32 colors.
inline void TextColored(ImU32 color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(ImU32ToImVec4(color), fmt, args);
    va_end(args);
}

} // namespace UI::Colors
