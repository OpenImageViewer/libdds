// Platform definitions for the CPU-only DirectXTex extraction.
#pragma once

#define LIBDDS_CPU_ONLY 1

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "dxgiformat.h"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #define PAL_STDCPP_COMPAT
    #include <sal.h>
    #undef PAL_STDCPP_COMPAT
    // Legacy SAL names collide with identifiers in libstdc++ and libc++.
    #undef __valid
    #undef __in
    #undef __out
    #ifndef __cdecl
        #define __cdecl
    #endif
using HRESULT                          = std::int32_t;
using DWORD                            = std::uint32_t;
using UINT                             = unsigned int;
inline constexpr HRESULT S_OK          = 0;
inline constexpr HRESULT S_FALSE       = 1;
inline constexpr HRESULT E_NOTIMPL     = static_cast<HRESULT>(0x80004001u);
inline constexpr HRESULT E_NOINTERFACE = static_cast<HRESULT>(0x80004002u);
inline constexpr HRESULT E_POINTER     = static_cast<HRESULT>(0x80004003u);
inline constexpr HRESULT E_ABORT       = static_cast<HRESULT>(0x80004004u);
inline constexpr HRESULT E_FAIL        = static_cast<HRESULT>(0x80004005u);
inline constexpr HRESULT E_UNEXPECTED  = static_cast<HRESULT>(0x8000ffffu);
inline constexpr HRESULT E_OUTOFMEMORY = static_cast<HRESULT>(0x8007000eu);
inline constexpr HRESULT E_INVALIDARG  = static_cast<HRESULT>(0x80070057u);
inline constexpr HRESULT E_BOUNDS      = static_cast<HRESULT>(0x8000000bu);
constexpr bool SUCCEEDED(HRESULT value) noexcept
{
    return value >= 0;
}
constexpr bool FAILED(HRESULT value) noexcept
{
    return value < 0;
}
// Used only by upstream's disabled WIC dispatch stubs.
struct GUID
{
    std::uint32_t Data1;
    std::uint16_t Data2, Data3;
    std::uint8_t Data4[8];
};
    #ifndef UNREFERENCED_PARAMETER
        #define UNREFERENCED_PARAMETER(value) static_cast<void>(value)
    #endif
    #define DEFINE_ENUM_FLAG_OPERATORS(T)                                                                              \
        constexpr T operator|(T a, T b) noexcept                                                                       \
        {                                                                                                              \
            return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) |                                          \
                                  static_cast<std::underlying_type_t<T>>(b));                                          \
        }                                                                                                              \
        constexpr T operator&(T a, T b) noexcept                                                                       \
        {                                                                                                              \
            return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) &                                          \
                                  static_cast<std::underlying_type_t<T>>(b));                                          \
        }                                                                                                              \
        constexpr T operator^(T a, T b) noexcept                                                                       \
        {                                                                                                              \
            return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ^                                          \
                                  static_cast<std::underlying_type_t<T>>(b));                                          \
        }                                                                                                              \
        constexpr T operator~(T a) noexcept                                                                            \
        {                                                                                                              \
            return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a));                                         \
        }                                                                                                              \
        inline T& operator|=(T& a, T b) noexcept                                                                       \
        {                                                                                                              \
            return a = a | b;                                                                                          \
        }                                                                                                              \
        inline T& operator&=(T& a, T b) noexcept                                                                       \
        {                                                                                                              \
            return a = a & b;                                                                                          \
        }                                                                                                              \
        inline T& operator^=(T& a, T b) noexcept                                                                       \
        {                                                                                                              \
            return a = a ^ b;                                                                                          \
        }
#endif

