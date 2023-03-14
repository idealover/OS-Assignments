/*
Group No: 4
Member 1: Esha Manideep Dinne, 19CS10030
Member 2: ASRP Vyahruth, 19CS10002
*/



#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include<sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
using namespace std;


typedef struct _process_data{

	double *A; 
	double *B;
	double *C;

	int veclen,another_len,i,j;
} ProcessData;


void mult(void *arg){

	ProcessData *P = (ProcessData*) arg;

	int len = P->veclen;
	int i = P->i;
	int j = P->j;
	int another_len = P->another_len;

	int val = i*another_len + j;
	P->C[val] = 0;
	for(int alpha=0; alpha < len; alpha++){

		P->C[val] += P->A[i*len +alpha] * P->B[alpha*another_len + j]; 
	}

	//printf("each val - %lf\n", P->C[val]);

	return;
}




int main(){

	int r1,c1,r2,c2;
	int pid;

	printf("Enter the number of rows of Matrix A: ");
	scanf("%d", &r1);
	printf("Enter the number of columns of Matrix A: ");
	scanf("%d", &c1);
	printf("Enter the number of columns of Matrix B:");
	scanf("%d", &c2);
	r2 = c1;

	ProcessData d;
	d.veclen = r2;
	d.another_len = c2;


	int shmid = shmget(IPC_PRIVATE,sizeof(double)*r1*c1,IPC_CREAT | 0666);
  
    // shmat to attach to shared memory
    d.A = (double*) shmat(shmid,0,0);

    int shmid1 = shmget(IPC_PRIVATE,sizeof(double)*r2*c2,IPC_CREAT | 0666);
  
    // shmat to attach to shared memory
    d.B = (double*) shmat(shmid1,0,0);



	printf("Please enter the values of the matrix A\n");
	for(int i=0; i < r1 ; i++){

		for(int j=0; j < c1; j++){

			int val = i*c1 + j;

			scanf("%lf", &d.A[val]);
		}
	}

	printf("Please enter the values of the matrix B\n");
	for(int i=0; i < r2 ; i++){

		for(int j=0; j < c2; j++){

			int val = i*c2 + j;

			scanf("%lf", &d.B[val]);
		}
	}

 
    // shmget returns an identifier in shmid
    int shmid2 = shmget(IPC_PRIVATE,sizeof(double)*r1*c2,IPC_CREAT | 0666);
  
    // shmat to attach to shared memory
    d.C = (double*) shmat(shmid2,0,0);


	for(int i=0; i <r1; i++){
		for(int j=0; j<c2 ; j++){

			pid = fork();
			
			if(pid < 0){

				fprintf(stderr,"Error, Fork Failed!\n");
				exit(EXIT_FAILURE);
			}


			if(pid == 0){

				d.i = i;
				d.j = j;
				mult(&d);
				exit(0);
			}


		}
	}


	while(wait(NULL) > 0);

	printf("The product of Matrix A and B is: \n");

	for(int i=0;i<r1;i++){
		for(int j=0;j<c2; j++){
			int val = i* (c2) + j;
			printf("%lf ", d.C[val]);
		}

		printf("\n");
	}

	shmdt((void *) (d.A));
	shmdt((void *) (d.B));
	shmdt((void *) (d.C));
	shmctl(shmid,IPC_RMID,NULL);
	shmctl(shmid1,IPC_RMID,NULL);
	shmctl(shmid2,IPC_RMID,NULL);
}
