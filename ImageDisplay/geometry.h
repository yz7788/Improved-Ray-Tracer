#pragma once

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
