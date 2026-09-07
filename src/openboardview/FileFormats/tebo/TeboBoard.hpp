/*
 * Tebo-ICT View (.tvw) board format parser
 * By: Pavel Kovalenko
 * Source: https://github.com/nitrocaster/eagleview
 *
 * MIT License
 * Copyright (c) 2020 Pavel Kovalenko
 */

#pragma once

#include "Common.hpp"
#include "Fixed32.hpp"
#include "StreamReader.hpp"
#include "Box2.hpp"
#include <istream> // std::istream
#include <vector>
#include <array>
#include <memory> // std::unique_ptr
#include <string>
#include <cstring>

namespace Tebo
{
    struct Box2S
    {
        Vector2S Min, Max;

        Vector2S Size() const
        { return Vector2S{Max.X-Min.X, Max.Y-Min.Y}; }

        template <typename T>
        operator Box2T<T>() const
        { return Box2T<T>(Min, Max); }
    };

    inline void ValidatePos(Vector2S) {}

    enum class LayerType : uint32_t
    {
        Document = 0,
        Top = 1,
        Bottom = 2,
        Signal = 3,
        Plane = 4, // PowerGround
        SolderTop = 5,
        SolderBottom = 6,
        SilkTop = 7,
        SilkBottom = 8,
        PasteTop = 9,
        PasteBottom = 10,
        Drill = 11,
        Roul = 12
    };

    enum class PrimitiveType
    {
        Line,
        Arc,
        Surface
    };

    enum class ShapeType
    {
        Round = 0,
        Rect = 1,
        RoundRect = 3,
        Poly = 5,
    };

    struct Shape
    {
        ShapeType Type;
        Vector2S Size;
        std::string Name;
        float Turn; // degrees

        Shape(ShapeType shapeType, Vector2S shapeSize, float turn)
        {
            Type = shapeType;
            Size = shapeSize;
            Turn = turn;
        }

        virtual ~Shape() = 0;

        static std::unique_ptr<Shape> Load(StreamReader &r);
    };

    inline Shape::~Shape() = default;

    struct Round : public Shape
    {
        Round(Vector2S shapeSize) :
            Shape(ShapeType::Round, shapeSize, 0)
        {}
    };

    struct Rect : public Shape
    {
        Rect(Vector2S shapeSize, float turn) :
            Shape(ShapeType::Rect, shapeSize, turn)
        {}
    };
    
    struct Poly : public Shape
    {
        struct Line
        {
            int32_t Param1, Param2, Param3; // 1, 0, 0
            Vector2S Start, End;
            Fixed32 Width;

            void Load(StreamReader &r);
        };

        Box2S BBox;
        std::vector<Poly::Line> Lines;
        int32_t Flags[3] = {};
        std::vector<Vector2S> Vertices;

        Poly(Vector2S shapeSize, std::string const &shapeName) :
            Shape(ShapeType::Poly, shapeSize, 0)
        {
            Name = shapeName;
        }
    };

    struct RoundRect : public Shape
    {
    public:
        Fixed32 CornerRadius;

        RoundRect(Vector2S size, Fixed32 r, float turn) :
            Shape(ShapeType::RoundRect, size, turn),
            CornerRadius(r)
        {}
    };
    
    struct Pad
    {
        Tebo::Shape *Shape;
        int32_t Net;
        uint32_t DCode;
        Vector2S Pos;
        bool IsExposed;
        bool IsCopper;
        uint8_t TestpointParam; // 0 - smd pin, 1 - accessible, 2 - mask
        // extensions
        bool IsSomething = false;
        struct TestPointData
        {
            uint8_t Data12[12] = {};
        } TestPoint;
        struct ExposedData
        {
            Vector2S Min = {};
            Vector2S Max = {};
        } Exposed;
        bool HasHole = false;
        uint8_t TailParam = 0;
        struct HoleData
        {
            uint8_t Data7[7] = {};
            Vector2S Size = {};
            uint8_t Param = 0;
        } Hole;
    };

