#include "statlib.h"
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <sys/time.h>
#include <sys/resource.h>

// Queue implementation for worked threads using vector lib
struct DISPATCH_QUEUE {
	vector<string> work;
	pthread_mutex_t lock;
	unsigned long current;
	bool finished;
};

// Struct filled by worker threads and the mutex lock
struct WORKER_DATA {
	pthread_mutex_t lock;
	ACQUIRED_STATS * toFill;
	DISPATCH_QUEUE * queue;
};

//Function prototypes
void* worker(void* arg);
struct WORKER_DATA* createWorkerData(struct ACQUIRED_STATS* stats, struct DISPATCH_QUEUE* queue);
struct DISPATCH_QUEUE* createDispatchQueue();
void addWork(struct DISPATCH_QUEUE* queue, string str);
inline string getWork(struct DISPATCH_QUEUE* queue);
inline bool isWorkFinished(struct DISPATCH_QUEUE* queue);
void finishWork(struct DISPATCH_QUEUE* queue);
double timevalConvert(struct timeval t);

// Default MAX allowed threads
int worker_allowed = 15;

int main(int argc, char* argv[]){
	// Struct to fill for WC and CPU time
	struct timeval startTime;
	struct timeval endTime;
	struct rusage rbefore;

	getrusage(RUSAGE_SELF, &rbefore);
	gettimeofday(&startTime, NULL);
	
	// Serial Architecture
	if(argc < 2){
		string str;
		struct ACQUIRED_STATS stats = makeAcquiredStats();

		while (getline(cin, str)) {
			struct ACQUIRED_STATS temp = acquireFileStats(str);
			stats = addAcquiredStats(stats, temp);
		}
		gettimeofday(&endTime, NULL);
		struct rusage rafter;
		getrusage(RUSAGE_SELF, &rafter);
		double wctime = timevalConvert(endTime) - timevalConvert(startTime);
		double usertime = timevalConvert(rafter.ru_utime) - timevalConvert(rbefore.ru_utime);
		double systime = timevalConvert(rafter.ru_stime) - timevalConvert(rbefore.ru_stime);

		printAcquiredStats(stats);
		std::cout << GREEN << "Elapsed Wall Clock Time: " << DEF << wctime << " milliseconds" << endl;
		std::cout << GREEN << "CPU (User) Time: " << DEF << usertime << " milliseconds" << endl;
		std::cout << GREEN << "CPU (System) Time: " << DEF << systime << " milliseconds" << endl;
	}
	// Multithreaded Architecture
	else {
		if (strcmp(argv[1], "thread") != 0) {
			std::cout << "Invalid Input\n";
			exit(1);
		}
		// Override allowed workers depending on input
		if (argc == 3) worker_allowed = atoi(argv[2]);

		// Queue, buffer and worker structs to fill
		struct DISPATCH_QUEUE * queue = createDispatchQueue();
		struct ACQUIRED_STATS stats = makeAcquiredStats(); // Uses the statlib.h for the implementation
		struct WORKER_DATA* w_data = createWorkerData(&stats, queue);
		
		// Array of thread ids
		pthread_t workers[worker_allowed];

		// Create threads and assign the function
		for(int i = 0; i < worker_allowed; i++){
			pthread_create(&workers[i], NULL, &worker, (void*) w_data);
		}
		
		// Input with getline
		string str;
		while (getline(cin, str)){
			addWork(queue, str);
		}
		
		// Executes every work in queue
		finishWork(queue);

		// Joining threads
		for(int i = 0; i < worker_allowed; i++){
			pthread_join(workers[i], NULL);
		}

		// Get the ending time
		gettimeofday(&endTime, NULL);
		struct rusage rafter;
		getrusage(RUSAGE_SELF, &rafter);

		// Calculate Wall-Clock, User, and System Time
		double wctime = timevalConvert(endTime) - timevalConvert(startTime);
		double usertime = timevalConvert(rafter.ru_utime) - timevalConvert(rbefore.ru_utime);
		double systime = timevalConvert(rafter.ru_stime) - timevalConvert(rbefore.ru_stime);
		
		// Printing the stats and the elapsed duration
		printAcquiredStats(stats);
		std::cout << GREEN << "Elapsed Wall Clock Time: " << DEF << wctime << " milliseconds" <<  endl;
		std::cout << GREEN << "CPU (User) Time: " << DEF << usertime << " milliseconds" << endl;
		std::cout << GREEN << "CPU (System) Time: " << DEF << systime << " milliseconds" << endl;
	}
	cout << "************************\n" << endl;
	return 0;
}

void* worker(void* arg){
	// Initialize argument (casting)
	struct WORKER_DATA* w_data = ((struct WORKER_DATA*)arg); 

	while(!isWorkFinished(w_data->queue)){
		// Get work to be done from the queue
		string str = getWork(w_data->queue);
		// If input string is valid, acquire stats
		if(str.length() > 0){
			struct ACQUIRED_STATS stats = acquireFileStats(str);
			pthread_mutex_lock(&w_data->lock); // Begin Critical Region
			stats = addAcquiredStats(stats, *w_data->toFill); 
			setAcquiredStats(w_data->toFill, stats);
			pthread_mutex_unlock(&w_data->lock); // End Critical Region
		} else {
			// Sleeps for other threads
			usleep(5000);
		}
	}

	return NULL;
}

// Initialize worker struct
struct WORKER_DATA* createWorkerData(struct ACQUIRED_STATS* stats, struct DISPATCH_QUEUE* queue){
	struct WORKER_DATA* data = new WORKER_DATA();
	pthread_mutex_init(&data->lock, NULL);	
	data->toFill = stats;
	data->queue = queue;
	return data;
}

// Initialize dispatcher queue struct
struct DISPATCH_QUEUE* createDispatchQueue(){
	struct DISPATCH_QUEUE* queue = new DISPATCH_QUEUE();
	pthread_mutex_init(&queue->lock, NULL);
	queue->finished = false;
	queue->current = 0;
	return queue;
}

// Pushes work to the queue
void addWork(struct DISPATCH_QUEUE* queue, string str){
	pthread_mutex_lock(&queue->lock); // Begin Critical Region
	if(str.length() > 0)queue->work.push_back(str); // Add work to the queue
	pthread_mutex_unlock(&queue->lock); // End Critical Region
}

// Pops work from the queue
string getWork(struct DISPATCH_QUEUE* queue){
	string str = "";
	pthread_mutex_lock(&queue->lock); // Begin Critical Region
	if(queue->current < queue->work.size()){
		str = queue->work[queue->current]; // If we have working capacity get work from queue
		queue->current++; // Increment current working threads
	}
	pthread_mutex_unlock(&queue->lock); // End Critical Region
	return str;
}

bool isWorkFinished(struct DISPATCH_QUEUE* queue){
	bool isFinished = false;
	pthread_mutex_lock(&queue->lock); // Begin Critical Region
	if(queue->current >= queue->work.size() && queue->finished){
		isFinished = true;
	}
	pthread_mutex_unlock(&queue->lock); // End Critical Region
	return isFinished;
}

void finishWork(struct DISPATCH_QUEUE* queue){
	pthread_mutex_lock(&queue->lock);
	queue->finished = true;
	pthread_mutex_unlock(&queue->lock);
}

// Time conversion from timeval type
double timevalConvert(timeval t) {
	return (double)t.tv_sec * 1000.0 + (double)t.tv_usec / 1000;
}
