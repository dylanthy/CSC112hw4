#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct station {
	// write your code here
	int free_seats; // Number of free seats on the bus
  	int waiting_students; // Number of waiting students
  	int next_ticket; // The ticket number of the next student to board
  	int ticket_counter; // The counter for assigning the next ticket number
  	pthread_mutex_t lock; // Mutex lock for synchronization
  	pthread_cond_t bus_arrived; // Condition variable for bus arrival
  	pthread_cond_t student_boarded; // Condition variable for student boarding

};

void
station_init(struct station *station)
{
	// write your code here
	station->free_seats = 0;
  	station->waiting_students = 0;
  	station->next_ticket = 1;
  	station->ticket_counter = 1;
  	pthread_mutex_init(&station->lock, NULL);
  	pthread_cond_init(&station->bus_arrived, NULL);
  	pthread_cond_init(&station->student_boarded, NULL);
}

void
station_load_bus(struct station *station, int count)
{
	// write your code here
	pthread_mutex_lock(&station->lock);

  	// If there are no waiting students or the bus has no free seats, the bus leaves
  	if (station->waiting_students == 0 || count == 0) {
    	pthread_mutex_unlock(&station->lock);
    	return;
  	}

  	// Update the number of free seats and notify the waiting students
  	station->free_seats = count;
  	pthread_cond_broadcast(&station->bus_arrived);

  	// Wait until all free seats are taken or all waiting students have boarded
  	while (station->free_seats > 0 && station->waiting_students > 0) {
		pthread_cond_wait(&station->student_boarded, &station->lock);
  	}

  	// Reset the number of free seats and allow the bus to leave
  	station->free_seats = 0;
  	pthread_mutex_unlock(&station->lock);
}

int
station_wait_for_bus(struct station *station, int myticket, int myid)
{
	// write your code here
	pthread_mutex_lock(&station->lock);
  	station->waiting_students++;

  	// Wait until the bus arrives and it's the student's turn to board
  	while (myticket != station->next_ticket || station->free_seats == 0) {
    	pthread_cond_wait(&station->bus_arrived, &station->lock);
  	}

  	// Board the bus and update the station state
  	station->waiting_students--;
  	station->next_ticket++;
  	station->free_seats--;

  	// Notify the bus that a student has boarded
  	pthread_cond_signal(&station->student_boarded);
  	pthread_mutex_unlock(&station->lock);

  	return (myticket);
}
