#include "imgui/imgui.h"

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
std::basic_ostream<C, T> & operator<<(std::basic_ostream<C, T> & os, ImVec2 const & v) {
	os << v.x << "," << v.y;
	return os;
}
