#define TEXT_SZ 2048

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>







struct shared_use_st {
	sem_t check;
	int PrintedByA, PrintedByB;
	char some_text[TEXT_SZ];
	char package[15];

};
int summ_of_packagesA=0;
int messagesA; //counter for the messages process B sent
int closing=0; //flag variable for closing when "Bye"
pthread_t producer;

void*  producer_thread (void * args){
	int running=1 ;
	char buffer[BUFSIZ];
	
	struct shared_use_st* shared_stuff= (struct shared_use_st*) args;
	while(running) {
		
		printf("enter some text: ");
		fgets(buffer,BUFSIZ, stdin);
		int message_length=strlen(buffer);
		int packages=message_length/15;
		int count=0;
		
		if(message_length!=0){//στη περίπτωσηη που η είσοδος είναι αλλαγή γραμμής
            while(packages>=0){
                if(shared_stuff->PrintedByA==0){//Ώστε να μη προλάβει να πάρει σειρά το consumer_thread
                sem_wait(&shared_stuff->check);
                strncpy(shared_stuff->package,&buffer[count],15);
				shared_stuff->PrintedByA=1;
                sem_post(&shared_stuff->check);
				count+=15;
				packages--;
				summ_of_packagesA++;
                }
            }
        }
		messagesA++;
		shared_stuff->PrintedByA = 1;
		if (strncmp(buffer, "#BYE#", 5) == 0) {
			running = 0;
			closing=1;
			messagesA--; //αφαιρούμε το #BYE# από το πλήθος μηνυμάτων
			summ_of_packagesA--;
		
		
		}sem_post(&shared_stuff->check);
	}
}


void* consumer_thread(void* args){
	int running=1 ;
	struct shared_use_st* shared_stuff= (struct shared_use_st*) args;
	int receivedA=0;
	double final_time=0.0; //το άθροισμα των διαφορών του κάθε πακέτου με το προηγούμενο
	while(running) {
		struct timeval arriving_time,last_time;
		if (shared_stuff->PrintedByB==1||closing==1){
			gettimeofday(&arriving_time,NULL);
			if(receivedA!=0){ // Στο πρώτο πακέτο κρατάμε την ώρα που ήρθε αλλα δεν το συγκρίνουμε
				double elapsed_time= (arriving_time.tv_sec-last_time.tv_sec)* 1e6;
				elapsed_time+=(arriving_time.tv_usec-last_time.tv_usec);
				final_time+=elapsed_time;
			}
			last_time=arriving_time; //αποθήκευση της τωρινής άφιξης πακέτου για σύγκριση στην επόμενη άφιξη
			if (strncmp(shared_stuff->package, "#BYE#", 5) != 0) {
				sem_wait(&shared_stuff->check);
				printf("\n you said: %s",shared_stuff->package);
				shared_stuff->PrintedByB=0;
				receivedA++;
			}
			else{
				running = 0;
				pthread_cancel(producer);
				break;
				
			}
			sem_post(&shared_stuff->check);
		}
		
	}
	double average_time = final_time / ((receivedA-1 ) *1e6); // Εύρεση μέσου όρου(σε microseconds)
    printf("The consumer_thread of process A had a package every %.2f seconds on average\n", average_time);
    printf("The total packages this process (A) received are: %d \n",receivedA);
}

int main()
{
	pthread_t consumer;
	void *shared_memory = (void *)0;
	struct shared_use_st *shared_stuff;
	void* thread_result;
	int shmid;
	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);

	shared_stuff = (struct shared_use_st *)shared_memory;
	shared_stuff->PrintedByA=0;
	shared_stuff->PrintedByB=0;
	if(pthread_create(&producer,NULL,producer_thread,(void *)shared_stuff)||pthread_create(&consumer,NULL,consumer_thread,(void*)shared_stuff)!=0){
		perror("Thread join failed");
		exit(EXIT_FAILURE);
	}
	pthread_join(consumer,NULL);
	pthread_join(producer, NULL);
	printf("the total messages this process(A) sent are: %d \n",messagesA);
	printf("the total packages this process (A) sent are: %d \n",summ_of_packagesA);


	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}