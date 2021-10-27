#pragma once
#include "imgui/imgui.h"
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


template <typename C, typename T>
inline std::basic_ostream<C, T> & operator<<(std::basic_ostream<C, T> & os, ImVec2 const & v) {
	os << v.x << "," << v.y;
	return os;
}

inline ImVec2 abs(ImVec2 const & v) {
	return { abs(v.x), abs(v.y) };
}
