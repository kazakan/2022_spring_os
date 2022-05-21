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


void init_rq(struct my_rq* rq){
    rq->curr = 0;
    rq->time_to_die = 0;
    rq->time_to_boost =0;
    rq->heads[0] =0;
    rq->heads[1] =0;
    rq->heads[2] =0;
}

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


// perform priority boost in rq
void prio_boost(struct my_rq *rq){
    struct my_task_struct dummy;
    dummy.pid = 0;
    dummy.next = 0;
    int prev_prio = rq->curr->prio;

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

    if((prev_prio != PRIO_HIGH) && (dummy.next !=0) && (rq->curr != dummy.next)){
        struct my_task_struct* tmp=&dummy;
        struct my_task_struct* curr=dummy.next;
        while((curr != rq->curr)){
            tmp = curr;
            curr= curr->next;    
        }
        
        struct my_task_struct* end = curr;
        while(end->next!=0){
            end = end->next;    
        }

        // curr == rq->curr
        
        end->next = dummy.next;
        tmp->next = 0;
        dummy.next = curr;
        
    }

    if(dummy.next !=0)
        enqueue_task(rq,dummy.next);

    rq->time_to_boost = SP_PRIOBOOST;
}

// choose next of curr or head of highest queue which task exits
struct my_task_struct* pick_next_task(struct my_rq *rq){
    struct my_task_struct* next_with_curr = 0;
    struct my_task_struct* next_with_prio = 0;

    if(rq->curr != 0) next_with_curr = rq->curr->next;

    for(int prio = PRIO_HIGH; prio >= PRIO_LOW; --prio){
        if((next_with_prio = rq->heads[prio-1])!=0) break;
    }   

    if((next_with_curr == 0) || (next_with_prio ==0)){
        if(next_with_curr) return next_with_curr;
        return next_with_prio;
    }

    if(next_with_curr->prio >= next_with_prio->prio){
        return next_with_curr;    
    }
    return next_with_prio;; 
}

// kill prev, continue next
void context_switch(struct my_rq *rq, struct my_task_struct *prev,
        struct my_task_struct *next){
    if(prev != 0){
        kill(prev->pid,SIGSTOP);
    }

    if(next != 0){
        rq->curr = next;
        kill(next->pid,SIGCONT);
    }
}

// -------- handler  --------
struct my_rq rq;

void schedule(int signo){
    rq.time_to_die -=1;
    rq.time_to_boost -=1;


    struct my_task_struct* prev = rq.curr;
    struct my_task_struct* next = pick_next_task(&rq);

    int was_highest =  (prev != 0) && (prev->prio == PRIO_HIGH);
    int boost = rq.time_to_boost <1;

    if(boost){ // need priority boost
        prio_boost(&rq);
        next = pick_next_task(&rq);
    }

    if(prev != 0){    

        prev->time_left -= SP_TIMESLICE;
        if((prev->time_left < 1) && (prev->prio > PRIO_LOW)){

            //printf("degrade %d from %d\n",prev->pid,prev->prio);

            pop_from(&rq,prev->prio);
            prev->prio -=1;
            prev->time_left = SP_TOLERANCE;

            push_to(&rq,prev,prev->prio);

            next = pick_next_task(&rq);
            prev->next = 0;
        }
    }


    if(rq.time_to_die < 1) next = 0;

    context_switch(&rq,prev,next);


}

// -------- main --------

int main(int argc,char** argv){

    if(argc != 3) exit(1);

    int n = atoi(argv[1]);
    int ts = atoi(argv[2]);

    init_rq(&rq);
    rq.time_to_die = ts+1;
    rq.time_to_boost = SP_PRIOBOOST+1;

    // fork process
    for(int i=0;i<n;++i){
        pid_t pid=0;

        if(pid=fork()){
            struct my_task_struct* task = calloc(1,sizeof(struct my_task_struct));
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
