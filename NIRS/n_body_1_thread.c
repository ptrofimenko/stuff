#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define random(X) ((X % 1000000) / 1000) //Для инициализации стартовых данных
#define N 10000                   //Количество тел
#define DT 1 
#define FINISH 10                  

struct point {
		double x, y;
};
struct point p[N], v[N], f[N]; //Положение, скорость, сила
double m[N]; 				   //Масса тел
	
double G = 6.67E-11;           //Гравитационная постоянная

void init();                   //Функция инициализации стартовых данных
void calculateForces();        //Функция вычисления сил между телами
void moveBodies();             //Функция вычисления новых положений тел
	
int main() {
	init();
	printf("Initialization completed\n");
	for(int t = 0; t < FINISH; t++) {
		calculateForces();
		moveBodies();
	}
	printf("Calculations completed\n");
	return 0;
}

void calculateForces() {
	double distance, magnitude; 
	struct point direction;
	for (int i = 0; i < N - 1; i++) {
		for (int j = i + 1; j < N; j++) {
			distance = sqrt( powf((p[i].x - p[j].x), 2) + 
							 powf((p[i].y - p[j].y), 2) );
			magnitude = (G * m[i] * m[j]) / powf(distance, 2);
			direction.x = p[j].x - p[i].x;
			direction.y = p[j].y - p[i].y;
			f[i].x = f[i].x + magnitude * direction.x / distance;
		    f[j].x = f[j].x - magnitude * direction.x / distance;
		    f[i].y = f[i].y + magnitude * direction.y / distance;
		    f[j].y = f[j].y - magnitude * direction.y / distance;
		}
	}
}

void moveBodies() {
	struct point deltav;	  //dv = f / m * DT
	struct point deltap;      //dp = (v + dv / 2) * DT
	for(int i = 0; i < N; i++) {
		deltav.x = f[i].x / m[i] * DT; 
		deltap.y = f[i].y / m[i] * DT;
		deltap.x = (v[i].x + deltav.x / 2) * DT;
		deltap.y = (v[i].y + deltav.y / 2) * DT;
		v[i].x = v[i].x + deltav.x;
		v[i].y = v[i].y + deltav.y;
		p[i].x = p[i].x + deltap.x;
		p[i].y = p[i].y + deltap.y;
		f[i].x = f[i].y = 0.0;
	}
} 

void init() {
	srand(1);
	for (int i = 0; i < N; i++) {
		m[i] = random(rand());
		v[i].x = random(rand());
		v[i].y = random(rand());
		p[i].x = random(rand());
		p[i].y = random(rand());
		f[i].x = f[i].y = 0.0;
	}
}
		
	
	
	
	
	
	
	
	
	
	
	
			
			
			
			
			
			
			
			

