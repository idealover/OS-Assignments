/*
Group No: 4
Member 1: Esha Manideep Dinne, 19CS10030
Member 2: ASRP Vyahruth, 19CS10002
*/

//some print lines are provided as comments to not clutter the output

#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>

using namespace std;

#define MAX_ID 100000000
//max children limit for each node, set to large number if no such limit
#define MAX_CHILDREN 50

//starting tree limits
#define LOWER 300
#define UPPER 500

//time bounds for each producer process
#define T_LOWER 10
#define T_UPPER 20

//producer limit used for allocating resources
#define P_LIMIT 10

// main idea: seperate locks for different parts of the node

//status = 0 if pending, 1 if ready to be executed, 2 if on going, 3 if done

//global variables for checking
int totalcreatedjobs = 0;
int totalconsumedjobs = 0;

//locks for modifying the global variables
pthread_mutex_t glob,glob1;

//main class Node
class Node {
	int id, completiontime, status; //main variables of the class
	int childcount, loocount, probe, completedchildren; //additional defined variables
	Node *parent; //probe can be 0 or 1 only, easier accesss to status
	 //use lock2 for status locks, lock1 for updating completedchildren
	pthread_mutex_t lock1, lock2;
	//semaphores for exploring and adding children
	sem_t e_mutex, mutex; 
	//addresses of children
	Node* children[MAX_CHILDREN];
public:
	Node() {} //default constructor

	void setvalues() {
		id = 1+random()%MAX_ID; //set random id
		parent = NULL; //initial parent is null
		completiontime = random()%251; //random completion time in ms
		status = -1; //nascent value, before assigning parent
		childcount = 0; loocount = 0; probe = 0; //initial values
		completedchildren = 0;

		//initializing mutex locks
		pthread_mutexattr_t attr;
	    pthread_mutexattr_init(&attr);
	    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	    pthread_mutex_init(&lock1, &attr);
	    pthread_mutex_init(&lock2,&attr);

	    //initializing semaphores
	    sem_init(&e_mutex,1,1); sem_init(&mutex,1,1);
	}

	void initialize_status() {
		//initialize status after assigning parent
		status = 1;
	}

	void set_parent(Node *par) {
		//set parent
		parent = par;
	}

	int probestatus() {
		//for easy checking
		return probe;
	}

	int getcompletiontime() {
		//no locks required
		return completiontime;
	}

	int add_child(Node *child) {
		if(probe==1) return -1; //probe is 1 only if status is atleast 2, so it is ongoing/done
		//similar solution to multiple readers and single writer problem, with reader priority
		//add_child() is the writer
		sem_wait(&e_mutex);
		pthread_mutex_lock(&lock2); //check status of node before adding a child
		if(status == -1) {
			//nascent node
			sem_post(&e_mutex);
			pthread_mutex_unlock(&lock2);
			return -1;
		}
		if(status>=2) {
			//ongoing/done node
			sem_post(&e_mutex);
			pthread_mutex_unlock(&lock2);
			return -2;
		}
		if(childcount==MAX_CHILDREN) {
			//max_children limit reached
			sem_post(&e_mutex);
			pthread_mutex_unlock(&lock2);
			return -1;
		}
		if(status==1) status = 0; //if it was ready to be executed, change to pending
		pthread_mutex_unlock(&lock2); //release lock2
		//add child
		children[childcount] = child;
		childcount++; 
		//child is not nascent anymore
		child->initialize_status();
		child->set_parent(this);
		sem_post(&e_mutex);
		return 0; //return 0 if success
	}

	int explore(Node *& intothis) {
		//this is the reader in reader-writer problem, multiple threads can explore at the same time
		// printNode();
		if(probe == 1) return -1; //if probe is 1, return -1
		pthread_mutex_lock(&lock2); //acquire lock
		if(status>=2) {
			//job is running/completed cant be explored further
			pthread_mutex_unlock(&lock2);
			return -1;
		}
		else if(status == 1) {
			//ready to be executed, so return this job by changing status appropriately
			status = 2; probe = 1;
			pthread_mutex_unlock(&lock2);
			intothis = this; 
			return 1;
		}
		pthread_mutex_unlock(&lock2); //checking status is done
		//multiple readers
		sem_wait(&mutex); loocount++;
		if(loocount==1) sem_wait(&e_mutex);
		sem_post(&mutex); bool isposs = false; //isposs checks whether a ready to be exec node is found
		for(int i=0;i<childcount;i++) {
			if(children[i]->explore(intothis)==1) {
				isposs = true;
				break;
			}
		}
		sem_wait(&mutex); loocount--;
		if(loocount==0) sem_post(&e_mutex);
		sem_post(&mutex);
		if(isposs) return 1;
		return -1;
	}

