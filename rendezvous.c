/*
 * Jackson Xiao
 * Rendezvous
 */
#include <stdio.h>
#include <stdlib.h>
#include "ud_thread.h"

sem_t *s1;
sem_t *s2;

void child1()
{
	printf("child 1: before\n");
	sem_signal(s2);
	t_yield();
	sem_wait(s1);
	printf("child 1: after\n");
	t_terminate();
}

void child2()
{
	printf("child 2: before\n");
	sem_signal(s1);
	sem_wait(s2);
	printf("child 2: after\n");
	t_yield();
	t_terminate();
}

int main(void)
{
	t_init();
	printf("parent: begin\n");
	sem_init(&s1,0);
	sem_init(&s2,0);
	t_create(child1,1,1);
	t_create(child2,2,1);

	t_yield();
	t_yield();
	printf("parent: end\n");

	sem_destroy(&s1);
	sem_destroy(&s2);
	t_shutdown();

	return 0;
}
