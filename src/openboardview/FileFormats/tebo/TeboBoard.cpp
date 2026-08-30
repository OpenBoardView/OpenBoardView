/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2020 Pavel Kovalenko
 */

#include "TeboBoard.hpp"

namespace Tebo
{
    std::unique_ptr<Shape> Shape::Load(StreamReader &r)
    {
        {
            uint32_t one = r.ReadU32();
            (void)one;
            R_ASSERT(one == 1);
        }
        auto const size = r.ReadVec2S();
        auto const type = ShapeType(r.ReadU32());
        switch (type)
        {
        case ShapeType::Round:
        {
            auto const skip = r.ReadVec2S();
            (void)skip;
            return std::unique_ptr<Round>(new Round(size));
        }
        case ShapeType::Rect:
        {
            auto const turn = r.ReadFloat();
            auto const skip = r.ReadS32();
            (void)skip;
            return std::unique_ptr<Rect>(new Rect(size, turn));
        }
        case ShapeType::Poly:
        {
            auto const skip = r.ReadU32();
            (void)skip;
            auto const name = r.ReadString255();
            auto shape = std::unique_ptr<Poly>(new Poly(size, name));
            shape->BBox.Min = r.ReadVec2S();
            shape->BBox.Max = r.ReadVec2S();
            auto subObjCount = r.ReadU32();
            for (uint32_t i = 0; i < subObjCount; i++)
            {
                auto const subObjType = r.ReadU32();
                switch (subObjType)
                {
                case 2: // poly
                {
                    R_ASSERT(shape->Vertices.empty());
                    r.Read(shape->Flags, 3);
                    uint32_t const vertexCount = r.ReadU32();
                    shape->Vertices.reserve(vertexCount);
                    for (uint32_t i = 0; i < vertexCount; i++)
                        shape->Vertices.push_back(r.ReadVec2S());
                    break;
                }
                case 5: // line
                {
                    shape->Lines.emplace_back();
                    shape->Lines.back().Load(r);
                    break;
                }
                default:
                    R_ASSERT(!"Unrecognized subobject type");
                    break;
                }
            }
            return shape;
        }
        case ShapeType::RoundRect:
        {
            auto const turn = r.ReadFloat();
            auto const cornerRadius = r.ReadS32();
            return std::unique_ptr<RoundRect>(new RoundRect(size, cornerRadius, turn));
        }
        default:
            R_ASSERT(!"Unrecognized shape type");
            return nullptr;
        }
    }

    void Poly::Line::Load(StreamReader &r)
    {
        Param1 = r.ReadU32();
        R_ASSERT(Param1 == 1);
        Param2 = r.ReadU32();
        R_ASSERT(Param2 == 0);
        Param3 = r.ReadU32();
        R_ASSERT(Param3 == 0);
        Start = r.ReadVec2S();
        End = r.ReadVec2S();
        Width = r.ReadS32();
    }