	void update_count(){
		//update completed children count when a child finishes execution
		pthread_mutex_lock(&lock1);
		completedchildren++;
		pthread_mutex_lock(&lock2); //acquire status for modifying if completedchildren is same as childcount
		if(completedchildren==childcount) status = 1;
		pthread_mutex_unlock(&lock2); //release locks
		pthread_mutex_unlock(&lock1);
	}

	void finish_job() {
		//finish the job
		pthread_mutex_lock(&lock2);
		status = 3;
		if(parent!=NULL) parent->update_count(); //inform parent once job is finished
		pthread_mutex_unlock(&lock2);
	}

	int get_id() {
		//get id function, utility
		return id;
	}

	void dfs() {
		// utility function if we want to check the tree structure
		// cout << "At node with id " << id << " completion time " << completiontime << endl;
		printNode();
		for(int i=0;i<childcount;i++) children[i]->dfs();
	}

	void printNode() {
		//print the details of Node when lower number of nodes
		cout << "--------------------ID: " << id << "----------------------" << endl;
		cout << "Completion Time: " << completiontime << endl;
		cout << "Childcount: " << childcount << endl;
		cout << "Status: " << status << endl;
		if(parent!=NULL) cout << "Parent ID: " << parent->get_id() << endl;
		else cout << "Root Node" << endl;
		cout << "-----------------------------------------" << endl;
	}
};

struct SharedDs{
	//structure of shared memory
	Node nodes[UPPER+8*T_UPPER*P_LIMIT]; //number of nodes created by process is on avg 4*time
	int index = 0; //index of the last node
	pthread_mutex_t mutex; //mutex required for creating new node etc
};

Node* create_node(SharedDs *ds) {
	//create node
	pthread_mutex_lock(&(ds->mutex)); //acquire lock
	Node *val = &(ds->nodes[ds->index]); //get address of new node
	(ds->index)++;  //increment index
	pthread_mutex_unlock(&(ds->mutex)); //release lock
	pthread_mutex_lock(&glob1); //lock glob1
	totalcreatedjobs++; //update totalcreated jobs
	pthread_mutex_unlock(&glob1);
	return val;
}

Node* get_random_node(SharedDs *ds) {
	//returns random node
	pthread_mutex_lock(&(ds->mutex));
	int toget = random()%(ds->index); 
	Node *temp = &(ds->nodes[toget]); //return random node, can be nascent node too
	pthread_mutex_unlock(&(ds->mutex));
	return temp;
}

void create_tree(int totalnodes, Node *root, SharedDs *ds) {
	//creates the tree
	int currentnodes = 1; //current nodes
	queue <Node*> toadd; toadd.push(root); //queue of nodes need to be processed
	while(currentnodes<totalnodes) {
		int n = random()%(min(totalnodes-currentnodes+1,MAX_CHILDREN+1)); //number of children to add
		if(toadd.size()==1) n = max(1,n);
		for(int i=0;i<n;i++) {
			Node* temp = create_node(ds); 
			// cout << "Created Node with ID: " << temp->get_id() << endl;
			(toadd.front())->add_child(temp); //create and add as child
			toadd.push(temp);  //push the child to be processed
		}
		toadd.pop(); currentnodes += n; //pop the processed node, update current nodes
	}
}

struct thread_data {
	//producer thread data
	int seed; 
	SharedDs *sm;
};

struct consumer_data {
	//consumer thread data
	int seed;
	SharedDs *sm;
	Node* root;
};

void *producer(void *threadarg) {
	//producer process
	struct thread_data *my_data; 
	my_data = (thread_data*) threadarg;
	srandom(time(NULL)^((my_data->seed)<<16)); //set seed
	int executiontime = T_LOWER + random()%(T_UPPER-T_LOWER+1); //get execution time

	//print execution time
	// cout << "Executing for " << executiontime << endl;

	//beginning time
	clock_t beg = clock();
	double time = 0,totaltime;

	bool added = false;
	Node *toadd = create_node(my_data->sm); //create a new nascent node to be added
	toadd->setvalues(); //set values

	while(1) {
		clock_t end = clock();
		totaltime = time + ((double)(end - beg))/CLOCKS_PER_SEC;
		if(totaltime>=executiontime) {
			//terminate thread when time limit is exceeded
			cout << "Producer process with time " << executiontime << " terminating" << endl;
			break;
		}

		// get random node
		Node *temp = get_random_node(my_data->sm);
		if(added) {
			//if previously added, create new node
			toadd = create_node(my_data->sm);
			toadd->setvalues(); 
		}
		if(temp->add_child(toadd)==0) {
			//if adding is possible, set added to true, sleep randomly between 0-500ms
			added = true; 
			int tosleep = 200 + random()%301;
			cout << "Added job with ID " << toadd->get_id() << endl;
			usleep(tosleep*1000); time += 0.001*tosleep;
		}
		else added = false; //set added to false if cannot be added for some reason
		// all sorts of errors while adding are handled in add_child function itself
	}
	if(added==false) {
		//remove total jobs in tree if a nascent node is waiting in the thread
		pthread_mutex_lock(&glob1);
		totalcreatedjobs--;
		pthread_mutex_unlock(&glob1);
	}
	pthread_exit(NULL);
}

