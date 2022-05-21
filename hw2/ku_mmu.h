#include <stdio.h>
#include <stdlib.h>

// ==============================
// struct ku_pte
// ==============================

struct{
    char value;
} ku_pte;

char PFN(struct ku_pte pte){ return pte.value>>2; }

char SWAP_OFFSET(struct ku_pte pte){ return pte.value >>1; }

char U(struct ku_pte pte){ return (((char) 2) & pte.value) >> 1; }

char P(struct ku_pte pte){ return 1 & pte.value; }

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
    Pcb* newPcb = (pcb*) malloc(sizeof(Pcb));
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
}



int ku_page_fault(char pid, char va){
    return -1;    
}

void * ku_mmu_init(
    unsigned int mem_size,
    unsigned int swap_size){
    
    void* physical_mem = malloc(sizeof(char)*mem_size());
    return physical_mem;    
}

int ku_run_proc(char pid,struct ku_pte** ku_cr3){
    return -1;    
}
