#pragma once

#include <array>
#include <cstdint>
#include <source_location>

constexpr void FNV1A_64Update(std::uint64_t& Hash, unsigned char Character)
{
    Hash ^= Character;
    Hash *= 1099511628211ull;
}

constexpr void FNV1A_64CString(std::uint64_t& Hash, const char* CString)
{
    for (; *CString; ++CString)
        FNV1A_64Update(Hash, static_cast<unsigned char>(*CString));

    FNV1A_64Update(Hash, 0);
}

constexpr void FNV1A_64Uint32(std::uint64_t& Hash, std::uint32_t Value)
{
    FNV1A_64Update(Hash, (Value >>  0) & 0xFF);
    FNV1A_64Update(Hash, (Value >>  8) & 0xFF);
    FNV1A_64Update(Hash, (Value >> 16) & 0xFF);
    FNV1A_64Update(Hash, (Value >> 24) & 0xFF);
}

constexpr std::uint64_t SplitMix64(std::uint64_t Hash)
{
    Hash += 0x9E3779B97F4A7C15ull;
    Hash = (Hash ^ (Hash >> 30)) * 0xBF58476D1CE4E5B9ull;
    Hash = (Hash ^ (Hash >> 27)) * 0x94D049BB133111EBull;
    return Hash ^ (Hash >> 31);
}


// Attempts to generate a unique 64-bit identifier based on the current source location.
// The identifier is computed using file name, function name, line number and column information.
// Note: This is a compile-time function (consteval)
consteval std::uint64_t SourceLocationUniqueId64(std::source_location Location = std::source_location::current())
{
    std::uint64_t Hash = 1469598103934665603ull;
    FNV1A_64CString(Hash, Location.file_name());
    FNV1A_64CString(Hash, Location.function_name());
    FNV1A_64Uint32(Hash, Location.line());
    FNV1A_64Uint32(Hash, Location.column());
    return SplitMix64(Hash);
}


// Creates a unique string identifier prefixed with "ID_" based on source location information
// The resulting string is 19 characters long:
// - First 3 characters are "ID_"
// - Following 16 characters represent a hexadecimal hash derived from file name, function name,
//   line number and column information
// Note: This is a compile-time function (consteval)
consteval std::array<char,19> SourceLocationUniqueCStrHexId(std::source_location Location = std::source_location::current())
{
    std::array<char,19> Output{'I', 'D', '_'};
    const uint64_t Hash = SourceLocationUniqueId64(Location);
    for (int Index = 0; Index < 16; ++Index)
    {
        constexpr const char* HexCharacter = "0123456789ABCDEF";
        Output[3 + Index] = HexCharacter[(Hash >> (4 * (15 - Index))) & 0xf];
    }
    return Output;
}
