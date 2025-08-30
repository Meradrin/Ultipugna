#pragma once

#include <charconv>
#include <ranges>
#include <string_view>

using namespace std::literals;

inline constexpr auto AsStringView = std::views::transform([](auto&& V){ return std::string_view { V.data(), V.size()}; });
inline constexpr auto SkipEmpty = std::views::filter([](const std::string_view& SV){ return !SV.empty(); });

// Note: When we move to C++23 that can be simplified
struct ToVectorType
{
    template<class R>
    auto operator()(R&& Range) const
    {
        std::vector<std::ranges::range_value_t<R>> Output;
        for (auto&& V : Range) Output.emplace_back(std::forward<decltype(V)>(V));
        return Output;
    }

    template<class R>
    friend auto operator|(R&& Range, const ToVectorType& Self)
    {
        return Self(std::forward<R>(Range));
    }
};
inline constexpr ToVectorType ToVector{};

template<class Type>
constexpr bool StringToNumber(std::string_view String, Type& OutValue, int Base = 10) noexcept
{
    static_assert(std::is_integral_v<Type> && !std::is_same_v<Type, bool>);
    return std::from_chars(String.begin(), String.end(), OutValue, Base).ec == std::errc{};
}