#ifndef VECTORHULLS
#define VECTORHULLS

#include <array>
#include <vector>

void VHRotateV(double *px, double *py, double ox, double oy, double theta);
ImVec2 VHRotateV(ImVec2 v, double theta);
ImVec2 VHRotateV(ImVec2 v, ImVec2 o, double theta);
double VHAngleToX(ImVec2 a, ImVec2 b);
int VHConvexHullOrientation(ImVec2 p, ImVec2 q, ImVec2 r);
std::vector<ImVec2> VHConvexHull(const std::vector<ImVec2> &points);
int VHTightenHull(ImVec2 hull[], int n, double threshold);
std::array<ImVec2, 4> VHMBBCalculate(std::vector<ImVec2> hull, double psz);

bool GetIntersection(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, ImVec2 *i);

#endif
