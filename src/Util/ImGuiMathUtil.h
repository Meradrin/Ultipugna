#pragma once

#include "imgui.h"

inline ImVec2 operator-(const ImVec2& LeftHandSide, const ImVec2& RightHandSide)
{
    return ImVec2(LeftHandSide.x - RightHandSide.x, LeftHandSide.y - RightHandSide.y);
}

inline ImVec2 operator+(const ImVec2& LeftHandSide, const ImVec2& RightHandSide)
{
    return ImVec2(LeftHandSide.x + RightHandSide.x, LeftHandSide.y + RightHandSide.y);
}

inline ImVec2 operator*(const ImVec2& LeftHandSide, float Scalar)
{
    return ImVec2(LeftHandSide.x * Scalar, LeftHandSide.y * Scalar);
}

inline ImVec2 operator/(const ImVec2& LeftHandSide, const ImVec2& RightHandSide)
{
    return ImVec2(LeftHandSide.x / RightHandSide.x, LeftHandSide.y / RightHandSide.y);
}

inline ImVec2 operator*(const ImVec2& LeftHandSide, const ImVec2& RightHandSide)
{
    return ImVec2(LeftHandSide.x * RightHandSide.x, LeftHandSide.y * RightHandSide.y);
}

template <class Type>
[[nodiscard]] constexpr Type NextPowerOfTwo(Type Value) noexcept
{
    static_assert(std::is_integral_v<Type>, "NextPowerOfTwo: integral types only");
    using U = std::make_unsigned_t<Type>;
    if (Value <= 1) return Type(1);
    const U Result = std::bit_ceil(static_cast<U>(Value));
    if (Result != 0) return static_cast<Type>(Result);
    return static_cast<Type>(U(1) << (std::numeric_limits<U>::digits - 1));
}