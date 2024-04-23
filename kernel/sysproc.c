#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	exit();
	return 0;  // not reached
}

int
sys_wait(void)
{
	return wait();
}

int
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)]; // extract the highest 10 bits from the address (we want an item from pgdir)
	if(*pde & PTE_P){ // check if the entry is present in pgdir
		pgtab = (pte_t*)P2V(PTE_ADDR(*pde)); // take the top 20 bits from the pgdir entry
	} else {		// if the item is not present
		if(!alloc || (pgtab = (pte_t*)kalloc()) == 0) // allocate a new page for the table
			return 0;
		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PGSIZE);
		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table
		// entries, if necessary.
		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	return &pgtab[PTX(va)]; // return the pte_t item (we take the lower 10 bits of the logical address)
}

int
sys_share_data(void){
	char* name;
	void* addr;
	int size;

	// bad arguments
	if(argstr(0,&name) < 0 || argint(2,&size)<0 || argptr(1, (void*)&addr, size)<0){
		return -1;
	}

	// take first 10 characters
	char *name_fix; 
	safestrcpy(name_fix, name, 10+1);

	// looks for an empty space
	int free_id = -1; 
	for(int i=0;i<10;i++){
		if(myproc()->sha[i].sz == 0){
			free_id = i;
			break;
		}
	}

	// no empty space
	if(free_id==-1){
		return -3;
	}

	// the name is already taken
	for(int i=0;i<10;i++){
		if (strncmp(myproc()->sha[i].name, name_fix, 11) == 0)
      		return -2; 
	}

	// writing a shared structure
	strncpy(myproc()->sha[free_id].name, name_fix, 11);
	myproc()->sha[free_id].name[11] = '\0';
	myproc()->sha[free_id].addr = addr;
	myproc()->sha[free_id].sz = size;

	// return id
	return free_id;
}

int
sys_get_data(void){
	char* name;
	void** addr;

	// bad arguments
	if(argstr(0,&name) < 0 || argptr(1, (void*)&addr, sizeof(addr))<0){
		return -1;
	}

	// go through all shared structures
	for(int i=0; i<10;i++){
		// found a structure with the name
		if(strncmp(myproc()->sha[i].name, name, 11) == 0){
			*addr =  (int*)myproc()->sha[i].addr;
			return 0;
		}
	}

	// no shared structure
	return -2;
	
	
}
