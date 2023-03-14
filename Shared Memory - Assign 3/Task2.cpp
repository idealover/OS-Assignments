/*
Group No: 4
Member 1: Esha Manideep Dinne, 19CS10030
Member 2: ASRP Vyahruth, 19CS10002
*/

#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

using namespace std;

#define QUEUE_SIZE 8
#define N 1000

struct compjob {
	int prod_no;
	int status;
	int matr[N][N];
	int id;
};

/*
status 0-7 means those many blocks are computed
8 means all blocks are computed, waiting for writing the product and erasing the jobs
*/

typedef compjob JOB;

JOB* generatejob(int pno) {
	JOB* j = (JOB*)malloc(sizeof(JOB));
	j -> prod_no = pno;
	j -> status = 0;
	for(int i=0;i<N;i++) {
		for(int k=0;k<N;k++) {
			j->matr[i][k] = 9-(random()%19);
		}
	}
	j->id = 1+(random())%100000;
	return j;
}

void copyjob(JOB* from, JOB* to) {
	to->prod_no = from->prod_no;
	to->status = from->status;
	for(int i=0;i<N;i++) {
		for(int k=0;k<N;k++) {
			to->matr[i][k] = from->matr[i][k];
		}
	}
	to->id = from->id;
}

void printJob(JOB *j1) {
	cout << "Product Number: " << j1->prod_no << endl;
	cout << "Status: " << j1->status << endl;
	cout << "Matrix ID: " << j1->id << endl;
	//can print matrix when N is small

	// cout << "Matrix:" << endl;
	// for(int i=0;i<N;i++) {
	// 	for(int j=0;j<N;j++) cout << j1->matr[i][j] << " ";
	// 	cout << endl;
	// }
}

void resetJob(JOB *j) {
	j->prod_no = 0;
	j->status = -8;
	for(int i=0;i<N;i++) {
		for(int k=0;k<N;k++) j->matr[i][k] = 0;
	}
	j->id = 0;
}

void computeblock(JOB *j1, JOB *j2, int **block, int d) {
	int hor1 = (N/2)*(d/4), ver1 = (N/2)*((d%4)/2), ver2 = (N/2)*(d%2);
	for(int i=0;i<N/2;i++) {
		for(int j=0;j<N/2;j++) {
			block[i][j] = 0;
			for(int k=0;k<N/2;k++) {
				block[i][j] += ((j1->matr[hor1+i][ver1+k])*(j2->matr[ver1+k][ver2+j]));
			}
		}
	}
	return;
}

void updateJob(JOB *j1, int ** block, int d) {
	int hor1 = (N/2)*(d/4), ver1 = (N/2)*((d%4)/2), ver2 = (N/2)*(d%2);
	// printf("d is %d\n",d);
	// printf("Updating ")
	for(int i=0;i<N/2;i++) {
		for(int j=0;j<N/2;j++) {
			j1->matr[hor1+i][ver2+j] += block[i][j];
		}
	}
	(j1->status)++;
}

int princediag(JOB *j) {
	int sum = 0;
	for(int i=0;i<N;i++) sum+=(j->matr[i][i]);
	return sum;
}

struct SharedDS{
	JOB queue[QUEUE_SIZE+1];
	JOB temporaryproduct;
	int job_created;
	int deletions,qsize;
	pthread_mutex_t mutex;
};

int jcl;