    static void DecodeString(std::string &s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            char c = s[i];
            if ('a' <= c && c <= 'j')
            {
                char x = c - i%3 - 4;
                if (x < 'a')
                    x += 10;
                s[i] = 154 - x;
                continue;
            }
            if ('k' <= c && c <= 'z')
            {
                char x = c - i%10 - 5;
                c = x;
                if (x < 'k')
                    c = x + 16;
                s[i] = c;
                continue;
            }
            if ('A' <= c && c <= 'Z')
            {
                char x = c + i%10 + 5;
                c = x;
                if (x > 'Z')
                    c = x - 26;
                s[i] = c;
                continue;
            }
            if ('0' <= c && c <= '9')
            {
                char x = c + i%3 + 4;
                if (x > '9')
                    x -= 10;
                s[i] = x + 49;
                continue;
            }
        }
    }

    void TvwHeader::Load(StreamReader &r)
    {
        Type = r.ReadString255();
        DecodeString(Type);
        Const1 = r.ReadU32();
        Customer = r.ReadString255();
        DecodeString(Customer);
        Const2 = r.ReadU8();
        Date = r.ReadString255();
        DecodeString(Date);
        r.Read(Const3, 3);
        Size1 = r.ReadU32();
        Size2 = r.ReadU32();
        Size3 = r.ReadU32();
        LayerCount = r.ReadU32();
    }

    ObjectType Object::Detect(StreamReader &r)
    {
        uint32_t const maxSkips = 4;
        for (uint32_t i = 0; i < maxSkips; i++)
        {
            uint32_t const type = r.ReadU32();
            if (!type)
                continue;
            return ObjectType(type);
        }
        return ObjectType::Undefined;
    }

    [[maybe_unused]] static char const *LayerTypeToString(LayerType type)
    {
        switch (type)
        {
        case LayerType::Document: return "Document";
        case LayerType::Top: return "Top";
        case LayerType::Bottom: return "Bottom";
        case LayerType::Signal: return "Signal";
        case LayerType::Plane: return "Plane";
        case LayerType::SolderTop: return "SolderTop";
        case LayerType::SolderBottom: return "SolderBottom";
        case LayerType::SilkTop: return "SilkTop";
        case LayerType::SilkBottom: return "SilkBottom";
        case LayerType::PasteTop: return "PasteTop";
        case LayerType::PasteBottom: return "PasteBottom";
        case LayerType::Drill: return "Drill";
        case LayerType::Roul: return "Roul";
        default: return "unknown";
        }
    }

    void Object::Load(StreamReader &r)
    {
        // 3, 2, 1 # logic layer
        // 1, 2, 1 # thru layer (from 0 to 0)
        uint32_t const pos(r.Tell());
        (void)pos;
        r.Read(Magic, 2);
        R_ASSERT(Magic[0] == 2 && Magic[1] == 1);
        Name = r.ReadString255();
        InitialName = r.ReadString255();
        InitialPath = r.ReadString255();
        Type = LayerType(r.ReadU32());
        PadColor = r.ReadU32();
        LineColor = r.ReadU32();
        TEBO_LOG("- loading object name[%s] type[%s] addr[0x%08X]\n",
            Name.c_str(), LayerTypeToString(Type), pos);
    }

    void TestPoint::Load(StreamReader &r)
    {
        Flag1 = r.ReadBool8();
        P1 = r.ReadS32();
        Handle = r.ReadS32();
        P2 = r.ReadS32();
        P3 = r.ReadS32();
        Pos = r.ReadVec2S();
        ValidatePos(Pos);
        P4 = r.ReadS32();
        Flag2 = r.ReadBool8();
        P5 = r.ReadS32();
        P6 = r.ReadS32();
        N = r.ReadS32();
    }

    void TestPoint2::Load(StreamReader &r)
    {
        P1 = r.ReadS32();
        Handle = r.ReadS32();
        P2 = r.ReadS32();
        Pos = r.ReadVec2S();
        ValidatePos(Pos);
        Pos1 = r.ReadVec2S();
        Pos2 = r.ReadVec2S();
        Flag1 = r.ReadBool8();
        Flag2 = r.ReadBool8();
        Flag3 = r.ReadBool8();
        Nail = r.ReadU32();
        Param = r.ReadS32();
        Flag4 = r.ReadBool8();
        Flag5 = r.ReadBool8();
        Flag6 = r.ReadBool8();
        N = r.ReadS32();
    }

    void TestNode::Load(StreamReader &r)
    {
        Current = r.ReadU32();
        Next = r.ReadU32();
        Flag = r.ReadBool8();
    }

    void UnknownItem::Load(StreamReader &r)
    {
        Name = r.ReadString255();
        Pos = r.ReadVec2S();
        Z1 = r.ReadS32();
        Param1 = r.ReadS32();
        Param2 = r.ReadS32();
        Param3 = r.ReadS32();
        Z2 = r.ReadS32();
        Z3 = r.ReadS32();
        r.Read(Flags, 3);
        Param4 = r.ReadS32();
    }

    void LogicLayer::LoadShapes(StreamReader &r)
    {
        // this is actually max dcode, not a 'count'
        uint32_t shapeCount = r.ReadU32();
        if (!shapeCount)
            return;
        R_ASSERT(shapeCount >= 10);
        shapeCount -= 10;
        for (uint32_t i = 0; i < shapeCount; i++)
        {
            std::unique_ptr<Shape> shape = Shape::Load(r);
            R_ASSERT(shape != nullptr);
            Shapes.push_back(std::move(shape));
        }
    }

    void LogicLayer::LoadPads(StreamReader &r)
    {
        uint32_t const instanceCount = r.ReadU32();
        if (!instanceCount)
            return;
        {
            uint32_t const two = r.ReadU32();
            (void)two;
            R_ASSERT(two == 2);
        }
        Pads.reserve(instanceCount);
        for (uint32_t i = 0; i < instanceCount; i++)
        {
            Pad obj;
            obj.Net = r.ReadS32();
            obj.DCode = r.ReadU32();
            obj.Pos = r.ReadVec2S();
            obj.IsExposed = r.ReadBool8();
            obj.IsCopper = r.ReadBool8();
            obj.TestpointParam = r.ReadU8();
            R_ASSERT(obj.DCode-10 < Shapes.size());
            obj.Shape = Shapes[obj.DCode-10].get();
            if (obj.IsCopper)
            {
                obj.IsSomething = r.ReadBool8();
                if (obj.TestpointParam == 1)
                    r.Read(obj.TestPoint.Data12, 12);
                if (obj.IsExposed || obj.IsSomething)
                { // bbox of exposed area of this pad without rotation
                    obj.Exposed.Min = r.ReadVec2S();
                    obj.Exposed.Max = r.ReadVec2S();
                }
                obj.HasHole = r.ReadBool8();
                obj.TailParam = r.ReadU8();
                if (obj.HasHole)
                {
                    r.Read(obj.Hole.Data7, 7);
                    obj.Hole.Size = r.ReadVec2S();
                    obj.Hole.Param = r.ReadU8();
                }
            }
            else // not copper
            {
                R_ASSERT(!obj.IsExposed && obj.TestpointParam != 1); // not expected
                // no exposed copper => no extra data
            }
            Pads.push_back(std::move(obj));
        }
    }

    void LogicLayer::LoadLines(StreamReader &r)
    {
        uint32_t const instanceCount = r.ReadU32();
        if (!instanceCount)
            return;
        {
            uint32_t const zero = r.ReadU32();
            (void)zero;
            R_ASSERT(!zero);
        }
        Lines.reserve(instanceCount);
        for (uint32_t i = 0; i < instanceCount; i++)
        {
            Line obj;
            obj.Net = r.ReadS32();
            obj.DCode = r.ReadU32();
            R_ASSERT(obj.DCode-10 < Shapes.size());
            obj.StartPos = r.ReadVec2S();
            obj.EndPos = r.ReadVec2S();
            Lines.push_back(std::move(obj));
        }
    }

    void LogicLayer::LoadArcs(StreamReader &r)
    {
        uint32_t const instanceCount = r.ReadU32();
        if (!instanceCount)
            return;
        {
            uint32_t zero = r.ReadU32();
            (void)zero;
            R_ASSERT(!zero);
        }
        Arcs.reserve(instanceCount);
        for (uint32_t i = 0; i < instanceCount; i++)
        {
            Arc obj;
            obj.Net = r.ReadS32();
            obj.DCode = r.ReadU32();
            R_ASSERT(obj.DCode-10 < Shapes.size());
            obj.Pos = r.ReadVec2S();
            obj.Radius = r.ReadS32();
            obj.StartAngle = r.ReadFloat();
            obj.SweepAngle = r.ReadFloat();
            Arcs.push_back(std::move(obj));
        }
    }

    void LogicLayer::LoadSurfaces(StreamReader &r)
    {
        uint32_t const instanceCount = r.ReadU32();
        if (!instanceCount)
            return;
        {
            uint32_t two = r.ReadU32();
            (void)two;
            R_ASSERT(two == 2);
        }
        Surfaces.reserve(instanceCount);
        for (uint32_t i = 0; i < instanceCount; i++)
        {
            Surface obj;
            obj.Net = r.ReadS32();
            obj.EdgeCount = r.ReadU32();
            obj.Vertices.reserve(obj.EdgeCount);
            for (uint32_t vi = 0; vi < obj.EdgeCount; vi++)
                obj.Vertices.push_back(r.ReadVec2S());
            obj.LineWidth = r.ReadS32();
            obj.VoidCount = r.ReadU32();
            if (obj.VoidCount)
            {
                obj.Voids.reserve(obj.VoidCount);
                for (uint32_t vi = 0; vi < obj.VoidCount; vi++)
                {
                    Cutout cutout;
                    cutout.Tag = r.ReadU32();
                    R_ASSERT(cutout.Tag <= 1);
                    cutout.EdgeCount = r.ReadU32();
                    cutout.Vertices.reserve(cutout.EdgeCount);
                    for (uint32_t ci = 0; ci < cutout.EdgeCount; ci++)
                        cutout.Vertices.push_back(r.ReadVec2S());
                    obj.Voids.push_back(std::move(cutout));
                }
                obj.VoidFlags = r.ReadU32();
            }
            Surfaces.push_back(std::move(obj));
        }
    }

    void LogicLayer::LoadUnknownItems(StreamReader &r)
    {
        UnknownItemCount = r.ReadU32();
        UnknownItemsParam = r.ReadU32();
        if (UnknownItemCount)
        {
            UnknownItems.reserve(UnknownItemCount);
            for (uint32_t i = 0; i < UnknownItemCount; i++)
            {
                UnknownItems.emplace_back();
                UnknownItems.back().Load(r);
            }
            uint32_t const skip = r.ReadU32();
            (void)skip;
            R_ASSERT(skip == 0);
        }
        uint32_t const skip = r.ReadU32();
        (void)skip;
        R_ASSERT(skip == 7);
    }

    void LogicLayer::LoadTestpoints(StreamReader &r)
    {
        TpCount = r.ReadU32();
        TestPoints.reserve(TpCount);
        for (uint32_t i = 0; i < TpCount; i++)
        {
            TestPoint obj;
            obj.Load(r);
            TestPoints.push_back(obj);
        }
        {
            uint32_t skip = r.ReadU32();
            (void)skip;
            R_ASSERT(skip == 0);
            skip = r.ReadU32();
            R_ASSERT(skip == 4);
        }
        TPS2Size = r.ReadU32();
        TPS2Param = r.ReadU32();
        TestPoints2.reserve(TPS2Size);
        for (uint32_t i = 0; i < TPS2Size; i++)
        {
            TestPoint2 obj;
            obj.Load(r);
            TestPoints2.push_back(obj);
        }
        TPS3Size = r.ReadU32();
        TPS3Param = r.ReadU32();
        TestPoints3.reserve(TPS3Size);
        for (uint32_t i = 0; i < TPS3Size; i++)
        {
            TestPoint2 obj;
            obj.Load(r);
            TestPoints3.push_back(obj);
        }
        TestSequenceSize = r.ReadU32();
        TestSequenceParam = r.ReadU32();
        TestSequence.reserve(TestSequenceSize);
        for (uint32_t i = 0; i < TestSequenceSize; i++)
        {
            TestNode obj;
            obj.Load(r);
            TestSequence.push_back(obj);
        }
        if (TestSequenceParam == 1)
        {
            uint32_t skip[3];
            r.Read(skip, 3);
            R_ASSERT(skip[0] == 0);
            R_ASSERT(skip[1] == 0);
            R_ASSERT(skip[2] == 0);
        }
    }

    void LogicLayer::Load(StreamReader &r)
    {
        Object::Load(r);
        LoadShapes(r);
        if (!Shapes.empty())
        {
            bool extraData = false;
            {
                auto const skip1 = r.ReadU32();
                (void)skip1;
                // NOTE: this might be incorrect way to determine data order
                // (maybe it has to be bound to layer type?)
                switch (skip1)
                {
                default:
                    R_ASSERT(!"Unrecognized data order");
                    break;
                case 1: // normal: shapes, [these flags], pads, lines, arcs, surfaces
                    break;
                case 2: // extra data: shapes, [these flags], pads, lines(0), arcs(0), surfaces, int32[4], lines, arcs
                    extraData = true;
                    break;
                }
                auto const skip2 = r.ReadU32();
                R_ASSERT(skip2 == 0);
                auto const skip3 = r.ReadU32();
                R_ASSERT(skip3 == 1);
            }
            LoadPads(r);
            LoadLines(r);
            LoadArcs(r);
            LoadSurfaces(r);
            if (extraData)
            {
                uint32_t skip3[4];
                r.Read(skip3, 4);
                TEBO_LOG("* skip: %u, %u, %u, %u\n",
                    skip3[0], skip3[1], skip3[2], skip3[3]);
                // reload lines and arcs
                LoadLines(r);
                LoadArcs(r);
                uint32_t const skip4 = r.ReadU32();
                R_ASSERT(skip4 == 0);
            }
        }
        LoadUnknownItems(r);
        LoadTestpoints(r);
    }

    void ThroughLayer::Tool::Load(StreamReader &r)
    {
        Flag1 = r.ReadBool8();
        Flag2 = r.ReadBool8();
        Size = r.ReadU32();
        r.Read(Data5, 5);
        r.Read(Data3, 3);
    }

    void ThroughLayer::DrillHole::Load(StreamReader &r)
    {
        Net = r.ReadS32();
        Tool = r.ReadU32();
        Pos = r.ReadVec2S();
    }

    void ThroughLayer::DrillSlot::Load(StreamReader &r)
    {
        Net = r.ReadS32();
        Tool = r.ReadU32();
        Begin = r.ReadVec2S();
        End = r.ReadVec2S();
        Zero = r.ReadU32();
    }

    void ThroughLayer::Load(StreamReader &r)
    {
        Object::Load(r);
        auto zero = r.ReadU32();
        (void)zero;
        R_ASSERT(zero == 0);
        zero = r.ReadU32();
        R_ASSERT(zero == 0);
        auto toolCount = r.ReadU32();
        if (toolCount == 0)
            return; // empty through-layer: no tools, no drills
        toolCount--;
        Tools.reserve(toolCount);
        for (uint32_t i = 0; i < toolCount; i++)
        {
            Tools.emplace_back();
            Tools.back().Load(r);
        }
        zero = r.ReadU8();
        R_ASSERT(zero == 0);
        auto const drillCount = r.ReadU32();
        {
            auto v2 = r.ReadU32();
            (void)v2;
            TEBO_LOG("- drill holes[%u], v2[%u]\n", drillCount, v2);
            // skip 4 zero dwords
            uint32_t dummy[4];
            r.Read(dummy, 4);
            R_ASSERT(dummy[0] == 0);
            R_ASSERT(dummy[1] == 0);
            R_ASSERT(dummy[2] == 0);
            R_ASSERT(dummy[3] == 0);
        }
        DrillHoles.reserve(drillCount);
        // XXX: drill slot count must be stored somewhere
        DrillSlots.reserve(100);
        for (uint32_t i = 0; i < drillCount; i++)
        {
            switch (r.ReadU8())
            {
            case 0x08:
                DrillHoles.emplace_back();
                DrillHoles.back().Load(r);
                continue;
            case 0x0A:
            case 0x0B:
                DrillSlots.emplace_back();
                DrillSlots.back().Load(r);
                continue;
            default:
                R_ASSERT(!"Unrecognized drill code");
            }
        }
    }

    static std::unique_ptr<Object> LoadObject(StreamReader &r)
    {
        Object *object = nullptr;
        ObjectType const type = Object::Detect(r);
        switch (type)
        {
        case ObjectType::Logic:
            object = new LogicLayer();
            break;
        case ObjectType::Through:
            object = new ThroughLayer();
            break;
        default:
            R_ASSERT(!"Unrecognized ObjectType");
            break;
        }
        if (object)
        {
            object->Load(r);
            TEBO_LOG("- done at addr[0x%08X]\n", uint32_t(r.Tell()));
        }
        return std::unique_ptr<Object>(object);
    }

    void ProbeBox32::Load(StreamReader &r)
    {
        Tag = r.ReadS32();
        V1 = r.ReadVec2S();
        V2 = r.ReadVec2S();
    }

    void DoubleBox32::Load(StreamReader &r)
    {
        Tag = r.ReadU32();
        B1.Load(r);
        B2.Load(r);
    }

    void ProbeBox8::Load(StreamReader &r)
    {
        Tag = r.ReadU8();
        N = r.ReadS32();
        A = r.ReadS32();
        P1 = r.ReadS32();
        P2 = r.ReadS32();
    }

    void ProbeDataItem::Load(StreamReader &r)
    {
        Present = r.ReadBool8();
        if (Present)
        {
            Size = r.ReadS32();
            r.Read(Params, 5);
            Color = r.ReadU32();
        }
    }

    void FixtureData::Load(StreamReader &r)
    {
        P1 = r.ReadU32();
        r.Read(PX, 6);
        r.Read(Flags, 3);
        uint32_t const itemCount = r.ReadU32();
        R_ASSERT(itemCount > 0);
        Items.reserve(itemCount);
        for (uint32_t i = 0; i < itemCount; i++)
        {
            Items.emplace_back();
            Items.back().Load(r);
        }
        uint32_t const boxCount = r.ReadU32();
        C1 = r.ReadU32();
        V1 = r.ReadVec2S();
        V2 = r.ReadVec2S();
        Boxes.reserve(boxCount);
        for (uint32_t i = 0; i < boxCount; i++)
        {
            Boxes.emplace_back();
            Boxes.back().Load(r);
        }
    }

    void ProbeData::Load(StreamReader &r)
    {
        Fixture.Load(r);
        V3 = r.ReadVec2S();
        V4 = r.ReadVec2S();
        uint32_t const box2Count = r.ReadU32();
        Boxes2.reserve(box2Count);
        for (uint32_t i = 0; i < box2Count; i++)
        {
            Boxes2.emplace_back();
            Boxes2.back().Load(r);
        }
    }

    void Probe::Load(StreamReader &r)
    {
        Header.Flag = r.ReadBool8();
        Header.Tag = r.ReadU32();
        Header.Name = r.ReadString255();
        Header.Size1 = r.ReadS32();
        Header.Param1 = r.ReadU32();
        Header.Size2 = r.ReadS32();
        Header.Param2 = r.ReadU32();
        Header.Size3 = r.ReadS32();
        Header.Param3 = r.ReadU32();
        Header.Color = r.ReadU32();
        Header.K1 = r.ReadU32();
        Header.V1 = r.ReadU32();
        Header.K2 = r.ReadU32();
        Header.V2 = r.ReadU32();
        Header.K3 = r.ReadU32();
        Header.V3 = r.ReadU32();
        Header.K4 = r.ReadU32();
        Header.V4 = r.ReadU32();
        bool const hasBody = r.ReadBool8();
        if (hasBody)
        {
            ProbeData body;
            body.Load(r);
            Body = std::unique_ptr<ProbeData>(new ProbeData());
            *Body = std::move(body);
        }
        Tail.Tag = r.ReadU32();
        Tail.Flag1 = r.ReadBool8();
        Tail.Flag2 = r.ReadBool8();
        Tail.Flag3 = r.ReadBool8();
        Tail.P0 = r.ReadU8();
        Tail.P1 = r.ReadS32();
        Tail.P2 = r.ReadS32();
        Tail.P3 = r.ReadS32();
        Tail.B1.Tag = r.ReadS32();
        Tail.B1.V1 = r.ReadVec2S();
        Tail.B1.V2 = r.ReadVec2S();
        Tail.B2.Tag = r.ReadS32();
        Tail.B2.V1 = r.ReadVec2S();
        Tail.B2.V2 = r.ReadVec2S();
    }

    void ProbeRegistry::Load(StreamReader &r)
    {
        Z1 = r.ReadU32();
        R_ASSERT(Z1 == 0);
        Z2 = r.ReadU32();
        R_ASSERT(Z2 == 0);
        Param = r.ReadU32();
        R_ASSERT(Param == 4);
        Name = r.ReadString255();
        DefaultSize = r.ReadU32();
        uint32_t const packCount = r.ReadU32();
        R_ASSERT(packCount > 0);
        Packs.reserve(packCount);
        for (uint32_t ip = 0; ip < packCount; ip++)
        {
            ProbePack pack;
            uint32_t const probeCount = r.ReadU32();
            for (uint32_t i = 0; i < probeCount; i++)
            {
                Probe p;
                p.Load(r);
                pack.push_back(std::move(p));
            }
            Packs.push_back(std::move(pack));
        }
    }

    void FixtureVariant::Load(StreamReader &r)
    {
        Name = r.ReadString255();
        ShortName = r.ReadString255();
        Flag1 = r.ReadBool8();
        Flag2 = r.ReadBool8();
        Data.Load(r);
    }

    void FixtureSetting::Load(StreamReader &r)
    {
        Tag = r.ReadU32();
        R_ASSERT(Tag == 3);
        Name = r.ReadString255();
        Param = r.ReadU32();
        R_ASSERT(Param == 0);
        uint32_t const variantCount = r.ReadU32();
        Variants.reserve(variantCount);
        for (uint32_t i = 0; i < variantCount; i++)
        {
            Variants.emplace_back();
            Variants.back().Load(r);
        }
        WorkspaceSize = r.ReadVec2S();
    }

    void FixtureRegistry::Load(StreamReader &r)
    {
        Tag1 = r.ReadU32();
        R_ASSERT(Tag1 == 0);
        Tag2 = r.ReadU32();
        R_ASSERT(Tag2 == 7874);
        Grids.reserve(8);
        for (uint32_t i = 0; i < 8; i++)
            Grids.push_back(r.ReadString255());
        Top.Load(r);
        Bottom.Load(r);
    }

    void Pin::Load(StreamReader &r)
    {
        Handle = r.ReadU32();
        Z1 = r.ReadU32();
        R_ASSERT(Z1 == 0);
        Id = r.ReadU32();
        Name = r.ReadString255();
        Z2 = r.ReadU32();
        R_ASSERT(Z2 == 0);
    }

    void Part::Load(StreamReader &r)
    {
        Name = r.ReadString255();
        Bbox.Min = r.ReadVec2S();
        Bbox.Max = r.ReadVec2S();
        Pos = r.ReadVec2S();
        Angle = r.ReadS32();
        Decal = r.ReadU32();
        Type = PartType(r.ReadU32());
        Z1 = r.ReadU32();
        R_ASSERT(Z1 == 0);
        Height = r.ReadS32();
        Flag0 = r.ReadBool8();
        Value = r.ReadString255();
        ToleranceP = r.ReadString255();
        ToleranceN = r.ReadString255();
        Desc = r.ReadString255();
        if (Flag0)
        {
            Serial = r.ReadString255();
            Z2 = r.ReadU32();
            R_ASSERT(Z2 == 0);
        }
        auto const pinCount = r.ReadU32();
        Layer = r.ReadU32();
        P2 = r.ReadU32();
        R_ASSERT(P2 == 0);
        Pins.reserve(pinCount);
        for (uint32_t i = 0; i < pinCount; i++)
        {
            Pins.emplace_back();
            Pins.back().Load(r);
        }
    }

    void MysteriousBlock::Load(StreamReader &r)
    {
        P1 = r.ReadU32();
        P2 = r.ReadU32();
        TopRight = r.ReadVec2S();
        P3 = r.ReadU32();
        P4 = r.ReadU32();
        Flag1 = r.ReadBool8();
        Flag2 = r.ReadBool8();
        P5 = r.ReadU8();
        P6 = r.ReadU8();
        r.Read(P7x, 4);
        r.Read(Flags, 6);
        P8 = r.ReadU32();
        P9 = r.ReadU32();
        P10 = r.ReadU32();
        P11 = r.ReadU32();
        P12 = r.ReadU8();
        P13 = r.ReadU8();
    }

    void Decal::Load(StreamReader &r)
    {
        Flag1 = r.ReadBool8();
        R_ASSERT(Flag1);
        Name = r.ReadString255();
        r.Read(HeaderParams, 3);
        Flag = r.ReadBool8();
        for (uint32_t i = 0; i < Layers.size(); i++)
        {
            bool const present = r.ReadBool8();
            if (!present)
                continue;
            Layers[i] = LoadObject(r);
        }
        OutlineFlag = r.ReadBool8();
        R_ASSERT(OutlineFlag);
        Param = r.ReadU32();
        N1 = r.ReadS32();
        OutlineVertexCount = r.ReadU32();
        Outline.reserve(OutlineVertexCount);
        for (uint32_t i = 0; i < OutlineVertexCount; i++)
        {
            Vector2S const v = r.ReadVec2S();
            Outline.push_back(v);
        }
        r.Read(Params, 2);
    }

    void Board::ReadNetList(StreamReader &r)
    {
        uint32_t const netCount = r.ReadU32();
        uint32_t const nc2 = r.ReadU32();
        (void)nc2;
        R_ASSERT(netCount > 0 && netCount == nc2);
        TEBO_LOG("- loading %u nets\n", netCount);
        Nets.reserve(netCount);
        for (uint32_t i = 0; i < netCount; i++)
            Nets.push_back(r.ReadString255());
    }

    void Board::ReadParts(StreamReader &r)
    {
        uint32_t const partCount = r.ReadU32();
        (void)partCount;
        uint32_t const skip = r.ReadU32();
        (void)skip;
        TEBO_LOG("- loading %u parts\n", partCount);
        Parts.reserve(partCount);
        for (uint32_t i = 0; i < partCount; i++)
        {
            Parts.emplace_back();
            Parts.back().Load(r);
        }
    }

    void Board::ReadDecals(StreamReader &r)
    {
        uint32_t const c = r.ReadU32();
        (void)c;
        R_ASSERT(c == 3);
        uint32_t decalCount = r.ReadU32();
        TEBO_LOG("- loading %u decals\n", decalCount);
        Decals.reserve(decalCount);
        for (uint32_t i = 0; i < decalCount; i++)
        {
            Decals.emplace_back();
            Decals.back().Load(r);
        }
    }

    void Board::Read(std::istream &fs)
    {
        StreamReader r(fs);
        Header.Load(r);
        Layers.reserve(Header.LayerCount);
        for (uint32_t li = 0; li < Header.LayerCount; li++)
        {
            std::unique_ptr<Object> layer = LoadObject(r);
            Layers.push_back(std::move(layer));
        }
        { // skip 4 zero dwords
            uint32_t dummy[4];
            r.Read(dummy, 4);
            R_ASSERT(dummy[0] == 0);
            R_ASSERT(dummy[1] == 0);
            R_ASSERT(dummy[2] == 0);
            R_ASSERT(dummy[3] == 0);
        }
        ReadNetList(r);
        Probes.Load(r);
        Fixtures.Load(r);
        Myb.Load(r);
        ReadParts(r);
        ReadDecals(r);
        TEBO_LOG("- done reading at addr[0x%08X]\n", uint32_t(r.Tell()));
    }
} // namespace Tebo
