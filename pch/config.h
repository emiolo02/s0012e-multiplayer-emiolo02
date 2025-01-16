#pragma once

//------------------------------------------------------------------------------
/**
    @file core/config.h

        Main configure file for types and OS-specific stuff.

        (C) 2015-2022 See the LICENSE file.
*/
// #ifdef __WIN32__
// #include "win32/pch.h"
// #endif
#define NOMINMAX

#include "GL/glew.h"
#include "GL/gl.h"
#include "core/debug.h"
#include "gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "gtc/quaternion.hpp"
#include "gtx/string_cast.hpp"
#include "gtx/transform.hpp"
#include <algorithm>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mat4x4.hpp> // glm::mat4
#include <stdint.h>
#include <vec3.hpp> // glm::vec3
#include <vec4.hpp> // glm::vec4
#include <xmmintrin.h>

using namespace glm;

typedef size_t index_t;
typedef unsigned int uint;
typedef unsigned short ushort;

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint8_t uchar;

typedef uint8_t byte;
typedef uint8_t ubyte;

typedef float float32;
typedef double float64;

#define j_min(x, y) x < y ? x : y
#define j_max(x, y) x > y ? x : y

#ifdef NULL
#undef NULL
#define NULL nullptr
#endif

// GCC settings
#if defined __GNUC__
#define __cdecl
#define __forceinline inline __attribute__((always_inline))
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#if !defined(__GNUC__)
#define __attribute__(x) /**/
#endif

#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif

#ifdef NDEBUG
#define LOG(msg)
#else
#define LOG(msg) std::cout << msg
#endif

struct Time {
    static uint64 Now() {
        const auto now = std::chrono::system_clock::now();
        const auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                .count();
    }
};
