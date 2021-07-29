// Program rysuje obraz barwnej sfery oświetlonej przez jedno barwne źródło światła.
// Środek sfery znajduje się w środku układu współrzędnych.
// Do obliczania oświetlenia wykorzystywany jest model Phonga.
/*************************************************************************************/
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <GL/glut.h>

/*************************************************************************************/
// Stałe
/*************************************************************************************/
const int MAX = 4;
const float A = 1.0f;
const float B = 0.01f;
const float C = 0.001f;

struct Sphere
{
	float radius;
	float position[3];
	float specular[3];
	float diffuse[3];
	float ambient[3];
	float specularshinines;
};

struct Source
{
	float position[3];
	float specular[3];
	float diffuse[3];
	float ambient[3];
};

std::vector<Sphere> spheres;
std::vector<Source> sources;

/*************************************************************************************/
// Zmienne globalne
/*************************************************************************************/

int dimensions[2]; // rozmiar obrazu
float background[3]; // kolor tła
float global[3]; // globalne światło rozproszone
float starting_z = 20.0f; // rozmiar okna obserwatora

// Parametry "śledzonego" promienia
float starting_point[3]; // punkt, z którego wychodzi promień
float starting_directions[3] = { 0.0f, 0.0f, -1.0f }; // wektor opisujący kierunek promienia

// Zmienne pomocnicze
float inter[3]; // współrzędne (x,y,z) punktu przecięcia promienia i sfery
float inters_c[3]; // składowe koloru dla oświetlonego punktu na powierzchni sfery
GLubyte pixel[1][1][3]; // składowe koloru rysowanego piksela
float vec_n[3];
float ref[3];
float color[3];

/*************************************************************************************/
// Prototypy używanych funkcji
/*************************************************************************************/

// Funkcja obliczająca iloczyn skalarny dwóch wektorów
float dotProduct(float* p1, float* p2);

// Funkcja normalizująca wektor
void normal(float* p);
void normal(Sphere* sp);

void trace(float* p, float* v, int step);

// Funkcja obliczająca oświetlenie punktu na powierzchni sfery według modelu Phonga 
void phong(float* v, Sphere* sp);

// Funkcja obliczająca punkt przecięcia promienia i powierzchni sfery
Sphere* intersect(float* v1, float* v2);

// Funkcja wyznaczająca wektor jednostkowy opisujący kierunek kolejnego śledzonego promienia
void reflect(float* v);

// Funkcja wczytująca dane z pliku
void readFile();


// Funkcja obliczająca iloczyn skalarny wektorów
float dotProduct(float* p1, float* p2)
{
	return p1[0] * p2[0] + p1[1] * p2[1] + p1[2] * p2[2];
}

// Funkcja obliczająca punkt przecięcia promienia i powierzchni sfery
// p - punkt początkowy promienia; v - wektor opisujący kierunek biegu promienia
Sphere* intersect(float* v1, float* v2)
{
	Sphere* s = nullptr;;
	float a = 0.0f;
	float b = 0.0f;
	float c = 0.0f;
	float delta = 0.0f;
	float r = 0.0f;

	for (auto& sphere : spheres)
	{
		a = v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2];
		b = 2 * (v2[0] * (v1[0] - sphere.position[0]) + v2[1] * (v1[1] - sphere.position[1]) + v2[2] * (v1[2] - sphere.position[2]));
		c = (v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]) -
			(2 * (sphere.position[0] * v1[0] + sphere.position[1] * v1[1] + sphere.position[2] * v1[2])) +
			(sphere.position[0] * sphere.position[0] + sphere.position[1] * sphere.position[1] + sphere.position[2] * sphere.position[2]) -
			(sphere.radius * sphere.radius);

		delta = (b * b) - 4 * (a * c);

		if (delta >= 0)
		{
			r = (-b - sqrt(delta)) / (2 * a);
			if (r > 0)
			{
				inter[0] = v1[0] + r * v2[0];
				inter[1] = v1[1] + r * v2[1];
				inter[2] = v1[2] + r * v2[2];
				s = &sphere;
			}
		}
	}
	return s;
}
// Funkcje normalizujące wektor
void normal(float* p)
{
	float d = 0.0f;
	for (int i = 0; i < 3; ++i)
	{
		d += p[i] * p[i];
	}
	d = sqrt(d);
	if (d > 0.0f)
	{
		for (int i = 0; i < 3; ++i)
		{
			p[i] /= d;
		}
	}
}

void normal(Sphere* sp)
{
	for (int i = 0; i < 3; ++i)
	{
		vec_n[i] = inter[i] - sp->position[i];
	}
	normal(vec_n);
}
// Obliczenie kierunku odbicia promienia w punkcie
void reflect(float* v)
{
	float inv[3];
	for (int i = 0; i < 3; ++i)
	{
		inv[i] = v[i] * -1;
	}

	float cos = dotProduct(vec_n, inv);
	for (int i = 0; i < 3; ++i)
	{
		ref[i] = 2 * cos * vec_n[i] - inv[i];
	}
	normal(ref);
}

