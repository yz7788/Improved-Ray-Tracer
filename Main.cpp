//#include "stdafx.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define strcasecmp _stricmp

using namespace cv;
using namespace std;

#define WIDTH 640
#define HEIGHT 480

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

struct Vertex
{
	float position[3];
	float color_diffuse[3];
	float color_specular[3];
	float normal[3];
	float shininess;
};

struct Triangle
{
	Vertex v[3];
};

struct Sphere
{
	float position[3];
	float color_diffuse[3];
	float color_specular[3];
	float shininess;
	float radius;
};

struct Light
{
	float position[3];
	float color[3];
};


class Vec3 {
public:
	float x, y, z;

	Vec3() { x = y = z = 0.0; }

	Vec3(const Vec3& a) : x(a.x), y(a.y), z(a.z) {}

	Vec3(const float x, const float y, const float z) : x(x), y(y), z(z) {}

	Vec3 operator+(const Vec3& a) const { return Vec3(x + a.x, y + a.y, z + a.z); }

	Vec3 operator-(const Vec3& a) const { return Vec3(x - a.x, y - a.y, z - a.z); }

	float operator*(const Vec3& a) const { return x * a.x + y * a.y + z * a.z; }

	Vec3 operator*(const float a) const { return Vec3(x * a, y * a, z * a); }

	Vec3 operator^(const Vec3& a) const { return Vec3(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x); }

	Vec3 operator/(const float a) const { return Vec3(x / a, y / a, z / a); }

	void normalize() {
		float length = x * x + y * y + z * z;
		if (length > 0.0) {
			float oneOverLength = 1.0f / sqrt(length);
			x *= oneOverLength;
			y *= oneOverLength;
			z *= oneOverLength;
		}
	}
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
float ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

inline float length(const Vec3& a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

#pragma region LoadScene

void parse_check(const char* expected, char* found)
{
	if (strcasecmp(expected, found))
	{
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}
}

void parse_floats(FILE* file, const char* check, float p[3])
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%f %f %f", &p[0], &p[1], &p[2]);
	printf("%s %f %f %f\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE* file, float* r)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%f", r);
	printf("rad: %f\n", *r);
}

void parse_shi(FILE* file, float* shi)
{
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%f", shi);
	printf("shi: %f\n", *shi);
}

int loadScene(char* argv)
{
	FILE* filepoint;
	FILE* file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);

	parse_floats(file, "amb:", ambient_light);

