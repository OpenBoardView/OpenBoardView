#ifndef VECTORHULLS
#define VECTORHULLS

void VHRotateV(double *px, double *py, double ox, double oy, double theta);
ImVec2 VHRotateV(ImVec2 v, double theta);
ImVec2 VHRotateV(ImVec2 v, ImVec2 o, double theta);
double VHAngleToX(ImVec2 a, ImVec2 b);
int VHConvexHullOrientation(ImVec2 p, ImVec2 q, ImVec2 r);
int VHConvexHull(ImVec2 hull[], ImVec2 points[], int n);
int VHTightenHull(ImVec2 hull[], int n, double threshold);
void VHMBBCalculate(ImVec2 box[], ImVec2 *hull, int n, double psz);

#endif