// Funkcja oblicza oświetlenie punktu na powierzchni sfery używając modelu Phonga
void phong(float* v, Sphere* sp)
{
	float light_vec[3];
	float reflection_vector[3];
	float viewer_v[3] = { 0.0f, 0.0f, 1.0f };
	float n_dot_1 = 0.0f;
	float v_dot_r = 0.0f;
	Sphere& sphere = *sp;

	inters_c[0] = 0;
	inters_c[1] = 0;
	inters_c[2] = 0;

	for (auto& light : sources)
	{
		light_vec[0] = light.position[0] - inter[0];
		light_vec[1] = light.position[1] - inter[1];
		light_vec[2] = light.position[2] - inter[2];

		normal(light_vec);
		n_dot_1 = dotProduct(light_vec, vec_n);

		reflection_vector[0] = 2 * n_dot_1 * vec_n[0] - light_vec[0];
		reflection_vector[1] = 2 * n_dot_1 * vec_n[1] - light_vec[1];
		reflection_vector[2] = 2 * n_dot_1 * vec_n[2] - light_vec[2];

		normal(reflection_vector);
		v_dot_r = dotProduct(reflection_vector, viewer_v);

		if (v_dot_r < 0)
		{
			v_dot_r = 0;
		}

		if (n_dot_1 > 0)
		{
			float dl = sqrt((light.position[0] - inter[0]) * (light.position[0] - inter[0]) +
				(light.position[1] - inter[1]) * (light.position[1] - inter[1]) +
				(light.position[2] - inter[2]) * (light.position[2] - inter[2]));
			for (int i = 0; i < 3; ++i)
			{
				inters_c[i] += (1.0f / (A + B * dl + C * dl * dl)) *
					(sphere.diffuse[i] * light.diffuse[i] * n_dot_1) +
					(sphere.specular[i] * light.specular[i] * pow(v_dot_r, sphere.specularshinines)) +
					(sphere.ambient[i] * light.ambient[i]) + (sphere.ambient[i] * global[i]);
			}
		}
		else
		{
			inters_c[0] += sphere.ambient[0] * global[0];
			inters_c[1] += sphere.ambient[1] * global[1];
			inters_c[2] += sphere.ambient[2] * global[2];
		}
	}
}

// Funkcja obliczająca kolor piksela dla promienia
void trace(float* p, float* v, int step)
{
	Sphere* sp = intersect(p, v);
	if (sp != nullptr)
	{
		normal(sp);
		reflect(v);
		phong(v, sp);
		color[0] += inters_c[0];
		color[1] += inters_c[1];
		color[2] += inters_c[2];

		if (step < MAX)
		{
			trace(inter, ref, step + 1);
		}
	}
}

// Funkcja rysująca obraz oświetlonej sceny
void Display()
{
	int x = 0;
	int y = 0;
	float xf = 0.0f;
	float yf = 0.0f;
	float width_2 = dimensions[0] / 2.0f;
	float height_2 = dimensions[1] / 2.0f;

	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	for (y = height_2; y > -height_2; --y)
	{
		for (x = -width_2; x < width_2; ++x)
		{
			xf = x / (dimensions[0] / starting_z);
			yf = y / (dimensions[1] / starting_z);

			starting_point[0] = xf;
			starting_point[1] = yf;
			starting_point[2] = starting_z;
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;

			trace(starting_point, starting_directions, 1);
			for (int i = 0; i < 3; ++i)
			{
				if (color[i] == 0.0f)
				{
					color[i] = background[i];
				}
				pixel[0][0][i] = std::min(255.0f, color[i] * 255);
			}
			glRasterPos3f(xf, yf, 0);
			glDrawPixels(1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
		}
	}
	glFlush();
}

// Funkcja wczytująca dane z pliku
void readFile()
{
	std::string filename;
	std::string line;
	std::cout << "Filename: ";
	std::cin >> filename;
	std::ifstream file(filename);
	while (!file.eof())
	{
		file >> line;
		if (line == "dimensions")
		{
			for (int i = 0; i < 2; ++i)
			{
				file >> dimensions[i];
			}
		}
		else if (line == "background")
		{
			for (int i = 0; i < 3; ++i)
			{
				file >> background[i];
			}
		}
		else if (line == "global")
		{
			for (int i = 0; i < 3; ++i)
			{
				file >> global[i];
			}
		}
		else if (line == "sphere")
		{
			Sphere sphere;
			file >> sphere.radius;
			for (int i = 0; i < 3; ++i)
			{
				file >> sphere.position[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> sphere.specular[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> sphere.diffuse[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> sphere.ambient[i];
			}
			file >> sphere.specularshinines;
			spheres.push_back(sphere);
		}
		else if (line == "source")
		{
			Source source;
			for (int i = 0; i < 3; ++i)
			{
				file >> source.position[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> source.specular[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> source.diffuse[i];
			}
			for (int i = 0; i < 3; ++i)
			{
				file >> source.ambient[i];
			}
			sources.push_back(source);
		}
	}
}

// Funkcja inicjalizująca definiująca sposób rzutowania
void MyInit()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-starting_z / 2, starting_z / 2, -starting_z / 2, starting_z / 2, -starting_z / 2, starting_z / 2);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
	readFile();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(dimensions[0], dimensions[1]);
	glutCreateWindow("Ray Tracing");
	MyInit();
	glutDisplayFunc(Display);
	glutMainLoop();

	return 0;
}