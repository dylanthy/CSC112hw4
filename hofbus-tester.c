// Author: Jianchen Shan 
// Please report bugs to Jianchen.Shan@hofstra.edu

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
// #include <unistd.h>
#include <pthread.h>

#include "hofbus.c"

// Count of student threads that have boarded (i.e. station_wait_for_bus has returned)
volatile int students_boarded = 0;

// A general-purpose lock
pthread_mutex_t mutex;

// Used to keep track of waiting order
int ticket = 0;

// Used to how many student have been boarded
int student_boarded_sofar = 0;

// Args passed to a new student thread
struct student_args {
        struct station *station;
        int id;
};


// function executed by each student thread
void*
student_thread(void *arg)
{
	// get a ticket when arrived
	pthread_mutex_lock(&mutex);
	int myticket = ++ticket;
	pthread_mutex_unlock(&mutex);

	// wait for bus to board
	struct student_args *sargs = (struct student_args*)arg;
	printf("student %d got ticket %d and start waiting\n", sargs->id, myticket);
	int myturn = station_wait_for_bus(sargs->station, myticket, sargs->id);

	// update the counter after boarding
        pthread_mutex_lock(&mutex);
	student_boarded_sofar++;
        if(myticket != myturn) {
        	pthread_mutex_unlock(&mutex);
		fprintf(stderr, "Error: student has boarded in wrong order,\t"
				"my ticket is %d, myturn is %d\n", myticket, student_boarded_sofar);
		exit(1);
	}
        printf("student %d with ticket %d has boarded, the turn is %d\n", sargs->id, myticket, myturn);
        pthread_mutex_unlock(&mutex);
		
	return NULL;
}

// Args passed to a new bus thread
struct load_bus_args {
	struct station *station;
	int free_seats;
};

// Flag to show if the bus has left or not
volatile int load_bus_returned = 0;

// function executed by each bus thread
void*
load_bus_thread(void *args)
{
	struct load_bus_args *lbargs = (struct load_bus_args*)args;
	station_load_bus(lbargs->station, lbargs->free_seats);
	load_bus_returned = 1;
	return NULL;
}

// Customized alarm and its handler
const char* alarm_error_str;
int alarm_timeout;

void
myalarm(int seconds, const char *error_str)
{
	alarm_timeout = seconds;
	alarm_error_str = error_str;
	alarm(seconds);
}

void
myalarm_handler(int foo)
{
	fprintf(stderr, "Error: Failed to complete after %d seconds. Something's "
		"wrong, or your system is terribly slow. Possible error hint: [%s]\n",
		alarm_timeout, alarm_error_str);
	exit(1);
}

#ifndef MIN
#define MIN(x,y) ((x) < (y)) ? (x) : (y)
#endif

/*
 * This creates a bunch of threads to simulate arriving buss and students.
 */
int
main()
{
	// Initializing data structures
	pthread_mutex_init(&mutex, NULL);
	struct station station;
	station_init(&station);
	srandom(getpid() ^ time(NULL));
	signal(SIGALRM, myalarm_handler);

	// Make sure station_load_bus() returns immediately if no waiting students.
	myalarm(2, "station_load_bus() did not return immediately when no waiting students");
	station_load_bus(&station, 10);
	// The previous alarm is cancelled if station_load_bus can return right away
	myalarm(0, NULL);

	// Create a bunch of 'students', each in their own thread.
	int i;
	const int total_students = 30;
	struct student_args *args = malloc(sizeof(struct student_args) * total_students);
	int students_left = total_students;
	for (i = 0; i < total_students; i++) {
		pthread_t tid;
		args[i].station = &station;
		args[i].id = i + 1;
		int ret = pthread_create(&tid, NULL, student_thread, &args[i]);
		if (ret != 0) {
			// If this fails, perhaps we exceeded some system limit.
			// Try reducing 'total_students'.
			perror("pthread_create");
			exit(1);
		}
	}
	// Wait a moment until all students are waiting
	usleep(2000000);

	// Make sure station_load_bus() returns immediately if no free seats.
	myalarm(2, "station_load_bus() did not return immediately when no free seats");
	station_load_bus(&station, 0);
	myalarm(0, NULL);

	// Tons of random tests.
	int total_students_boarded = 0;
	const int max_free_seats_per_bus = 10;
	while (students_left > 0) {

		int last_boarded_student = student_boarded_sofar;
		int free_seats = random() % max_free_seats_per_bus;
		printf("Bus entering station with %d free seats\n", free_seats);

		load_bus_returned = 0;
		struct load_bus_args args = { &station, free_seats };
		pthread_t lb_tid;
		int ret = pthread_create(&lb_tid, NULL, load_bus_thread, &args);
		if (ret != 0) {
			perror("pthread_create");
			exit(1);
		}

		int students_to_board = MIN(students_left, free_seats);
		// Wait for seconds. Give station_load_bus() a chance to return
		// and ensure that no additional students board the bus.
		for (i = 0; i < 1000; i++) {
			//printf("students_to_board is %d sofar %d\n", students_to_board, student_boarded_sofar);
			if (student_boarded_sofar - last_boarded_student == students_to_board && load_bus_returned)
				break;
			usleep(3000);
		}

		if (!load_bus_returned) {
			fprintf(stderr, "Error: station_load_bus failed to return\n");
			exit(1);
		} else if (student_boarded_sofar - last_boarded_student != students_to_board){
			printf("Bus departed station with %d new student(s) (expected %d)\n",
					student_boarded_sofar - last_boarded_student, students_to_board);
			fprintf(stderr, "Error: the bus failed to board right amount of students\n");
			exit(1);
		}
		total_students_boarded += students_to_board;
		students_left -= students_to_board;
	}

	if (total_students_boarded == total_students) {
		printf("Looks good!\n");
		return 0;
	} else {
		// I don't think this is reachable, but just in case.
		fprintf(stderr, "Error: expected %d total boarded students, but got %d!\n",
			total_students, total_students_boarded);
		return 1;
	}
}