	for (int i = 0; i < number_of_objects; i++)
	{
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (strcasecmp(type, "triangle") == 0)
		{
			printf("found triangle\n");
			for (int j = 0; j < 3; j++)
			{
				parse_floats(file, "pos:", t.v[j].position);
				parse_floats(file, "nor:", t.v[j].normal);
				parse_floats(file, "dif:", t.v[j].color_diffuse);
				parse_floats(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (num_triangles == MAX_TRIANGLES)
			{
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = t;
		}
		else if (strcasecmp(type, "sphere") == 0)
		{
			printf("found sphere\n");

			parse_floats(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_floats(file, "dif:", s.color_diffuse);
			parse_floats(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (num_spheres == MAX_SPHERES)
			{
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = s;
		}
		else if (strcasecmp(type, "light") == 0)
		{
			printf("found light\n");
			parse_floats(file, "pos:", l.position);
			parse_floats(file, "col:", l.color);

			if (num_lights == MAX_LIGHTS)
			{
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights[num_lights++] = l;
		}
		else
		{
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	return 0;
}


#pragma endregion

#pragma region CalculateScene

//calculate the color at intersection point using Phong Shading
Vec3 phongShading(const Vec3 ks, const Vec3 kd, const Vec3 normal, const Vec3 intersectionPoint, const float shi, const Vec3& lightPosition, const Vec3& lightColor) {
	Vec3 unitVectortoLight = (lightPosition - intersectionPoint);
	unitVectortoLight.normalize();

	float d = max(unitVectortoLight * normal, 0.0f);
	Vec3 unitVectortoCamera = (Vec3(0, 0, 0) - intersectionPoint);
	unitVectortoCamera.normalize();
	Vec3 reflectDir = normal * (unitVectortoLight * normal) * 2 - unitVectortoLight;
	float s = max(reflectDir * unitVectortoCamera, 0.0f);

	return Vec3(lightColor.x * (kd.x * d + ks.x * pow(s, shi)), lightColor.y * (kd.y * d + ks.y * pow(s, shi)), lightColor.z * (kd.z * d + ks.z * pow(s, shi)));
}

//test whether the ray starting from (x0, y0, z0) with unit direction (xd, yd, zd) can intersect with a sphere (of sphere_index)
//if intersect, return the intersection Vec3; else, return Vec3(0,0,0)
inline Vec3 calculateSphereIntersection(const Vec3& p0, const Vec3& direction, const int& sphere_index) {
	float xc = spheres[sphere_index].position[0];
	float yc = spheres[sphere_index].position[1];
	float zc = spheres[sphere_index].position[2];
	float radius = spheres[sphere_index].radius;

	float b = 2 * (direction.x * (p0.x - xc) + direction.y * (p0.y - yc) + direction.z * (p0.z - zc));
	float c = (p0.x - xc) * (p0.x - xc) + (p0.y - yc) * (p0.y - yc) + (p0.z - zc) * (p0.z - zc) - radius * radius;

	if (b * b >= 4 * c) {
		float t_0 = (-b - sqrt(b * b - 4 * c)) / 2;
		float t_1 = (-b + sqrt(b * b - 4 * c)) / 2;

		if (t_0 > 0.001) return Vec3(p0.x + direction.x * t_0, p0.y + direction.y * t_0, p0.z + direction.z * t_0);
		else if (t_1 > 0.001) return Vec3(p0.x + direction.x * t_1, p0.y + direction.y * t_1, p0.z + direction.z * t_1);
	}

	return Vec3(0, 0, 0);
}

//test whether the ray starting from (x0, y0, z0) with unit direction (xd, yd, zd) can intersect with a triangle (of tri_index)
//if intersect, return the intersection Vec3; else, return Vec3(0,0,0)
inline Vec3 calculateTriIntersection(const Vec3& p0, const Vec3& direction, const int& tri_index) {
	Vec3 C0 = Vec3(triangles[tri_index].v[0].position[0], triangles[tri_index].v[0].position[1], triangles[tri_index].v[0].position[2]);
	Vec3 C1 = Vec3(triangles[tri_index].v[1].position[0], triangles[tri_index].v[1].position[1], triangles[tri_index].v[1].position[2]);
	Vec3 C2 = Vec3(triangles[tri_index].v[2].position[0], triangles[tri_index].v[2].position[1], triangles[tri_index].v[2].position[2]);

	Vec3 normal = ((C0 - C1) ^ (C1 - C2));
	normal.normalize();

	//calculate the intersection Vec3 of ray with plane containing triangle
	float d = -(normal.x * C0.x + normal.y * C0.y + normal.z * C0.z);
	float t = -(normal.x * p0.x + normal.y * p0.y + normal.z * p0.z + d) / (normal.x * direction.x + normal.y * direction.y + normal.z * direction.z);

	if (t > 0.001) {
		Vec3 C = Vec3(p0.x + direction.x * t, p0.y + direction.y * t, p0.z + direction.z * t);

		float totalArea = 1.0f / 2.0f * length((C1 - C0) ^ (C2 - C0));

		if (abs(1.0 / 2.0 * length((C1 - C) ^ (C2 - C)) / totalArea + 1.0 / 2.0 * length((C - C0) ^ (C2 - C0)) / totalArea + 1.0 / 2.0 * length((C1 - C0) ^ (C - C0)) / totalArea - 1) < 0.001) return C;
	}
	return Vec3(0, 0, 0);
}

void calculateScene(vector<vector<vector<int>>>& pixels)
{
	int dividedLightNumber = 1;
	int subSampleSize = 1;

	// add divided light for soft shadow
	Vec3* lightColorBuffer = new Vec3[num_lights];
	Vec3* lightPosBuffer = new Vec3[num_lights * dividedLightNumber * dividedLightNumber * dividedLightNumber];

	int newNumLights = num_lights * dividedLightNumber * dividedLightNumber * dividedLightNumber;
	int midIndex = (int)dividedLightNumber / 2;

	float delta = 0.05;
	int lightIndex = 0;
	for (int n = 0; n < num_lights; n++) {
		lightColorBuffer[n] = Vec3(lights[n].color[0], lights[n].color[1], lights[n].color[2]) / (float)dividedLightNumber / (float)dividedLightNumber / (float)dividedLightNumber;
		for (int i = 0; i < dividedLightNumber; i++) {
			for (int j = 0; j < dividedLightNumber; j++) {
				for (int k = 0; k < dividedLightNumber; k++) {
					lightPosBuffer[lightIndex] = Vec3(lights[n].position[0] + (i - midIndex) * delta, lights[n].position[1] + (j - midIndex) * pow(-1, j) * delta, lights[n].position[2] + (k - midIndex) * pow(-1, k) * delta);
					lightIndex++;
				}
			}
		}
	}

	int width = subSampleSize * WIDTH;
	int height = subSampleSize * HEIGHT;

	Vec3* colorbuffer = new Vec3[HEIGHT * subSampleSize * subSampleSize];
	for (unsigned int x = 0; x < width; x++)
	{
		pixels[x].resize(height);
		for (unsigned int y = 0; y < height; y++)
		{
			Vec3 intersectionPoint;
			Vec3 newIntersectionPoint;
			Vec3 color;
			Vec3 backgroundColor = Vec3(1.0, 1.0, 1.0);

			int intersectTri = num_triangles;
			int intersectSphere = num_spheres;

			float a = (float)width / (float)height;
			float imageplaneX = a * x * (2 * tan(3.1415926 / 3 / 2)) / width - a * tan(3.1415926 / 3 / 2);
			float imageplaneY = y * (2 * tan(3.1415926 / 3 / 2)) / height - tan(3.1415926 / 3 / 2);

			// test whether the ray intersect with triangles or spheres
			// if yes, store the nearest intersection point
			Vec3 unitViewRay = Vec3(imageplaneX, imageplaneY, -1);
			unitViewRay.normalize();

			// calculate the nearest view ray intersection point with objects, store the value in 'intersectionPoint'
			// if there is no intersection, intersectionPoint is (0, 0, 0)
			for (int i = 0; i < num_triangles; i++) {
				newIntersectionPoint = calculateTriIntersection(Vec3(0, 0, 0), unitViewRay, i);
				if (newIntersectionPoint.z != 0 && newIntersectionPoint.z > intersectionPoint.z || intersectionPoint.z == 0) {
					intersectionPoint = newIntersectionPoint;
					intersectTri = i;
				}
			}
			for (int i = 0; i < num_spheres; i++) {
				newIntersectionPoint = calculateSphereIntersection(Vec3(0, 0, 0), unitViewRay, i);
				if (newIntersectionPoint.z != 0 && newIntersectionPoint.z > intersectionPoint.z || intersectionPoint.z == 0) {
					intersectionPoint = newIntersectionPoint;
					intersectSphere = i;
					intersectTri = num_triangles;
				}
			}
			
			//if there is an intersection, calculate color of the intersection point
			if (intersectionPoint.z != 0)
			{
				//the intersection point is of a triangle
				if (intersectTri != num_triangles) {
					// calculate color for point(x, y) on image plane
					for (int i = 0; i < newNumLights; i++) {
						bool shadow = false;

						Vec3 lightPosition = lightPosBuffer[i];
						Vec3 unitLightDirection = (lightPosition - intersectionPoint);
						unitLightDirection.normalize();

						// test whether the shadow ray is blocked by a triangle
						for (int j = 0; j < num_triangles; j++) {
							Vec3 shadowIntersection = calculateTriIntersection(intersectionPoint, unitLightDirection, j);
							if (shadowIntersection.z != 0) {
								if ((lightPosition - shadowIntersection) * (intersectionPoint - shadowIntersection) < 0) {
									shadow = true;
									break;
								}
							}
						}

						// test whether the ray  the shadow ray is blocked by a sphere
						if (!shadow) {
							for (int j = 0; j < num_spheres; j++) {
								Vec3 shadowIntersection = calculateSphereIntersection(intersectionPoint, unitLightDirection, j);
								if (shadowIntersection.z != 0 || (lightPosition - shadowIntersection) * (intersectionPoint - shadowIntersection) < 0) {
									shadow = true;
									break;
								}
							}
						}

						// if not in shadow, apply phong shading
						if (!shadow) {
							// triangle interpolation
							Vec3 C0 = Vec3(triangles[intersectTri].v[0].position[0], triangles[intersectTri].v[0].position[1], triangles[intersectTri].v[0].position[2]);
							Vec3 C1 = Vec3(triangles[intersectTri].v[1].position[0], triangles[intersectTri].v[1].position[1], triangles[intersectTri].v[1].position[2]);
							Vec3 C2 = Vec3(triangles[intersectTri].v[2].position[0], triangles[intersectTri].v[2].position[1], triangles[intersectTri].v[2].position[2]);

							float totalArea = 1.0 / 2.0 * length((C1 - C0) ^ (C2 - C0));
							float alpha = 1.0 / 2.0 * length((C1 - intersectionPoint) ^ (C2 - intersectionPoint)) / totalArea;
							float beta = 1.0 / 2.0 * length((intersectionPoint - C0) ^ (C2 - C0)) / totalArea;
							float gamma = 1.0 / 2.0 * length((C1 - C0) ^ (intersectionPoint - C0)) / totalArea;

							Vec3 n0 = Vec3(triangles[intersectTri].v[0].normal[0], triangles[intersectTri].v[0].normal[1], triangles[intersectTri].v[0].normal[2]);
							Vec3 n1 = Vec3(triangles[intersectTri].v[1].normal[0], triangles[intersectTri].v[1].normal[1], triangles[intersectTri].v[1].normal[2]);
							Vec3 n2 = Vec3(triangles[intersectTri].v[2].normal[0], triangles[intersectTri].v[2].normal[1], triangles[intersectTri].v[2].normal[2]);

							Vec3 normal = n0 * alpha + n1 * beta + n2 * gamma;
							normal.normalize();

							Vec3 d0 = Vec3(triangles[intersectTri].v[0].color_diffuse[0], triangles[intersectTri].v[0].color_diffuse[1], triangles[intersectTri].v[0].color_diffuse[2]);
							Vec3 d1 = Vec3(triangles[intersectTri].v[1].color_diffuse[0], triangles[intersectTri].v[1].color_diffuse[1], triangles[intersectTri].v[1].color_diffuse[2]);
							Vec3 d2 = Vec3(triangles[intersectTri].v[2].color_diffuse[0], triangles[intersectTri].v[2].color_diffuse[1], triangles[intersectTri].v[2].color_diffuse[2]);

							Vec3 kd = d0 * alpha + d1 * beta + d2 * gamma;

							Vec3 s0 = Vec3(triangles[intersectTri].v[0].color_specular[0], triangles[intersectTri].v[0].color_specular[1], triangles[intersectTri].v[0].color_specular[2]);
							Vec3 s1 = Vec3(triangles[intersectTri].v[1].color_specular[0], triangles[intersectTri].v[1].color_specular[1], triangles[intersectTri].v[1].color_specular[2]);
							Vec3 s2 = Vec3(triangles[intersectTri].v[2].color_specular[0], triangles[intersectTri].v[2].color_specular[1], triangles[intersectTri].v[2].color_specular[2]);

							Vec3 ks = s0 * alpha + s1 * beta + s2 * gamma;

							float shi = triangles[intersectTri].v[0].shininess * alpha + triangles[intersectTri].v[1].shininess * beta + triangles[intersectTri].v[2].shininess * gamma;

							int lightColorIndex = (int)i / dividedLightNumber / dividedLightNumber / dividedLightNumber;
							color = color + phongShading(ks, kd, normal, intersectionPoint, shi, lightPosBuffer[i], lightColorBuffer[lightColorIndex]);
						}
					}
				}

				// the intersection point is of a sphere
				if (intersectSphere != num_spheres) {
					Vec3 sphereCenter = Vec3(spheres[intersectSphere].position[0], spheres[intersectSphere].position[1], spheres[intersectSphere].position[2]);

					Vec3 normal = intersectionPoint - sphereCenter;
					normal.normalize();

					Vec3 kd = Vec3(spheres[intersectSphere].color_diffuse[0], spheres[intersectSphere].color_diffuse[1], spheres[intersectSphere].color_diffuse[2]);
					Vec3 ks = Vec3(spheres[intersectSphere].color_specular[0], spheres[intersectSphere].color_specular[1], spheres[intersectSphere].color_specular[2]);

					for (int i = 0; i < newNumLights; i++) {
						bool shadow = false;
						Vec3 lightPosition = lightPosBuffer[i];
						Vec3 unitLightDirection = lightPosition - intersectionPoint;
						unitLightDirection.normalize();

						for (int j = 0; j < num_triangles; j++) {
							Vec3 shadowIntersection = calculateTriIntersection(intersectionPoint, unitLightDirection, j);
							if (shadowIntersection.z != 0) {
								if ((lightPosition - shadowIntersection) * (intersectionPoint - shadowIntersection) < 0) {
									shadow = true;
									break;
								}
							}
						}

						if (shadow == 0) {
							for (int j = 0; j < num_spheres; j++) {
								Vec3 shadowIntersection = calculateSphereIntersection(intersectionPoint, unitLightDirection, j);
								if (shadowIntersection.z != 0) {
									if ((lightPosition - shadowIntersection) * (intersectionPoint - shadowIntersection) < 0) {
										shadow = true;
										break;
									}
								}
							}
						}

						// if not in shadow, apply phong shading
						if (shadow == 0) {
							int lightColorIndex = (int)i / dividedLightNumber / dividedLightNumber / dividedLightNumber;
							color = color + phongShading(ks, kd, normal, intersectionPoint, spheres[intersectSphere].shininess, lightPosBuffer[i], lightColorBuffer[lightColorIndex]);
						}
					}
				}

				color = color + Vec3(ambient_light[0], ambient_light[1], ambient_light[2]);
		
			}
			else color = backgroundColor;

			// AntiAliasing
			// store color in colorbuffer for anti-aliasing
			colorbuffer[height * (x % subSampleSize) + y] = Vec3(color.x, color.y, color.z);

			// when the divided pixels are all calculated, paint the pixel as their average color
			if (x % subSampleSize == subSampleSize - 1 && y % subSampleSize == subSampleSize - 1) {
				Vec3 sumColor;
				for (int i = 0; i < subSampleSize; i++) {
					for (int j = 0; j < subSampleSize; j++) {
						sumColor = sumColor + colorbuffer[y - i + height * j];
					}
				}
				Vec3 finalColor = sumColor / subSampleSize / subSampleSize;


				pixels[x][y].push_back(min(1.0f, finalColor.z) * 255.0);
				pixels[x][y].push_back(min(1.0f, finalColor.y) * 255.0);
				pixels[x][y].push_back(min(1.0f, finalColor.x) * 255.0);
			}
		}
	}
}

#pragma endregion

int display(vector<vector<vector<int>>>& pixels)
{
	if (pixels.size() == 0 || pixels[0].size() == 0)
	{
		cout << "No image data to be displayed" << endl;
		return -1;
	}

	unsigned int width = pixels.size();
	unsigned int height = pixels[0].size();

	Mat M(height, width, CV_8UC3);
	for (unsigned int i = 0; i < height; ++i) {
		uchar* p = M.ptr<uchar>(i);// get the pointer of the row first
		for (unsigned int j = 0; j < width; ++j)
		{	
			p[3 * j] = pixels[j][height - i - 1][0];
			p[3 * j + 1] = pixels[j][height - i - 1][1];
			p[3 * j + 2] = pixels[j][height - i - 1][2];
			
		}
	}
	imshow("ray tracer", M);

	return 0;
}

int main(int argc, char** argv)
{
	loadScene(argv[1]);

	vector<vector<vector<int>>> pixels(WIDTH, vector<vector<int>>());

	calculateScene(pixels);
	
	display(pixels);

	waitKey(0);
}
