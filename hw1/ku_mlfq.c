/*
   Operating system assignment 1 : KU_MLFQ

   201813339 JunHyung Do
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

// prio constants
const int PRIO_HIGH = 3; // highest prio
const int PRIO_LOW = 1; // lowest prio

// scheduling policy seconds
const int SP_TIMESLICE = 1;
const int SP_TOLERANCE = 2;
const int SP_PRIOBOOST = 10;

// -------- my_task_struct definition --------//
struct my_task_struct{
    pid_t pid;

    int prio;

    int time_left;    

    struct my_task_struct* next;
};

// -------- my_rq definition -------- //

struct my_rq{
    struct my_task_struct * curr; // currently running task
    struct my_task_struct* heads[3]; // head of queues
    int time_to_die; // die when 0
    int time_to_boost; // boost when 0
};

// push task to specific prio in rq
void push_to(struct my_rq* rq, struct my_task_struct *p, int prio){
    struct my_task_struct** phead = &(rq->heads[prio-1]);
    struct my_task_struct* cur = *phead;
    if(*phead == 0){
        *phead = p;
        return;
    }

    while(cur->next) cur  = cur->next;
    cur->next = p;
}

// pop task from specific prio from rq
struct my_task_struct* pop_from(struct my_rq* rq, int prio){
    struct my_task_struct** phead = &(rq->heads[prio-1]);
    struct my_task_struct* ret = *phead;

    if(ret != 0){
        *phead = (*phead)->next;    
    }

    return ret ;
}

// free all tasks in rq
void free_rq(struct my_rq* rq){
    for(int prio = 0;prio < PRIO_HIGH;prio++){
        struct my_task_struct* tmp,*todelete;
        tmp = rq->heads[prio];
        while(tmp!=0){
                todelete = tmp;
                tmp = todelete->next;
                free(todelete);
        }
    }    
}

// register task to HIGHEST priority with resetting values.
void enqueue_task(struct my_rq *rq, struct my_task_struct *p){
    p->prio = PRIO_HIGH;
    p->time_left = SP_TOLERANCE;
    push_to(rq,p,PRIO_HIGH);
}


// push job to rq based on task values.
void put_prev_task(struct my_rq *rq, struct my_task_struct *p){
    p->time_left -= SP_TIMESLICE;
    p->next = 0;
    int degrade = (p->time_left < 1) && (p->prio > PRIO_LOW);
    if(degrade){
        //printf("degrade %d from %d\n",p->pid,p->prio);
        p->prio-=1;
        p->time_left = SP_TOLERANCE;
    }
    push_to(rq,p,p->prio);
}

// perform priority boost in rq
void prio_boost(struct my_rq *rq){
    struct my_task_struct dummy;
    dummy.pid = 0;
    dummy.next = 0;

    struct my_task_struct* head = &dummy;

    struct my_task_struct* poped;

    for(int prio=PRIO_LOW;prio<PRIO_HIGH;++prio){
        while((poped = pop_from(rq,prio)) != 0){
            struct my_task_struct* curr = head;
            //printf("pb %p %p\n",poped,curr);
            poped->prio = PRIO_HIGH;
            poped->time_left = SP_TOLERANCE;
            poped->next = 0;

            while((curr->next != 0) && (poped->pid > curr->next->pid)){
                curr = curr->next;    
            }

            poped->next = curr->next;
            curr->next = poped;
            //printf("%d UP\n",poped->pid);
        }
    }

    if(head->next !=0)
        enqueue_task(rq,head->next);
    rq->time_to_boost = SP_PRIOBOOST;
}

// get highest priority task
struct my_task_struct* pick_next_task(struct my_rq *rq){
    struct my_task_struct* ret =0;
    for(int prio = PRIO_HIGH; prio >= PRIO_LOW; --prio){
        if((ret = pop_from(rq,prio))!=0) break;
    }   
    return ret; 
}

// kill prev, continue next
void context_switch(struct my_rq *rq, struct my_task_struct *prev,
        struct my_task_struct *next){
    if(prev != 0){
        kill(prev->pid,SIGSTOP);
    }

    if(next!=0){
        rq->curr = next;
        kill(next->pid,SIGCONT);
    }
}

// -------- handler  --------
struct my_rq rq;

void schedule(int signo){
    rq.time_to_die -=1;
    rq.time_to_boost -=1;

    if(rq.time_to_boost < 1){
        prio_boost(&rq);
    }

    struct my_task_struct* prev = rq.curr;
    if(prev != 0) put_prev_task(&rq,prev);


    struct my_task_struct* next = pick_next_task(&rq);
    if(rq.time_to_die < 1) next = 0;


    context_switch(&rq,prev,next);


}

// -------- main --------

int main(int argc,char** argv){

    if(argc != 3) exit(1);

    int n = atoi(argv[1]);
    int ts = atoi(argv[2]);

    rq.time_to_die = ts+1;
    rq.time_to_boost = SP_PRIOBOOST+1;

    // fork process
    for(int i=0;i<n;++i){
        pid_t pid=0;

        if(pid=fork()){
            struct my_task_struct* task = malloc(sizeof(struct my_task_struct));
            task->pid = pid;
            enqueue_task(&rq,task);                
        }else{
            char arg[] ={'A'+i,'\0'};
            execl("./ku_app","ku_app",arg,(char*)0);
        }    

    }

    // register handler
    signal(SIGALRM, schedule);

    // sleep
    sleep(3);
    
    // set & start timer
    struct itimerval tval;
    tval.it_interval.tv_sec=1; 
    tval.it_interval.tv_usec=0;
    tval.it_value.tv_sec=0;
    tval.it_value.tv_usec=1;

    int ret = setitimer(ITIMER_REAL,&tval,0);

    if(ret){
        perror("settimer");
        exit(1);    
    }

    while(rq.time_to_die > 0)
        pause();

    free_rq(&rq);
}