int accessmemory(SharedDS *ds, int type, JOB **j1, JOB **j2 = NULL) {
	pthread_mutex_lock(&(ds->mutex)); //lock the critical section

	//to push a job into the queue
	if(type==0) {
		if((ds->qsize)-(ds->deletions)>=QUEUE_SIZE) {
			pthread_mutex_unlock(&(ds->mutex)); return -1;
		}
		else if((ds->job_created)>=jcl) {
			pthread_mutex_unlock(&(ds->mutex));
			return -2; //return -2 when jobcounter limit is reached
		}
		else {
			JOB *toinsert = &(ds->queue[(ds->qsize)%QUEUE_SIZE]); (ds->qsize)++;
			// cout << "Inserting Job: " << endl; printJob(*j1);
			(ds->job_created)++;
			pthread_mutex_unlock(&(ds->mutex)); copyjob(*j1,toinsert);
			return 1;
		}
	}

	//get jobcreated counter
	else if(type==1) { 
		pthread_mutex_unlock(&(ds->mutex)); return ds->job_created;
	}

	//work on the top two blocks
	else if(type==2) {
		if((ds->qsize)-(ds->deletions)<2) {
			pthread_mutex_unlock(&(ds->mutex)); return -1;
		}
		else {
			//when the job is getting updated
			if(ds->queue[(ds->deletions)%QUEUE_SIZE].status<0) {
				pthread_mutex_unlock(&(ds->mutex)); return -1;
			}
			//if it is the first job
			else if(ds->queue[(ds->deletions)%QUEUE_SIZE].status==0) {
				//update the temporary product first
				resetJob(&(ds->temporaryproduct)); 
				int tempstatus = ds->queue[(ds->deletions)%QUEUE_SIZE].status;
				(ds->queue[(ds->deletions)%QUEUE_SIZE].status)++; 
				(ds->queue[(ds->deletions+1)%QUEUE_SIZE].status)++; //update the status of the jobs
				pthread_mutex_unlock(&(ds->mutex));
				*j1 = &(ds->queue[(ds->deletions)%QUEUE_SIZE]);
				*j2 = &(ds->queue[(ds->deletions+1)%QUEUE_SIZE]); 
				return tempstatus;

			}
			else if(ds->queue[(ds->deletions)%QUEUE_SIZE].status<8) {
				int tempstatus = ds->queue[(ds->deletions)%QUEUE_SIZE].status;
				(ds->queue[(ds->deletions)%QUEUE_SIZE].status)++; 
				(ds->queue[(ds->deletions+1)%QUEUE_SIZE].status)++; //update the status of the jobs
				pthread_mutex_unlock(&(ds->mutex));
				*j1 = &(ds->queue[(ds->deletions)%QUEUE_SIZE]);
				*j2 = &(ds->queue[(ds->deletions+1)%QUEUE_SIZE]); 
				return tempstatus;
			}
			//when all the multiplications are being carried out
			else {
				pthread_mutex_unlock(&(ds->mutex)); return -1;
			}
		}
	}

	//get address of temporary product
	else if(type==3) {
		pthread_mutex_unlock(&(ds->mutex)); *j1 = &(ds->temporaryproduct);
		return 1;
	}

	//remove first two elements and add one element
	else if(type==4) {
		(ds->deletions)+=2; 
		JOB *toinsert = &(ds->queue[(ds->qsize)%QUEUE_SIZE]); (ds->qsize)++;
		copyjob(&(ds->temporaryproduct),toinsert);
		pthread_mutex_unlock(&(ds->mutex)); return 1;
	}

	//size of queue
	else if(type==5) {
		int size = (ds->qsize - ds->deletions); 
		pthread_mutex_unlock(&(ds->mutex)); return size;
	}

	//retrieve the last element of the queue
	else if(type==6) {
		*j1 = &(ds->queue[(ds->qsize-1+QUEUE_SIZE)%QUEUE_SIZE]); 
		pthread_mutex_unlock(&(ds->mutex)); return 1;
	}

	pthread_mutex_unlock(&(ds->mutex)); //unlock the critical section
	return -1;
}

