#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geometry.h"
#define strcasecmp _stricmp

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

int loadScene(char* argv, Triangle* triangles, Sphere* spheres, Light* lights, float* ambient_light, int& num_triangles, int& num_spheres, int& num_lights)
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
