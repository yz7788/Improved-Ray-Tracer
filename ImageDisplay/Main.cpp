//#include "stdafx.h"
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/core/mat.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry.h"
#include "loadScene.h"
#include "renderer.h"

using namespace cv;
using namespace std;

#define WIDTH 640
#define HEIGHT 480
#define SUBSAMPLE 1
#define SUBLIGHT 1

int display(unsigned char* pixels, const int& height, const int& width)
{
	if (pixels == NULL)
	{
		cout << "No image data to be displayed" << endl;
		return -1;
	}

	Mat M(height, width, CV_8UC3, (void*)pixels);

	// in ray tracing render, the x,y of the pixels is the xy coordinate system, so (0,0) is at the bottom left
	// while in OpenCV Matrix, the matrix data is read and shown starting from the top left, in this way (0, 0) is at the top left
	// So we need to reverse the image upside down
	flip(M, M, 0);

	imshow("ray tracer", M);

	return 0;
}

int main(int argc, char** argv)
{
	int num_triangles = 0;
	int num_spheres = 0;
	int num_lights = 0;

	Triangle* triangles = (Triangle*)malloc(MAX_TRIANGLES * sizeof(Triangle));
	Sphere* spheres = (Sphere*)malloc(MAX_SPHERES * sizeof(Sphere));
	Light* lights = (Light*)malloc(MAX_LIGHTS * sizeof(Light));
	float* ambient_light = (float*)malloc(3 * sizeof(float));

	loadScene(argv[1], triangles, spheres, lights, ambient_light, num_triangles, num_spheres, num_lights);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(unsigned char) * 3 * WIDTH * HEIGHT);

	calculateScene(pixels, WIDTH, HEIGHT, SUBSAMPLE, SUBLIGHT, triangles, spheres, lights, ambient_light, num_triangles, num_spheres, num_lights);

	display(pixels, HEIGHT, WIDTH);

	free(triangles);
	free(spheres);
	free(lights);
	free(ambient_light);
	free(pixels);

	waitKey(0);
}