#pragma once
#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui/imgui_internal.h" // math ops


inline bool operator>(ImVec2 const & a, ImVec2 const & b) {
	return a.x > b.x && a.y > b.y;
}
inline bool operator>=(ImVec2 const & a, ImVec2 const & b) {
	return a.x >= b.x && a.y >= b.y;
}
inline bool operator<(ImVec2 const & a, ImVec2 const & b) {
	return a.x < b.x && a.y < b.y;
}
inline bool operator<=(ImVec2 const & a, ImVec2 const & b) {
	return a.x <= b.x && a.y <= b.y;
}

inline bool operator==(ImVec2 const & a, ImVec2 const & b) {
	return a.x == b.x && a.y == b.y;
}
inline bool operator!=(ImVec2 const & a, ImVec2 const & b) {
	return a.x != b.x || a.y != b.y;
}

#if 0
inline ImVec2 operator-(ImVec2 const & a, ImVec2 const & b) {
	return { a.x - b.x, a.y - b.y };
}
inline ImVec2 operator+(ImVec2 const & a, ImVec2 const & b) {
	return { a.x + b.x, a.y + b.y };
}

inline ImVec2 operator/(ImVec2 const & a, float b) {
	return { a.x / b, a.y / b };
}
#endif

template <typename C, typename T>
inline std::basic_ostream<C, T> & operator<<(std::basic_ostream<C, T> & os, ImVec2 const & v) {
	os << v.x << "," << v.y;
	return os;
}

inline ImVec2 abs(ImVec2 const & v) {
	return { abs(v.x), abs(v.y) };
}
