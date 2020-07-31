#include <stdio.h>
#include "geometry.h"

void parse_check(const char* expected, char* found);

void parse_floats(FILE* file, const char* check, float p[3]);

void parse_rad(FILE* file, float* r);

void parse_shi(FILE* file, float* shi);

int loadScene(char* argv, Triangle* triangles, Sphere* spheres, Light* lights, float* ambient_light, int& num_triangles, int& num_spheres, int& num_lights);