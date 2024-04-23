#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
#include "stddef.h"

#define MAX_REG_SZ 0x40000000 // maximum regular process size on virtual memory


int map_shared(struct proc *curproc);

// returns the address of the pte from pgdir which refers to the virtual address va
// if alloc!=0 creates all required table pages

static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)]; // extract the highest 10 bits from the address (we want an entry from pgdir)
	if(*pde & PTE_P){ // check if the entry is present in pgdir
		pgtab = (pte_t*)P2V(PTE_ADDR(*pde)); // get the top 20 bits from the pgdir entry
	} else {		//ako stavka nije prisutna
		if(!alloc || (pgtab = (pte_t*)kalloc()) == 0) // allocate a new page for the table
			return 0;
		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PGSIZE);
		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table
		// entries, if necessary.
		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	return &pgtab[PTX(va)]; // return the pte_t entry (take the lower 10 bits of the logical address)
}

// creates pte's for virtual addresses starting with va that point to physical addresses starting with pa,
// va and size do not need to be rounded per page
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
	char *a, *last;
	pte_t *pte;

	a = (char*)PGROUNDDOWN((uint)va); // round to the beginning of the page (ex. 5KB => 4KB)
	last = (char*)PGROUNDDOWN(((uint)va) + size - 1); // round the beginning of the last page that has to be mapped
	for(;;){
		if((pte = walkpgdir(pgdir, a, 1)) == 0) // look for pte in pgdir, if it doesn't exist, create it (if the flag was 0, not 1, then it would only be read)
			return -1;
		if(*pte & PTE_P)  	// if something is already mapped to that page
			panic("remap");
		*pte = pa | perm | PTE_P;
		if(a == last)
			break;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

int
exec(char *path, char **argv)
{
	char *s, *last;
	int i, off;
	uint argc, sz, sp, ustack[3+MAXARG+1];
	struct elfhdr elf;
	struct inode *ip;
	struct proghdr ph;
	pde_t *pgdir, *oldpgdir;
	struct proc *curproc = myproc();

	begin_op();

	if((ip = namei(path)) == 0){
		end_op();
		cprintf("exec: fail\n");
		return -1;
	}
	ilock(ip);
	pgdir = 0;

	// Check ELF header
	if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
		goto bad;
	if(elf.magic != ELF_MAGIC)
		goto bad;

	if((pgdir = setupkvm()) == 0)
		goto bad;

	// Load program into memory.
	sz = 0;
	for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
		if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
			goto bad;
		if(ph.type != ELF_PROG_LOAD)
			continue;
		if(ph.memsz < ph.filesz)
			goto bad;
		if(ph.vaddr + ph.memsz < ph.vaddr)
			goto bad;
		if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
			goto bad;
		if(ph.vaddr % PGSIZE != 0)
			goto bad;
		if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
			goto bad;
	}

	iunlockput(ip);
	end_op();
	ip = 0;

	// Allocate two pages at the next page boundary.
	// Make the first inaccessible.  Use the second as the user stack.
	sz = PGROUNDUP(sz);


	if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
		goto bad;
	clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
	sp = sz;

	// Push argument strings, prepare rest of stack in ustack.
	for(argc = 0; argv[argc]; argc++) {
		if(argc >= MAXARG)
			goto bad;
		sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
		if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
			goto bad;
		ustack[3+argc] = sp;
	}
	ustack[3+argc] = 0;

	ustack[0] = 0xffffffff;  // fake return PC
	ustack[1] = argc;
	ustack[2] = sp - (argc+1)*4;  // argv pointer

	sp -= (3+argc+1) * 4;
	if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
		goto bad;

	// Save program name for debugging.
	for(last=s=path; *s; s++)
		if(*s == '/')
			last = s+1;
	safestrcpy(curproc->name, last, sizeof(curproc->name));

	// Commit to the user image.
	oldpgdir = curproc->pgdir;
	curproc->pgdir = pgdir;
	curproc->sz = sz;
	curproc->tf->eip = elf.entry;  // main
	curproc->tf->esp = sp;
	if(map_shared(curproc)<0) 
		goto bad;
	switchuvm(curproc);
	
	freevm(oldpgdir);

	

	return 0;

	bad:
	if(pgdir)
		freevm(pgdir);
	if(ip){
		iunlockput(ip);
		end_op();
	}
	return -1;
}

// maps shared memory
int
map_shared(struct proc *curproc){
	uint va = MAX_REG_SZ;
	
	for(int i=0;i<10;i++){
		if(curproc->sha[i].sz>0){
				uint off = PTE_FLAGS((uint)(curproc->sha[i].addr)); // get offset (last 12 bits)
				uint last = PGROUNDUP(curproc->sha[i].sz); // last page (for the loop)
				
				// passes through all pages of the shared structure
				for(uint j=0;j<last;j+=PGSIZE){
					
					// takes pte from parent directory
					pte_t *pte = walkpgdir(curproc->parpgdir, (char*)(curproc->sha[i].addr + j), 0);

					// physical address of the pte
					uint pa = PTE_ADDR(*pte);
					
					// adds last 12 bits (current offset)
					pa |= PTE_FLAGS((uint)(curproc->sha[i].addr + j));

					// maps the page
					if(mappages(curproc->pgdir, va + j, PGSIZE, pa, PTE_W | PTE_U)<0){
						freevm(curproc->pgdir);
						return -1;
					}
				}

				// puts the virtual address of the shared structure at the starting address of the previous loop iteration
				curproc->sha[i].addr = va | off;

				// increments the virtual address by the size of the shared structure, rounded to pages
				va += curproc->sha[i].sz;
				va = PGROUNDUP(va);

			
		}
	}

	return 0;
}
