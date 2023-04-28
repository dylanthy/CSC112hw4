#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct station {
	int available_spots; //on bus
  	int next; //Next available student's ticket to board
  	int counter;//counter that gives tickets
  	int students;//amount waiting
  	//pthread vars
  	pthread_mutex_t lock; //mutex lock for synchronization, at start and end of load() and wait()
  	pthread_cond_t bus_arrived; //conditional for bus arrival 
  	pthread_cond_t student_boarded; //dconditional for boarding

};

void
station_init(struct station *station)
{
	//initializing station
	station->students = 0;
	station->available_spots = 0;
  	station->next= 1;
  	station->counter = 1;
  	pthread_mutex_init(&station->lock, NULL);
  	pthread_cond_init(&station->bus_arrived, NULL);
  	pthread_cond_init(&station->student_boarded, NULL);
}

void
station_load_bus(struct station *station, int count)
{
	pthread_mutex_lock(&station->lock);//lock station

  	//Bus leaves if no spots or waiters
  	if (station->students == 0 || count == 0) {
    	pthread_mutex_unlock(&station->lock);
    	return;
  	}

  	//Update available seats and notify students through pthread
  	station->available_spots = count;
  	pthread_cond_broadcast(&station->bus_arrived);

  	//Hold bus until no spots or waiters
  	while (station->available_spots > 0 && station->students > 0) {
		pthread_cond_wait(&station->student_boarded, &station->lock);
  	}

  	// Update free spots and let bus leave
  	station->available_spots = 0;
  	pthread_mutex_unlock(&station->lock);//unlock station
}

int
station_wait_for_bus(struct station *station, int myticket, int myid)
{
	pthread_mutex_lock(&station->lock);//lock station
  	station->students++;

  	//wait for arrival and boarding
  	while (myticket != station->next|| station->available_spots== 0) {
    	pthread_cond_wait(&station->bus_arrived, &station->lock);
  	}

  	//fill bus and update station
  	station->students --;
  	station->next ++;
  	station->available_spots --;

  	//signal bus that student has boarded
  	pthread_cond_signal(&station->student_boarded);
  	pthread_mutex_unlock(&station->lock);//unlock station

  	return (myticket);
}
