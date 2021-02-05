#include "t_lib.h"
#include <string.h>
tcb *running;
tcb *ready;
tcb *thTbl; // Thread table

void t_yield()
{
  tcb* tmp;
  tmp = running; // store the currently running thread in tmp
  tmp->next = NULL;
  
  if (ready != NULL) { // only yield if there is a TCB in ready queue
    running = ready; // first TCB in ready queue becomes running 
    ready = ready->next; // ready to next thread
    running->next = NULL;
    tcb* iter;
    iter = ready;
    if (iter == NULL) // if there is nothing else in ready queue
      ready = tmp;
    else { 
      while (iter->next != NULL) // loop till end of queue
        iter = iter->next;
      iter->next = tmp; //add tmp to end of queue
    }
	
    swapcontext(tmp->thread_context, running->thread_context);
  }
}

/* Initialize the UD thread library by setting up the "running" 
and the "ready" queues, creating the TCB of the "main" thread,
and inserting it into the running queue. */
void t_init()
{
  tcb *tmp = (tcb *) malloc(sizeof(tcb));
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));
  getcontext(tmp->thread_context);
  tmp->next = NULL;
  tmp->msg = NULL;
  tmp->mq_sem = NULL;
  tmp->thread_id = 0;  // main thread has ID 0 running = tmp;
  running = tmp;
  thTbl = malloc(sizeof(tcb)*100);
  thTbl[0] = *tmp;
  ready = NULL; 
}

/* Create a thread with priority pri, start function func with argument 
thr_id as the thread id. Function func should be of type void func(int).
TCB of the newly created thread is added to the end of the "ready" queue;
the parent thread calling t_create() continues its execution upon returning
from t_create(). */

int t_create(void (*fct)(int), int id, int pri)
{ 
  size_t sz = 0x10000;
  tcb* tmp = (tcb*) malloc(sizeof(tcb));
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

  getcontext(tmp->thread_context);
  tmp->thread_context->uc_stack.ss_sp = malloc(sz);  /* new stack */
  tmp->thread_context->uc_stack.ss_size = sz;
  tmp->thread_context->uc_stack.ss_flags = 0;
  tmp->thread_context->uc_link = running->thread_context;
  makecontext(tmp->thread_context, (void (*)(void)) fct, 1, id);
  tmp->thread_id = id;
  tmp->thread_priority = pri;
  tmp->next = NULL;

  tmp->msg = NULL;
  sem_t *semMsg = (sem_t *) malloc(sizeof(sem_t));
  semMsg->count = 1;
  semMsg->q = NULL;
  tmp->mq_sem= semMsg;

  

  thTbl[id] = *tmp;
 
  

  if (ready == NULL)
    ready = tmp;
  else {
    tcb* t = ready;
    while(t->next != NULL) {
      t = t->next;
    }
    t->next = tmp; // insert to the end of ready queue
  }
}

/* Terminate the calling thread by removing (and freeing) its TCB from the
"running" queue, and resuming execution of the thread in the head of the 
"ready" queue via setcontext(). */
void t_terminate()
{
  tcb* tmp;
  tmp = running;
  running = ready;  // 1st ready TCB becomes running

  if (ready != NULL)
    ready = ready->next;  // every TCB in ready moves forward

  free(tmp->thread_context->uc_stack.ss_sp);
  free(tmp->thread_context);
  free(tmp->mq_sem);

  messageNode *n1 = tmp->msg;
  messageNode *n2;
  while(n1 != NULL){
	  n2 = n1;
          n1 = n1->next;
	  free(n2->message);
	  free(n2);
  }
  free(tmp);

  setcontext(running->thread_context);  // resume running TCB
}

// free all the TCBs to avoid memory leaks
void t_shutdown()
{
  if (ready != NULL) {
    tcb* tmp;
    tmp = ready;
    while (tmp != NULL) {
      ready = ready->next;
      free(tmp->mq_sem);
      free(tmp->thread_context->uc_stack.ss_sp);
      free(tmp->thread_context);
      messageNode *n1 = tmp->msg;
      messageNode *n2;
      while(n1 != NULL){
	      n2 = n1;
	      n1= n1->next;
	      free(n2->message);
	      free(n2);
      }
      free(tmp);
      tmp = ready;
    }
  }

  free(running->thread_context);
  free(running->mq_sem);
  messageNode *n1 = running->msg;
  messageNode *n2;
  while(n1 != NULL){
	  n2 = n1;
	  n1 = n1->next;
	  free(n2->message);
	  free(n2);
  }
  free(running);

/* for (int i = 0; i<100; i++){
	tcb *th = &thTbl[i];
	if(th != NULL){
		  free(th->thread_context->uc_stack.ss_sp);
		  free(th->thread_context);
		  free(th->mq_sem);
		  messageNode *n1 = th->msg;
		  messageNode *n2;
		  while(n1 != NULL){
			  n2 = n1;
			  n1 = n1->next;
			  free(n2->message);
			  free(n2);
		  }
		  free(th);
	  }
  }*/
 free(thTbl);
  
}
/*
 * Create a new semaphore pointed by sp with a count value of sem_count
 */
int sem_init(sem_t **sp, int sem_count)
{
	*sp = (sem_t*) malloc(sizeof(sem_t));
  	(*sp)->count = sem_count;
  	(*sp)->q = NULL;
}
/*
 * Current thread does a wait operation on the specified sempaphore
 */
