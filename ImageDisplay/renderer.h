#include "geometry.h"

Vec3 phongShading(const Vec3 ks, const Vec3 kd, const Vec3 normal, const Vec3 intersectionPoint, const float shi, const Vec3& lightPosition, const Vec3& lightColor);

Vec3 calcSphereIntersection(const Vec3& p0, const Vec3& direction, const int& sphere_index, Sphere* spheres);

Vec3 calcTriIntersection(const Vec3& p0, const Vec3& direction, const int& tri_index, Triangle* triangles);

void calculateScene(unsigned char* pixels, const int& w, const int& h, const int& subsampleSize, const int& dividedLightNumber, Triangle* triangles, Sphere* spheres, Light* lights, float* ambient_light, const int num_triangles, const int num_spheres, const int num_lights);