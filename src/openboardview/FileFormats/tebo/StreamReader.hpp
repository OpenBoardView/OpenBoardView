/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2019 Pavel Kovalenko
 */

#pragma once

#include "Common.hpp"
#include "Fixed32.hpp"
#include <istream>

namespace Tebo
{
    class StreamReader
    {
    protected:
        std::istream &is;
        std::streampos rPos;

    public:
        StreamReader(std::istream &s) : is(s)
        {
            rPos = Tell();
        }

        std::streampos Tell() { return is.tellg(); }
        void Seek(size_t pos) { is.seekg(pos); }

        uint8_t ReadU8()
        {
            uint8_t r;
            is.read((char *)&r, 1);
            rPos = Tell();
            return r;
        }

        bool ReadBool8()
        {
            uint8_t const v = ReadU8();
            R_ASSERT(v <= 1);
            return v;
        }

        template <typename T>
        void Read(T *dst, size_t count)
        {
            std::streamsize const itemSize = sizeof(T);
            is.read((char *)dst, itemSize*count);
            rPos = Tell();
        }

        uint16_t ReadU16()
        {
            uint16_t r;
            Read(&r, 1);
            return r;
        }

        uint32_t ReadU32()
        {
            uint32_t r;
            Read(&r, 1);
            return r;
        }

        int32_t ReadS32()
        {
            int32_t r;
            Read(&r, 1);
            return r;
        }

        float ReadFloat()
        {
            float r;
            Read(&r, 1);
            return r;
        }

        Vector2S ReadVec2S()
        { return {ReadS32(), ReadS32()}; }

        std::string ReadString255()
        {
            uint8_t const size = ReadU8();
            std::string s;
            s.resize(size);
            Read(s.data(), size);
            return s;
        }
    };

    // inline: this header is included from multiple translation units
    template <>
    inline void StreamReader::Read(bool *dst, size_t count)
    {
        for (size_t i = 0; i < count; i++)
            dst[i] = ReadBool8();
    }

} // namespace Tebo