void *consumer(void *threadarg) {
	//consumer runner function
	struct consumer_data *my_data; 
	my_data = (consumer_data*) threadarg;
	srandom(time(NULL)^((my_data->seed)<<16));
	clock_t beg = clock();
	double time = 0,totaltime;

	Node tempnode;
	Node *temp = &tempnode; //temp is where the final node will be stored

	while(1) {
		if(my_data->root->explore(temp)==-1) {
			// we modified the consumer processes to wait until producer processes terminate
			// we can modify this easily so that they dont wait
			// break;
			clock_t end = clock();
			totaltime = time + ((double)(end - beg))/CLOCKS_PER_SEC;
			if(totaltime>=T_UPPER) break; //wait until T_UPPER secs to terminate the thread
			// cout << "Total time is " << totaltime << endl;
			usleep(10000); time+= 0.01; continue;
		}
		// cout << "Started job with id " << temp->get_id() << " completion time " << temp->getcompletiontime() << endl;
		usleep((temp->getcompletiontime())*1000); //sleep for completiontime ms
		time += 0.001*temp->getcompletiontime();
		cout << "Finished job with id " << temp->get_id() << endl;
		temp->finish_job(); //send finished signal to the job
		pthread_mutex_lock(&glob); //acquire lock
		totalconsumedjobs++; //update consumed jobs
		pthread_mutex_unlock(&glob);
	}
	// cout << "Consumer process terminating" << endl;
	pthread_exit(NULL);
}

int main() {
	//process A, create a tree
	srandom(time(NULL)^(getpid()<<16)); //set seed
	int totalnodes = LOWER + random()%(UPPER-LOWER+1); //get total nodes in the tree, between lower and upper
	// cout << "Total nodes is " << totalnodes << endl;

	//take input
	int p,y; 
	cout << "Enter the number of producer threads: "; cin >> p;
	cout << "Enter the number of consumer threads: "; cin >> y;

	//create sharedmemory
	int shmid = shmget(IPC_PRIVATE, sizeof(SharedDs), 0666 | IPC_CREAT);
	if(shmid < 0)
	{
		cout << "Error creating shared memory " << endl;
		exit(EXIT_FAILURE);
	}

	//get pointer to the shared memory
	SharedDs *sm = (SharedDs *)shmat(shmid, NULL, 0);

	//set default values
	sm->index = 0;
	for(int i=0;i<UPPER+8*T_UPPER*P_LIMIT;i++) sm->nodes[i].setvalues();

	//set mutex
	pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(sm->mutex), &attr);
    pthread_mutex_init(&glob,&attr);
    pthread_mutex_init(&glob1,&attr);

	//creating the tree
	Node* root = create_node(sm); 
	root->initialize_status(); //roots status needs to be initialized seperately
	create_tree(totalnodes,root,sm); //create the tree

	// root->dfs();

	//creating the producer threads
	// cout << "Created jobs before producers: " << totalcreatedjobs << endl;
	pthread_t producers[p];
	struct thread_data pdata[p];
	for(int i=0;i<p;i++) {
		pdata[i].sm = sm;
		pdata[i].seed = random()%10000; //get seed for the producer process
		int retval = pthread_create(&producers[i],NULL,&producer,(void*)&pdata[i]); //create thread
		if(retval) {
			//error checking
			cout << "Error creating producer thread, " << retval << endl;
			exit(-1);
		}
	}

	if(fork()==0) {
		//consumer process B
		pthread_t consumers[y]; //consumer threads
		struct consumer_data cdata[y];
		for(int i=0;i<y;i++) {
			cdata[i].sm = sm;
			cdata[i].seed = random()%10000; //set seed
			cdata[i].root = root; //root value needs to be passed
			int retval = pthread_create(&consumers[i],NULL,&consumer,(void*)&cdata[i]);
			if(retval) {
				//error checking
				cout << "Error creating consumer thread, " << retval << endl;
				exit(-1);
			}
		}
		//wait until all consumer threads terminate
		for(int i=0;i<y;i++) pthread_join(consumers[i],NULL);
		cout << "Total consumed Jobs: " << totalconsumedjobs << endl; //print total consumed jobs
		exit(0);
	}

	//wait until all producers terminate
	for(int i=0;i<p;i++) pthread_join(producers[i],NULL);
	int status; pid_t wpid; while ((wpid = wait(&status)) > 0); //wait until process B terminates
	
	cout << "Total created Jobs: " << totalcreatedjobs << endl;
	cout << "Root ID is: " << root->get_id() << endl; //check if it is the last node terminated

	//detach and destroy shared memory
	shmdt(sm);
	shmctl(shmid,IPC_RMID,NULL);

	return 0;
}