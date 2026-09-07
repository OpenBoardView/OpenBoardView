/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2020 Pavel Kovalenko
 *
 * Adapted for OpenBoardView: assertions throw instead of trapping, so a
 * malformed .tvw fails gracefully rather than crashing the application.
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#if defined(_MSC_VER)
#define FUNCTION_NAME __FUNCTION__
// workaround for MSVC bug
#define CONSTEXPR_DEF const
#elif defined(__GNUC__)
#define FUNCTION_NAME __PRETTY_FUNCTION__
#define CONSTEXPR_DEF constexpr
#else
#define FUNCTION_NAME "?"
#define CONSTEXPR_DEF constexpr
#endif

// Verbose parse tracing (off by default; the parser is a library inside OBV).
#if !defined(TEBO_VERBOSE)
#define TEBO_VERBOSE 0
#endif
#if TEBO_VERBOSE
#define TEBO_LOG(...) std::printf(__VA_ARGS__)
#else
#define TEBO_LOG(...) ((void)0)
#endif

[[noreturn]] inline void Fail(char const *expr, char const *func,
    char const *file, int line, char const *desc)
{
    // report only the basename: full build paths are noise for the user
    char const *base = file;
    for (char const *p = file; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    (void)func;
    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s (%s) at %s:%d", desc, expr, base, line);
    throw std::runtime_error(buf);
}

#define R_ASSERT(expr) \
    do \
    { \
        if (!(expr)) \
            Fail(#expr, FUNCTION_NAME, __FILE__, __LINE__, "assertion failed"); \
    } while (0)
