#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define random(X) ((X % 1000000) / 1000)                                //Для инициализации стартовых данных
#define N 10000                                                     		//Количество тел
#define DT 1 
#define FINISH 10

double G = 6.67E-11;  

struct point {
		double x, y;
};
																		
void init(struct point *, struct point *, struct point *, double *);
void get_bordres_of_block(int *, int *, int , int , int );
void calculateForces(int, int, int, int, struct point *, struct point *, double *);
void moveBodies(struct point *p, struct point *v, struct point *f, double *m);
int calculate_count_of_tasks(int );
void struct_to_array(double *, struct point *);
void array_to_struct(double *, struct point *);
void zero_forces_array(double *f_array);

int main(int argc, char *argv[]) {
	int commsize, rank, len;
	char procname[MPI_MAX_PROCESSOR_NAME];
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(procname, &len);
	MPI_Status status;
	
	double f_array[2 * N], f_array_new[2 * N];							//Массив для передачи сил через allreduce, 0=x 1=y | 2=x 3=y | 4=x 5=y
	zero_forces_array(f_array_new);
	zero_forces_array(f_array);
	
	if(rank == 0) {
		printf("commsize = %d\n", commsize);
		double time = 0;                                                 //master code
		int count_of_tasks = calculate_count_of_tasks(commsize - 1);
		
		
		int *tasks = malloc(sizeof(*tasks) * count_of_tasks * 2);       //Умножаем на 2, т.к в каждой задаче 2 блока
		
		int position = 0;
		for (int i = 1; i < commsize; i++) {                 //Инициализация пакета задач
			for (int j = i; j < commsize; j++) {
				tasks[position] = i;
				position++;
				tasks[position] = j;
				position++;
			}
		}
		
		
		int *task = malloc(sizeof(*task) * 2);
		int block_num = 0;
		int *recv_rank = malloc(sizeof(*recv_rank) * 1);
		
		time = MPI_Wtime();
		for (int time = 0; time < FINISH; time++) {
			for (int i = 0; i < count_of_tasks + commsize - 1; i++) {
				if (block_num < count_of_tasks * 2) {	
					for (int j = 0; j < 2; j++) {
						task[j] = tasks[block_num];
						block_num++;
					}
				}
				else {                                              	//Если портфель задач исчерпан, инициализируем блоки передаваемой задачи нулями
					task[0] = 0;
					task[1] = 0;
				}
				MPI_Recv(recv_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0,
						 MPI_COMM_WORLD, &status);                      //Принимаем ранг потока, который запрашивает задачу
				MPI_Send(task, 2, MPI_INT, recv_rank[0], 0,             
						 MPI_COMM_WORLD);								//Отправка задачи потоку, сделавшему запрос
				/* printf("\n\n--------------debugging------------\n");
				printf("count_of_tasks = %d\n", count_of_tasks);
				printf("block_num = %d\n", block_num); */				
			}
			
			MPI_Allreduce(f_array, f_array_new, 2 * N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
			zero_forces_array(f_array_new);
			block_num = 0;
			printf("completed\n");
				
		}
		time = MPI_Wtime() - time;
		printf("%f\n", time);
				 
	}
	
	else {                                                              //slave code
		struct point p[N], v[N], f[N];                                  //Положение, скорость, сила
		double m[N];
		init(p, v, f, m);				                                //Масса тел
		
		int bodies_per_block = N / (commsize - 1);
		
		int *task = malloc(sizeof(*task) * 2);
		
		int lb1, rb1, lb2, rb2;
		int *rb, *lb;
		
														
		
		int *send_rank = malloc(sizeof(*send_rank) * 1);
		
		for (int time = 0; time < FINISH; time++) { 
			
			while(1) {
				
				send_rank[0] = rank;
				
				MPI_Send(send_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  //Запрос задачи
				MPI_Recv(task, 2, MPI_INT, 0, 0, MPI_COMM_WORLD,
						 &status);		                                //Прием задачи
				
				if(task[0] == 0) break;									//Прерываение цикла если пакет задач исчерпан
				
				rb = &rb1;
				lb = &lb1;
				get_bordres_of_block(lb, rb, task[0], bodies_per_block, commsize);		//Вычисление границ блоков задачи
				
				
				
				if(task[0] == task[1]) {
					rb2 = rb1;
					lb2 = lb1;
				}
				
				else {
					rb = &rb2;
					lb = &lb2;
					get_bordres_of_block(lb, rb, task[1],
										 bodies_per_block, commsize);
				}
				calculateForces(lb1, rb1, lb2, rb2, f, p, m);
				
			}
			
			struct_to_array(f_array, f);
			
			MPI_Allreduce(f_array, f_array_new, 2 * N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
			array_to_struct(f_array_new, f);
		    
		    moveBodies(p, v, f, m);
			
		}
	} 
	MPI_Finalize();	
	return 0;
}


void zero_forces_array(double *f_array) {
	for (int i = 0; i < N * 2; i++) {
		f_array[i] = 0;
	}
}




void init(struct point *p, struct point *v, struct point *f, double *m) {
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



int calculate_count_of_tasks(int commsize) {
	for (int i = commsize - 1; i != 0; i--) {
		commsize+=i;
	}
	return commsize;
}
	


void get_bordres_of_block(int *lb, int *rb, int num_of_block, 
						  int bodies_per_block, int commsize) {	
	*lb = bodies_per_block * (num_of_block - 1);
	if(num_of_block == commsize) {
		*rb = N;
	}
	else {
		*rb = *lb + bodies_per_block;
	}
} 

void calculateForces(int lb1, int rb1, int lb2, int rb2, struct point *f,
					 struct point *p, double *m) {	
	double distance, magnitude; 
	struct point direction;
	if (rb1 == rb2) {													//В случае когда блоки в задаче равны, расчет производится как в однопоточной версии
		for (int i = lb1; i < rb1 - 1; i++) {
			for (int j = i + 1; j < rb1; j++) {
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
	else {																//Когда блоки разные, необходимо расчитать пары всех тел из блоков		
		for (int i = lb1; i < rb1; i++) {
			for (int j = lb2; j < rb2; j++) {
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
}
	
void struct_to_array(double *f_array, struct point *f) {	
	int j = 0;
	for (int i = 0; i < 2 * N; i++) {
		f_array[i] = f[j].x;
		i++;
		f_array[i] = f[j].y;
		j++;
	}
}

void array_to_struct(double *f_array, struct point *f) {	
	int j = 0;
	for (int i = 0; i < 2 * N; i++) {
		f[j].x = f_array[i];
		i++;
		f[j].x = f_array[i];
		j++;
	}
}
	
void moveBodies(struct point *p, struct point *v, struct point *f, double *m) {
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
	
	
	
	
	
	
	
	
