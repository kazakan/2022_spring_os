
// Operating system assignment 2
// 2022 Spring
// 201813339 JunHyung Do

#include <stdio.h>
#include <stdlib.h>

// ==============================
// struct ku_pte
// ==============================

struct ku_pte{
    char value;
};

char PFN(char  pte){ return pte>>2; }

char SWAP_OFFSET(char  pte){ return pte >>1; }

char U(char  pte){ return (((char) 2) & pte) >> 1; }

char P(char  pte){ return 1 & pte; }

// ==============================
// struct pcb
// ==============================

typedef struct{
    int pid;
    struct ku_pte* ltable; //linear page table
    struct Pcb* next;   
} Pcb;

// pcb list head & tail
Pcb* ku_mmu_pcbhead = 0;
Pcb* ku_mmu_pcbtail = 0;

// create new pcb and append to list
Pcb* initPcb(int pid){
    Pcb* newPcb = (Pcb*) malloc(sizeof(Pcb));
    newPcb->pid = pid;
    newPcb->ltable = (struct ku_pte*) calloc(256,sizeof(struct ku_pte));
    newPcb->next = 0;

    if(ku_mmu_pcbhead == 0 ){ // when head not exists
        ku_mmu_pcbhead = ku_mmu_pcbtail = newPcb;
    }else{ // at least one node exists
        ku_mmu_pcbtail->next = newPcb;
        ku_mmu_pcbtail = newPcb;
    }
}

// free pcb list
void freePcblist(){
    while(ku_mmu_pcbhead != 0 ){
        Pcb* to_remove = ku_mmu_pcbhead;
        ku_mmu_pcbhead = to_remove->next;

        free(to_remove->ltable);
        free(to_remove);
    }    
    ku_mmu_pcbtail = 0;
}

// get pcb with given pid
Pcb* getPcb(int pid){
    Pcb* cur = ku_mmu_pcbhead;
    while(cur != 0){
        if(cur->pid == pid) return cur;
        cur = cur->next;    
    }    
    return 0;
}

// ==============================
// Queue for fifo
// ==============================

typedef struct {
    struct ku_pte*  pte;
    struct Q* next;     
} Q;

int size = 0;
Q* head = 0;

void push(struct ku_pte* v){
    Q* newQ = malloc(sizeof(Q));
    newQ->pte = v;
    newQ-> next = 0;

    if(head == 0){
        head = newQ;
    }else{
        Q* cur = head;
        while(cur->next != 0){
            cur = cur->next;
        }
        cur->next = newQ;
    }
    ++size;
}

struct ku_pte* pop(){
    if(head == 0) return 0;
    struct ku_pte* ret = head->pte;
    Q* tmp = head;
    head = head->next;
    free(tmp);
    --size;
    return ret;
}

char inQueue(struct ku_pte* v){
    Q* cur = head;
    while(cur != 0){
        if(cur->pte ==  v) return 1;
        cur = cur->next;    
    }
    return 0;   
}
// ==============================
// Free list
// ==============================

// just fill linearly from start.
// And, pcb not removed.
// So, just fill from start till fully allocated.
// After fully filled, what we do is just switching.

int ku_mmu_mem_max=0; 
int ku_mmu_mem_allocated = 0;

int ku_mmu_swap_max = 0;
int ku_mmu_swap_allocated = 0;

#define MEM_FULL (ku_mmu_mem_max <= ku_mmu_mem_allocated)
#define SWAP_FULL (ku_mmu_swap_max <= ku_mmu_swap_allocated)

// ==============================
// ku_page_fault
// ==============================

int ku_page_fault(char pid, char va){
    Pcb* pcb = getPcb(pid); // pcb must be initialized by ku_run_proc
    struct ku_pte* pte =&((pcb->ltable)[PFN(va)]); // get correspond pte

    if(pte->value == 0){ // neither mapped nor swapped out -> allocate new
        if(MEM_FULL && SWAP_FULL) return -1;
        if(MEM_FULL){ // choose to swap out and fill in it
            struct ku_pte* popped = pop(); // this will be swapped out, FIFO 
            if(popped == 0) return -1; // only os page on memory & no other memory available

            pte->value = popped->value;
            push(pte);

            // swap out
            ++ku_mmu_swap_allocated;
            popped->value = (ku_mmu_swap_allocated-1) << 1;
        }else{ //simply allocate  to memory
            ++ku_mmu_mem_allocated;
            pte->value = ((ku_mmu_mem_allocated-1) << 2) + 1;
            push(pte);
        }

    }else{ // swapped out, do swap in
        struct ku_pte* popped = pop();
        char tmp = popped -> value;
        popped -> value = pte->value;
        pte->value = tmp;
        push(pte);
    }
    return 0;    
}


// ==============================
// ku_mmu_init
// ==============================

void * ku_mmu_init(
        unsigned int mem_size,
        unsigned int swap_size){

    void* physical_mem = malloc(sizeof(char)*mem_size);

    ku_mmu_mem_max = mem_size / 4; // maximum # of memory page
    ku_mmu_swap_max = swap_size / 4; // maximum # of swap page

    ku_mmu_mem_allocated = 1; // one is already got by os

    return physical_mem;    
}


// ==============================
// ku_run_proc 
// ==============================

int ku_run_proc(char pid,struct ku_pte** ku_cr3){
    Pcb* pcb;
    if((pcb = getPcb(pid)) == 0){
        pcb = initPcb(pid);    
    }

    *ku_cr3 = pcb->ltable;
    return 0;    
}
