/*
 * Copyright (c) 2010-2026 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

namespace stdext
{
    // Robin Hood lib
    constexpr size_t hash_int(size_t x) noexcept
    {
        if constexpr (sizeof(size_t) >= 8) {
            x ^= x >> 33U;
            x *= UINT64_C(0xff51afd7ed558ccd);
            x ^= x >> 33U;
        } else {
            x ^= x >> 16U;
            x *= 0x85ebca6bU;
            x ^= x >> 13U;
            x *= 0xc2b2ae35U;
            x ^= x >> 16U;
        }
        return x;
    }

    // Boost Lib
    constexpr void hash_union(size_t& seed, const size_t h)
    {
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <class T>
    void hash_combine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        hash_union(seed, hasher(v));
    }
}
