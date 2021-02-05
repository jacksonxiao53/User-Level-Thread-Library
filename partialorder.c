/*
 * Jackson Xiao
 * Partial Order
 */
#include <stdio.h>
#include <stdlib.h>
#include "ud_thread.h"

sem_t *s1, *s2, *s3;

void worker1()
{
	sem_wait(s2);
	printf("I am worker 1\n");
	sem_signal(s2);
	sem_signal(s1);
	t_yield();
	t_terminate();
}

void worker2()
{
	printf("I am worker 2\n");
	sem_signal(s2);
	t_yield();
	t_terminate();
}

void worker3()
{
	sem_wait(s1);
	sem_wait(s3);
	printf("I am worker 3\n");
	t_yield();
	t_terminate();
}

void worker4()
{
	sem_wait(s2);
	t_yield();
	printf("I am worker 4\n");
	sem_signal(s2);
	sem_signal(s3);
	t_terminate();
}

int main(void)
{
	t_init();
	sem_init(&s1,0);
	sem_init(&s2,0);
	sem_init(&s3,0);

	t_create(worker3,1,1);
	t_create(worker4,2,1);
	t_create(worker2,3,1);
	t_create(worker1,4,1);

	t_yield();
	t_yield();
	t_yield();
	t_yield();
	t_yield();

	sem_destroy(&s1);
	sem_destroy(&s2);
	sem_destroy(&s3);

	t_shutdown();

	printf("done......\n");

	return 0;
	
}
