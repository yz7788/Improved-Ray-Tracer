#define _USE_MATH_DEFINES
#include <iostream>
#include <algorithm>
#include <cmath>
#include "geometry.h"

float length(const Vec3& a) {
	return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

//calculate the color at intersection point using Phong Shading
Vec3 phongShading(const Vec3 ks, const Vec3 kd, const Vec3 normal, const Vec3 intersectionPoint, const float shi, const Vec3& lightPosition, const Vec3& lightColor) {
	Vec3 unitVectortoLight = (lightPosition - intersectionPoint);
	unitVectortoLight.normalize();

	float d = std::max(unitVectortoLight * normal, 0.0f);
	Vec3 unitVectortoCamera = (Vec3(0, 0, 0) - intersectionPoint);
	unitVectortoCamera.normalize();
	Vec3 reflectDir = normal * (unitVectortoLight * normal) * 2 - unitVectortoLight;
	float s = std::max(reflectDir * unitVectortoCamera, 0.0f);

	return Vec3(lightColor.x * (kd.x * d + ks.x * pow(s, shi)), lightColor.y * (kd.y * d + ks.y * pow(s, shi)), lightColor.z * (kd.z * d + ks.z * pow(s, shi)));
}

//test whether the ray starting from (x0, y0, z0) with unit direction (xd, yd, zd) can intersect with a sphere (of sphere_index)
//if intersect, return the intersection Vec3; else, return Vec3(0,0,0)
Vec3 calcSphereIntersection(const Vec3& p0, const Vec3& direction, const int& sphere_index, Sphere* spheres) {
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
Vec3 calcTriIntersection(const Vec3& p0, const Vec3& direction, const int& tri_index, Triangle* triangles) {
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

void calculateScene(unsigned char* pixels,const int& w, const int& h, const int& subSample, const int& subLight, Triangle* triangles, Sphere* spheres, Light* lights, float* ambient_light, const int num_triangles, const int num_spheres, const int num_lights)
{
	// add divided light for soft shadow
	Vec3* lightColorBuffer = new Vec3[num_lights];
	Vec3* lightPosBuffer = new Vec3[num_lights * pow(subLight,3)];

	int newNumLights = num_lights * pow(subLight, 3);
	int midIndex = (int)subLight / 2;

	float delta = 0.05;
	int lightIndex = 0;
	for (int n = 0; n < num_lights; n++) {
		lightColorBuffer[n] = Vec3(lights[n].color[0], lights[n].color[1], lights[n].color[2]) / (float)pow(subLight, 3);
		for (int i = 0; i < subLight; i++) {
			for (int j = 0; j < subLight; j++) {
				for (int k = 0; k < subLight; k++) {
					lightPosBuffer[lightIndex] = Vec3(lights[n].position[0] + (i - midIndex) * delta, lights[n].position[1] + (j - midIndex) * pow(-1, j) * delta, lights[n].position[2] + (k - midIndex) * pow(-1, k) * delta);
					lightIndex++;
				}
			}
		}
	}

	int width = subSample * w;
	int height = subSample * h;

	Vec3* colorbuffer = new Vec3[h * subSample * subSample];
	for (unsigned int x = 0; x < width; x++)
	{
		for (unsigned int y = 0; y < height; y++)
		{
			Vec3 intersectionPoint;
			Vec3 newIntersectionPoint;
			Vec3 color;
			Vec3 backgroundColor = Vec3(1.0, 1.0, 1.0);

			int intersectTri = num_triangles;
			int intersectSphere = num_spheres;

			float a = (float)width / (float)height;
			float imageplaneX = a * x * (2 * tan(M_PI / 3 / 2)) / width - a * tan(M_PI / 3 / 2);
			float imageplaneY = y * (2 * tan(M_PI / 3 / 2)) / height - tan(M_PI / 3 / 2);

			// test whether the ray intersect with triangles or spheres
			// if yes, store the nearest intersection point
			Vec3 unitViewRay = Vec3(imageplaneX, imageplaneY, -1);
			unitViewRay.normalize();

			// calculate the nearest view ray intersection point with objects, store the value in 'intersectionPoint'
			// if there is no intersection, intersectionPoint is (0, 0, 0)
			for (int i = 0; i < num_triangles; i++) {
				newIntersectionPoint = calcTriIntersection(Vec3(0, 0, 0), unitViewRay, i, triangles);
				if (newIntersectionPoint.z != 0 && newIntersectionPoint.z > intersectionPoint.z || intersectionPoint.z == 0) {
					intersectionPoint = newIntersectionPoint;
					intersectTri = i;
				}
			}
			for (int i = 0; i < num_spheres; i++) {
				newIntersectionPoint = calcSphereIntersection(Vec3(0, 0, 0), unitViewRay, i, spheres);
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
							Vec3 shadowIntersection = calcTriIntersection(intersectionPoint, unitLightDirection, j, triangles);
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
								Vec3 shadowIntersection = calcSphereIntersection(intersectionPoint, unitLightDirection, j, spheres);
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

							int lightColorIndex = (int)i / pow(subLight, 3);
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
							Vec3 shadowIntersection = calcTriIntersection(intersectionPoint, unitLightDirection, j, triangles);
							if (shadowIntersection.z != 0) {
								if ((lightPosition - shadowIntersection) * (intersectionPoint - shadowIntersection) < 0) {
									shadow = true;
									break;
								}
							}
						}

						if (shadow == 0) {
							for (int j = 0; j < num_spheres; j++) {
								Vec3 shadowIntersection = calcSphereIntersection(intersectionPoint, unitLightDirection, j, spheres);
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
							int lightColorIndex = (int)i / pow(subLight, 3);
							color = color + phongShading(ks, kd, normal, intersectionPoint, spheres[intersectSphere].shininess, lightPosBuffer[i], lightColorBuffer[lightColorIndex]);
						}
					}
				}

				color = color + Vec3(ambient_light[0], ambient_light[1], ambient_light[2]);

			}
			else color = backgroundColor;

			// AntiAliasing
			// store color in colorbuffer for anti-aliasing
			colorbuffer[height * (x % subSample) + y] = Vec3(color.x, color.y, color.z);

			// when the divided pixels are all calculated, paint the pixel as their average color
			if (x % subSample == subSample - 1 && y % subSample == subSample - 1) {
				Vec3 sumColor;
				for (int i = 0; i < subSample; i++) {
					for (int j = 0; j < subSample; j++) {
						sumColor = sumColor + colorbuffer[y - i + height * j];
					}
				}
				Vec3 finalColor = sumColor / pow(subSample, 2);

				//pixels[3 * (y * width + x)] = std::min(1.0f, finalColor.z) * 255.0;
				//pixels[3 * (y * width + x) + 1] = std::min(1.0f, finalColor.y) * 255.0;
				//pixels[3 * (y * width + x) + 2] = std::min(1.0f, finalColor.x) * 255.0;
				pixels[3 * (int(y/subSample) * w + int(x/subSample))] = std::min(1.0f, finalColor.z) * 255.0;
				pixels[3 * (int(y / subSample) * w + int(x / subSample)) + 1] = std::min(1.0f, finalColor.y) * 255.0;
				pixels[3 * (int(y / subSample) * w + int(x / subSample)) + 2] = std::min(1.0f, finalColor.x) * 255.0;
			}
		}
	}
}