void sem_wait(sem_t *sp)
{
	sp->count--;
	if(sp->count < 0){
		tcb* tmp;
		tmp = running;
		tmp->next = NULL;
		running = ready;
		if(ready != NULL)
			ready = ready->next;
		if(sp->q == NULL)
			sp->q = tmp;
		else{
			tcb* iter;
			iter = sp->q;
			while(iter->next != NULL)
				iter = iter->next;
			iter->next = tmp;
		}
		swapcontext(tmp->thread_context, running->thread_context);
	}
}
/*
 * Current thread does a signal on the specified semaphore
 */
void sem_signal(sem_t *sp)
{
	sp->count++;
	if(sp->q != NULL){
		tcb* tmp;
		tmp = sp->q;
		sp->q = sp->q->next;
		if(ready == NULL)
			ready = tmp;
		else{
			tcb* iter;
			iter = ready;
			while(iter->next != NULL)
				iter = iter->next;
			iter->next = tmp;
		}
		tmp->next = NULL;
	}

}
/*
 * Free any state related to the specified semaphore
 */
void sem_destroy(sem_t **sp)
{
	if((*sp)->q != NULL){
		if(ready == NULL)
			ready = (*sp)->q;
		else{
			tcb* iter;
			iter = ready;
			while(iter->next!=NULL)
				iter=iter->next;
			iter->next = (*sp)->q;
		}
		(*sp)->q = NULL;
	}
	free(*sp);
}

/*
 * Create a mailbox pointed by mb 
 */
int mbox_create(mbox **mb)
{
	mbox *mailbox = (mbox *) malloc(sizeof(mbox));
	mailbox->msg = NULL;
	sem_t *tmp = (sem_t *) malloc(sizeof(sem_t));
	tmp->count = 1;
	tmp->q = NULL;
	mailbox->mbox_sem = tmp;
	*mb = mailbox;
	return 1;
}

/*
 * Destroy any state related to the mailbox pointed to by mb
 */
void mbox_destroy(mbox **mb)
{
	//sem_destroy(&(*mb)->mbox_sem);
	free((*mb)->mbox_sem);
	messageNode *tmp = (*mb)->msg;
	messageNode *tmp2;
	while(tmp != NULL){
		tmp2 = tmp;
		tmp = tmp->next;
		free(tmp2->message);
		free(tmp2);
	}
	free(tmp);
	free(*mb);


}

/*
 * Deposit message msg of length len into the mailbox pointed to by mb
 */
void mbox_deposit(mbox *mb, char *msg, int len)
{
	sem_wait(mb->mbox_sem);	
       	messageNode *newNode = (messageNode *) malloc(sizeof(messageNode));
	messageNode *tmp;

	if(mb->msg == NULL){
		char* m = (char *)malloc((1+strlen(msg))*sizeof(char));
		strcpy(m,msg);
		newNode->message = m;
		newNode->len = len;
		newNode->next = NULL;

		mb->msg = newNode;
	}
	else{
		char* m = (char *)malloc((1+strlen(msg))*sizeof(char));
		strcpy(m,msg);
		newNode->message = m;
		newNode->len = len;
		newNode->next = NULL;
		
		tmp = mb->msg;
		while(tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = newNode;
	}
	sem_signal(mb->mbox_sem);

}

/*
 * Withdraw the first message from the mailbox pointed to by mb into
 * msg and set the message's length in len accordingly.
 */
void mbox_withdraw(mbox *mb, char *msg, int *len)
{
	
	sem_wait(mb->mbox_sem);
	messageNode *head = mb->msg;
	char *message = "";

	if(head == NULL){
		return;
	}
	else{
		*len = head->len;
		message = head->message;
		mb->msg = mb->msg->next;
	}
	strcpy(msg,message);
	free(head->message);
	free(head);
	sem_signal(mb->mbox_sem);
	
}

/*
 * Send a message to the thread whose tid is tid.
 * msg is the pointer to the start of the message, and len
 * specifies the length of the message in bytes
 */

void send(int tid, char *msg, int len)
{
	tcb *recThread = &thTbl[tid];

	sem_wait(recThread->mq_sem);

	messageNode *newNode = (messageNode *) malloc(sizeof(messageNode));
	char* m = (char *)malloc((1+strlen(msg))*sizeof(char));
        strcpy(m,msg);
	newNode->message = m;
	newNode->len = len;
	newNode->receiver = tid;
	newNode->sender = running->thread_id;
	newNode->next = NULL;

	if(recThread->msg == NULL)
		recThread->msg = newNode;
	else{
		messageNode *tmp = recThread->msg;
		while(tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = newNode;
	}
	sem_signal(recThread->mq_sem);
}

/*
 * Wait for and receive a message from another thread.
 */
void receive(int *tid, char *msg, int *len)
{
	if(*tid == 0){
		for(int i = 0; i<100; i++){
			tcb *thread = &thTbl[i];
			if(thread != NULL && thread->msg != NULL){
				sem_wait(thread->mq_sem);
				messageNode *queue = thread->msg;
				strcpy(msg,queue->message);
				*len = queue->len;
				*tid = queue->sender;
				thread->msg = queue->next;
				free(queue->message);
				free(queue);
				sem_signal(thread->mq_sem);
				return;
			}
		}
	}
		

	tcb *thread = &thTbl[running->thread_id];
	
	sem_wait(thread->mq_sem);
	messageNode *queue = thread->msg;

	if(queue != NULL){
		strcpy(msg,queue->message);
		*len = queue->len;
		*tid = queue->sender;
		thread->msg= queue->next;
		free(queue->message);
		free(queue);
	}
	sem_signal(thread->mq_sem);
	
}