    struct Primitive
    {
        int32_t Net;
        PrimitiveType Type;
        uint32_t DCode;

        Primitive(PrimitiveType type)
        { Type = type; }

        virtual ~Primitive() = 0;
    };

    inline Primitive::~Primitive() = default;

    struct Line : public Primitive
    {
        Vector2S StartPos, EndPos;

        Line() : Primitive(PrimitiveType::Line)
        {}
    };

    struct Arc : public Primitive
    {
        Vector2S Pos;
        Fixed32 Radius;
        float StartAngle, SweepAngle;

        Arc() : Primitive(PrimitiveType::Arc)
        {}
    };

    struct Cutout
    {
        uint32_t Tag;
        uint32_t EdgeCount;
        std::vector<Vector2S> Vertices;
    };

    struct Surface : public Primitive
    {
        uint32_t EdgeCount;
        Fixed32 LineWidth;
        uint32_t VoidCount;
        std::vector<Vector2S> Vertices;
        std::vector<Cutout> Voids;
        uint32_t VoidFlags = 0;

        Surface() : Primitive(PrimitiveType::Surface)
        {
            DCode = 0;
        }
    };

    struct TvwHeader
    {
        std::string Type;
        uint32_t Const1; // = 1
        std::string Customer;
        uint8_t Const2; // = 0
        std::string Date;
        uint8_t Const3[3]; // = {0, 0, 0}
        uint32_t Size1;
        uint32_t Size2;
        uint32_t Size3;
        uint32_t LayerCount;

        void Load(StreamReader &r);
    };

    enum class ObjectType : uint32_t
    {
        Undefined = 0,
        Through = 1,
        Logic = 3
    };

    struct Object
    {
        ObjectType ObjType; // 3 or 1
        uint32_t Magic[2]; // 2, 1
        std::string Name;
        std::string InitialName;
        std::string InitialPath;
        LayerType Type;
        uint32_t PadColor;
        uint32_t LineColor;

        static ObjectType Detect(StreamReader &r);

        Object(ObjectType objType)
        {
            R_ASSERT(objType == ObjectType::Logic || objType == ObjectType::Through);
            ObjType = objType;
        }

        virtual ~Object() = default;

        virtual void Load(StreamReader &r);
    };

    struct TestPoint
    {
        bool Flag1;
        int32_t P1, Handle, P2;
        int32_t P3;
        Vector2S Pos;
        int32_t P4;
        bool Flag2;
        int32_t P5, P6;
        int32_t N;

        void Load(StreamReader &r);
    };

    struct TestPoint2
    {
        uint32_t P1, Handle, P2;
        Vector2S Pos;
        Vector2S Pos1, Pos2;
        bool Flag1, Flag2, Flag3;
        uint32_t Nail;
        int32_t Param;
        bool Flag4, Flag5, Flag6;
        int32_t N;

        void Load(StreamReader &r);
    };

    struct TestNode
    {
        uint32_t Current, Next;
        bool Flag;

        void Load(StreamReader &r);
    };

    struct UnknownItem
    {
        std::string Name;
        Vector2S Pos;
        int32_t Z1;
        int32_t Param1, Param2, Param3;
        int32_t Z2, Z3;
        bool Flags[3];
        uint32_t Param4;

        void Load(StreamReader &r);
    };

    struct LogicLayer : public Object
    {
        std::vector<std::unique_ptr<Shape>> Shapes;
        std::vector<Pad> Pads;
        std::vector<Line> Lines;
        std::vector<Arc> Arcs;
        std::vector<Surface> Surfaces;
        uint32_t UnknownItemCount;
        uint32_t UnknownItemsParam;
        std::vector<UnknownItem> UnknownItems;
        uint32_t TpCount;
        std::vector<TestPoint> TestPoints;
        uint32_t TPS2Size;
        uint32_t TPS2Param;
        std::vector<TestPoint2> TestPoints2; // first tps
        uint32_t TPS3Size;
        uint32_t TPS3Param;
        std::vector<TestPoint2> TestPoints3; // assistant tps
        uint32_t TestSequenceSize;
        uint32_t TestSequenceParam;
        std::vector<TestNode> TestSequence;

