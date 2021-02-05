/*
 * types used by thread library
 */
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/mman.h>

struct tcb {
  int         thread_id;
  int         thread_priority;
  ucontext_t  *thread_context;
  struct messageNode *msg; //thread's message queue
  struct sem_t *mq_sem; //lock for message queue
  struct tcb *next;
};

typedef struct tcb tcb;

typedef struct sem_t {
  int count;
  tcb *q;
} sem_t;

typedef struct messageNode {
  char *message; //copy of the message
  int len; // length of the message
  int sender; //TID of sender thread
  int receiver; //TID of receiver thread
  struct messageNode *next; //pointer to next node
} messageNode;

typedef struct mbox {
  struct messageNode *msg; //message queue
  sem_t *mbox_sem; //used as lock
} mbox;
