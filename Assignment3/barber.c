/*
THE SLEEPING BARBER PROBLEM

A barbershop consists of a waiting room with n chairs, and the
barber room containing the barber chair. If there are no customers
to be served, the barber goes to sleep. If a customer enters the
barbershop and all chairs are occupied, then the customer leaves
the shop. If the barber is busy, but chairs are available, then the
customer sits in one of the free chairs. If the barber is asleep, the
customer wakes up the barber. Write a program to coordinate the
barber and the customers.

To make the problem a little more concrete, I added the following information:
• Customer threads should invoke a function named getHairCut.
• If a customer thread arrives when the shop is full, it can invoke balk,
which does not return.
• The barber thread should invoke cutHair.
• When the barber invokes cutHair there should be exactly one thread
invoking getHairCut concurrently.
Write a solution that guarantees these constraints.
*/

#define _REENTRANT

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>
#include <immintrin.h>
#include <semaphore.h>

// The maximum number of customer threads.
#define MAX_CUSTOMERS 25
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL
#define UPPER_MASK 0x80000000UL
#define LOWER_MASK 0x7fffffffUL

/* This calls assembler instruction rdrand and places the random number value into *arand */
int rdrand64_step (unsigned long long *arand)
{
        unsigned char ok;
        __asm__ __volatile__ ("rdrand %0; setc %1" : "=r" (*arand), "=qm" (ok));
        return (int) ok;
}

static unsigned long mt[N];
static int mti = N + 1;

// Function prototypes...
void *getHairCut(void *num);
void *cutHair(void *);

int rdrand(int min, int max);
void barber_cut_time(int min, int max);
int mt19937(int min, int max);
void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
unsigned long genrand_int32(void);

// total number of customers that can be in the shop
// 1 in barber chair, n - 1 in waiting room
int n = 4; 

int customers = 0; //number of customers in the shop

//customers counts the number of customers 
//in the shop; it is protected by mutex.
sem_t mutex; 

sem_t customer;
sem_t barber;

sem_t customerDone; //done getting haircut
sem_t barberDone; //done cutting hair

int main(int argc, char *argv[]) {
		//initialize semaphores
		sem_init(&mutex, 0, 1);  
		sem_init(&customer, 0, 0); 
		sem_init(&barber, 0, 0); 
		sem_init(&customerDone, 0, 0); 
		sem_init(&barberDone, 0, 0); 
		
		
		pthread_t barber_thread;
		pthread_t customer_thread[n];
		
		// Create the barber.
    	pthread_create(&barber_thread, NULL, cutHair, NULL);

		// Create the customers.
		int i;
		for (i=0; i<10; i++) {
		pthread_create(&customer_thread[i], NULL, getHairCut, NULL);
		}

		// Join each of the threads to wait for them to finish.
		for (i=0; i<10; i++) {
		pthread_join(customer_thread[i],NULL);
		}

		// When all of the customers are finished, kill the
		// barber thread.
		
		pthread_join(barber_thread,NULL);
		
		
}

void *getHairCut(void *number) {
		sem_wait(&mutex); 
	
		if(customers == n){
			printf("No seats available, leaving store.\n");
			sem_post(&mutex); 
		}
	
		customers += 1;
		sem_post(&mutex); 

		sem_post(&customer); 
		sem_wait(&barber); 

		//getHairCut ()

		sem_post(&customerDone); 
		sem_wait(&barberDone); 


		sem_wait(&mutex); 
		customers -= 1;
		sem_post(&mutex); 
}

void *cutHair(void *b) {
		sem_wait(&customer);
		sem_post(&barber); 
		
		//cutHair ()
		barber_cut_time(3, 7);
		
		sem_wait(&customerDone); 
		sem_post(&barberDone); 
}

void barber_cut_time(int min, int max)
{
		int time;
        unsigned int eax;
        unsigned int ebx;
        unsigned int ecx;
        unsigned int edx;
        eax = 0x01;
        __asm__ __volatile__("cpuid;" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax));
        if (ecx & 0x40000000)
        		time = rdrand(min, max);
        else
                time = mt19937(min, max);
                
        printf("Barber is cutting hair for %d seconds...\n", time);
        sleep(time);
}

int rdrand(int min, int max)
{
		unsigned long long arand;
		int success = 1;
		int attempt_limit_exceeded = -1;
		int attempt_limit = 10;
		int i;
		for (i = 0; i < attempt_limit; i++) {
				/* if rdrand64_step returned a usable random value */
				if (rdrand64_step(&arand)) {
						if (min == -1 && max == -1)
								return (int) arand;
						else
								return (arand % (unsigned long long)(max - min + 1)) + min;
				}
		}
		return attempt_limit_exceeded;
}

int mt19937(int min, int max)
{
		/* use mt19937 */
		if (min == -1 && max == -1)
				return (int) genrand_int32();
		else
				return (genrand_int32() % (unsigned long long)(max - min + 1)) + min;

}
void init_genrand(unsigned long s)
{
        mt[0]= s & 0xffffffffUL;
        for (mti=1; mti<N; mti++) {
                mt[mti] = (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
                /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
                /* 2002/01/09 modified by Makoto Matsumoto             */
                mt[mti] &= 0xffffffffUL;
        }
}

void init_by_array(unsigned long init_key[], int key_length)
{
        int i;
        int j;
        int k;
        init_genrand(19650218UL);
        i = 1;
        j = 0;
        k = (N > key_length ? N : key_length);
        for (; k; k--) {
                mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL)) + init_key[j] + j;
                mt[i] &= 0xffffffffUL;
                i++;
                j++;
                if (i>=N) {
                        mt[0] = mt[N-1];
                        i=1;
                }
                if (j>=key_length)
                        j=0;
        }
        for (k=N-1; k; k--) {
                mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL)) - i;
                mt[i] &= 0xffffffffUL;
                i++;
                if (i>=N) {
                        mt[0] = mt[N-1];
                        i=1;
                }
        }
        mt[0] = 0x80000000UL;
}

unsigned long genrand_int32(void)
{
        unsigned long y;
        static unsigned long mag01[2]={0x0UL, MATRIX_A};
        if (mti >= N) {
                int kk;
                if (mti == N+1)
                        init_genrand(5489UL);
                for (kk=0;kk<N-M;kk++) {
                        y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                        mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
                }
                for (;kk<N-1;kk++) {
                        y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                        mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
                }
                y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
                mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];
                mti = 0;
        }
        y = mt[mti++];
        y ^= (y >> 11);
        y ^= (y << 7) & 0x9d2c5680UL;
        y ^= (y << 15) & 0xefc60000UL;
        y ^= (y >> 18);
        return y;
}