        LogicLayer() : Object(ObjectType::Logic)
        {}

        void LoadShapes(StreamReader &r);
        void LoadPads(StreamReader &r);
        void LoadLines(StreamReader &r);
        void LoadArcs(StreamReader &r);
        void LoadSurfaces(StreamReader &r);
        void LoadUnknownItems(StreamReader &r);
        void LoadTestpoints(StreamReader &r);
        virtual void Load(StreamReader &r) override;
    };

    struct ThroughLayer : public Object
    {
        struct Tool
        {
            bool Flag1, Flag2;
            Fixed32 Size;
            uint32_t Data5[5];
            uint8_t Data3[3]; // color?

            void Load(StreamReader &r);
        };
        std::vector<Tool> Tools;
        struct DrillHole
        {
            //uint8_t Code; // = 0x08
            int32_t Net;
            uint32_t Tool; // 1-based index
            Vector2S Pos;

            void Load(StreamReader &r);
        };
        std::vector<DrillHole> DrillHoles;
        struct DrillSlot
        {
            //uint8_t Code; // = 0x0A
            int32_t Net;
            uint32_t Tool;
            Vector2S Begin, End;
            uint32_t Zero; // = 0

            void Load(StreamReader &r);
        };
        std::vector<DrillSlot> DrillSlots;

        ThroughLayer() : Object(ObjectType::Through)
        {}
    
        virtual void Load(StreamReader &r) override;
    };
    
    struct ProbeBox32
    {
        int32_t Tag;
        Vector2S V1, V2;

        void Load(StreamReader &r);
    };

    struct DoubleBox32
    {
        uint32_t Tag;
        ProbeBox32 B1, B2;

        void Load(StreamReader &r);
    };

    struct ProbeBox8
    {
        int8_t Tag;
        int32_t N, A, P1, P2;

        void Load(StreamReader &r);
    };

    struct ProbeDataItem
    {
        bool Present = false;
        Fixed32 Size = 0;
        uint32_t Params[5] = {};
        uint32_t Color = 0;

        void Load(StreamReader &r);
    };

    struct FixtureData
    {
        uint32_t P1;
        uint32_t PX[6];
        bool Flags[3];
        std::vector<ProbeDataItem> Items;
        // u32 ProbeBox8_count
        uint32_t C1;
        Vector2S V1, V2;
        std::vector<ProbeBox8> Boxes;

        void Load(StreamReader &r);
    };

    struct ProbeData
    {
        FixtureData Fixture;
        Vector2S V3, V4;
        std::vector<DoubleBox32> Boxes2;

        void Load(StreamReader &r);
    };

    struct Probe
    {
        struct
        {
            bool Flag; // 01
            uint32_t Tag; // 15
            std::string Name; // "Spear_B_100 Mil"
            Fixed32 Size1; // 7000
            uint32_t Param1; // 0x0012CCFC
            Fixed32 Size2; // 2000
            uint32_t Param2; // 98
            Fixed32 Size3; // 3000
            uint32_t Param3; // 5299
            uint32_t Color; // 0x0000FFFF
            uint32_t K1, V1;
            uint32_t K2, V2;
            uint32_t K3, V3;
            uint32_t K4, V4;
        } Header;
        std::unique_ptr<ProbeData> Body;
        struct
        {
            uint32_t Tag;
            bool Flag1, Flag2, Flag3;
            uint8_t P0;
            int32_t P1, P2, P3;        
            ProbeBox32 B1, B2;
        } Tail;

        void Load(StreamReader &r);
    };

    using ProbePack = std::vector<Probe>;