int main() {
	int np,nw; pid_t childpid;
	cout << "Enter number of producer processes: "; cin >> np;
	cout << "Enter number of worker processes: "; cin >> nw;
	cout << "How many matrices to multiply: "; cin >> jcl;

	int shmid = shmget(IPC_PRIVATE, sizeof(SharedDS), 0666 | IPC_CREAT);
	if(shmid < 0)
	{
		cout << "Error creating shared memory " << endl;
		exit(EXIT_FAILURE);
	}

	//get pointer to the shared memory
	SharedDS *sm = (SharedDS *)shmat(shmid, NULL, 0);

	//set default values
	sm->job_created = 0; sm->deletions = 0; sm->qsize = 0;

	//set mutex
	pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(sm->mutex), &attr);

    clock_t starttime = clock(); 
    double totaltime = 0;

    for(int i=0;i<nw;i++) {
    	if((childpid=fork())==0) {
    		//consumer processes
    		srandom(time(NULL)^(getpid()<<1)); //set seed
    		while(1) {
    			usleep(random()%3000000); //sleep for 0-3 seconds
	    		JOB *j1, *j2; int d;
	    		while((d=accessmemory(sm,2,&j1,&j2))==-1) usleep(10000); //sleep for 10ms when the first processes are busy
	    		cout << "Worker " << i+1 << " is computing block " << d << endl;
	    		int **block = (int**)malloc(sizeof(int*)*(N/2));
				for(int j=0;j<N/2;j++) block[j] = (int*)malloc(sizeof(int)*(N/2));
				computeblock(j1,j2,block,d); JOB *tempprod;
				accessmemory(sm,3,&tempprod); 
				cout << "Worker " << i+1 << " is updating block " << d << endl;
				updateJob(tempprod,block,d);
				// printJob(tempprod);
	    		if(d==0) {
	    			//first process, need to handle pushing back to queue
	    			while(tempprod->status<0) {
	    				usleep(10000);
	    			}
	    			accessmemory(sm,4,NULL); //remove first two processes and add product to end of queue
	    		}
    		}
    	}
    }

    for(int i=0;i<np;i++) {
    	if((childpid=fork())==0) {
    		//producer process
    		srandom(time(NULL)^(getpid()<<1)); //set seed
    		while(1) {
    			int retval;
    			//check if Job counter is at its limit
    			if(accessmemory(sm,1,NULL)>=jcl) {
    				cout << "Producer process " << i+1 << " is terminating" << endl;
    				exit(EXIT_SUCCESS);
    			}
    			JOB *j = generatejob(i+1); usleep(random()%3000000); //generate job and sleep for 0-3 seconds
    			while((retval=accessmemory(sm,0,&j))==-1) {
    				if(retval==-2) break; //break if total jobs created more than matrices to multiply
    				usleep(10000); //sleep for 10ms if cannot insert into queue
    			}
    			//terminate producer process when job created limit is reached
    			if(retval == -2) {
    				cout << "Producer process " << i+1 << " is terminating" << endl;
    				exit(EXIT_SUCCESS);
    			}
    			cout << "Inserted a Job."; cout << "Produer No: " << i+1 << endl;
    			cout << "Producer PID: " << getpid() << endl; cout << "Job details: " << endl;
    			printJob(j); 
    		}
    	}
    }

    while(1) {
    	usleep(10000); //sleep for 10ms
    	totaltime += 0.01;

    	if(accessmemory(sm,1,NULL)>=jcl) {
    		if(accessmemory(sm,5,NULL)!=1) {
    			// cout << "Queue size is " << accessmemory(sm,5,NULL) << endl;
    			continue;
    		}
    		cout << "Final jobs: " << accessmemory(sm,1,NULL) << endl;
    		clock_t endtime = clock();
    		totaltime += ((double)(endtime - starttime))/CLOCKS_PER_SEC;
    		cout << setprecision(3) << fixed << "Total Time:  " << totaltime << "secs" << endl;
    		JOB *lastele; accessmemory(sm,6,&lastele);
			cout << "Sum of elements of final matrix: " << princediag(lastele) << endl;
			kill(-getpid(), SIGQUIT); // quit all processes of ths process groups
    	}
    }

	return 0;
}