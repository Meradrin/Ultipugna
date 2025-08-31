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