    struct ProbeRegistry
    {
        uint32_t Z1, Z2; // 0, 0
        uint32_t Param; // 4
        std::string Name;
        Fixed32 DefaultSize; // 118.11 mil = 3 mm
        std::vector<ProbePack> Packs;

        void Load(StreamReader &r);
    };

    struct FixtureVariant
    {
        std::string Name;
        std::string ShortName;
        bool Flag1; // = 1
        bool Flag2; // = 0
        FixtureData Data;

        void Load(StreamReader &r);
    };

    struct FixtureSetting
    {
        uint32_t Tag;
        std::string Name;
        uint32_t Param;
        std::vector<FixtureVariant> Variants;
        Vector2S WorkspaceSize;

        void Load(StreamReader &r);
    };

    struct FixtureRegistry
    {
        uint32_t Tag1; // must be 0
        uint32_t Tag2; // must be 7874
        std::vector<std::string> Grids;
        FixtureSetting Top, Bottom;

        void Load(StreamReader &r);
    };

    struct Pin
    {
        uint32_t Handle;
        uint32_t Z1;
        uint32_t Id;
        std::string Name;
        uint32_t Z2;

        void Load(StreamReader &r);
    };

    enum class PartType : uint32_t
    {
        Chip = 0,
        Diode = 1,
        Transistor = 2,
        Resistor = 3,
        Unknown4 = 4,
        Capacitor = 5, // some fiducials (CFxx) go there for some reason
        Unknown6 = 6,
        Unknown7 = 7,
        Unknown8 = 8,
        Jumper = 9,
        Unknown10 = 10,
        Unknown11 = 11,
        Unknown12 = 12,
        Fuse = 13,
        Choke = 14,
        Oscillator = 15,
        Switch = 16,
        Connector = 17,
        Testpoint = 18, // transformers go there as well
        Unknown19 = 19,
        Unknown20 = 20,
        Mechanical = 21,
        Fiducial = 29, // FDxx
    };

    struct Part
    {
        std::string Name;
        Box2S Bbox;
        Vector2S Pos;
        int32_t Angle;
        uint32_t Decal;
        PartType Type;
        uint32_t Z1;
        Fixed32 Height;
        bool Flag0;
        std::string Value;
        std::string ToleranceP;
        std::string ToleranceN;
        std::string Desc;
        std::string Serial;
        uint32_t Z2 = 0;
        // uint32_t PinCount
        uint32_t Layer; // 3 / 12
        uint32_t P2; // 0
        std::vector<Pin> Pins;

        void Load(StreamReader &r);
    };

    struct MysteriousBlock
    {
        uint32_t P1, P2; // 50, 12452
        Vector2S TopRight;
        uint32_t P3, P4; // 3, 1
        bool Flag1, Flag2;
        uint8_t P5, P6;
        uint32_t P7x[4];
        bool Flags[6];
        uint32_t P8; // 10000
        uint32_t P9, P10, P11;
        uint8_t P12, P13;

        void Load(StreamReader &r);
    };

    struct Decal
    {
        bool Flag1;
        std::string Name;

        uint32_t HeaderParams[3];
        bool Flag;
        std::array<std::unique_ptr<Object>, 3> Layers;
    
        bool OutlineFlag;
        uint32_t Param; // 2
        int32_t N1; // -1
        uint32_t OutlineVertexCount; // 8
        std::vector<Vector2S> Outline;
        uint32_t Params[2]; // 0, 0

        void Load(StreamReader &r);
    };

    class Board
    {
    public:
        TvwHeader Header;
        std::vector<std::unique_ptr<Object>> Layers;
        std::vector<std::string> Nets;
        ProbeRegistry Probes;
        FixtureRegistry Fixtures;
        MysteriousBlock Myb;
        std::vector<Part> Parts;
        std::vector<Decal> Decals;

    private:
        void ReadNetList(StreamReader &r);
        void ReadParts(StreamReader &r);
        void ReadDecals(StreamReader &r);

    public:
        void Read(std::istream &fs);
    };
} // namespace Tebo
