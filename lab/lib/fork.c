// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
        // TODO: chky
	if (!(err & FEC_WR)) {
            panic("non-write fault: %x", addr);
        }
        if (!(uvpt[PGNUM((uintptr_t)addr)] & PTE_COW)) {
            panic("non-cow fault: %x", addr);
        }
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
        // TODO: chky
        envid_t envid = sys_getenvid(); // thisenv has not been set yet!
        addr = ROUNDDOWN(addr, PGSIZE);
        if ((r = sys_page_alloc(envid, (void *)PFTEMP, PTE_W|PTE_U|PTE_P)) < 0) {
            panic("allocating at PFTEMP in pgfault: %e", r);
        }
       
        memmove((void *)PFTEMP, addr, PGSIZE);

        if ((r = sys_page_map(envid, (void *)PFTEMP, envid, addr, PTE_U|PTE_W|PTE_P)) < 0) {
            panic("Mapping at %x in pgfault: %e", addr, r);
        }
        //if ((r = sys_page_unmap(envid, (void *)PFTEMP)) < 0) {
        //    panic("Unmapping at %x in pgfault: %e", PFTEMP, r);
        //}

	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
        // TODO: chky
        void *addr = (void *)(pn*PGSIZE);
        int perm;
        envid_t thisenvid;

        thisenvid = sys_getenvid();
        assert(thisenvid != envid);
        
        perm = PGOFF(uvpt[pn]) & (PTE_SYSCALL);

	// LAB 5: shared page
	// TODO: chky
	if (perm & PTE_SHARE) {
        	if ((r = sys_page_map(thisenvid, addr, envid, addr, perm)) < 0) {
            		panic("duppage->sys_page_map %e", r);
        	}
		return r;
	}

        perm |= PTE_COW;
        perm &= ~PTE_W;

        if ((r = sys_page_map(thisenvid, addr, envid, addr, perm)) < 0) {
            panic("duppage->sys_page_map %e", r);
        }
        
        // The code below must be in the end of duppage()!!!
        // Because duppage() run in user env with user stack
        // Which may trigger page fault
        if ((r = sys_page_map(thisenvid, addr, thisenvid, addr, perm)) < 0) {
            panic("duppage->sys_page_map %e", r);
        }
        
	//panic("duppage not implemented");
	return r;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
        // TODO: chky
        int r;
        envid_t envid;
        uint8_t *addr;

        set_pgfault_handler(pgfault);
        if ((envid = sys_exofork()) < 0) {
            return envid;
        }
        if (envid == 0) {
            // We're the child.
            thisenv = &envs[ENVX(sys_getenvid())];
            return 0;
        }

        // We're the parent.
        for (addr = (uint8_t *)UTEXT; addr < (uint8_t *)UTOP; addr += PGSIZE) {
            if ((uintptr_t)addr == UXSTACKTOP - PGSIZE) {
                if ((r = sys_page_alloc(envid, addr, PTE_W|PTE_U|PTE_P)) < 0) {
	            panic("fork->sys_page_alloc");
                    return r;
                }
            } else if (!(uvpd[PDX(addr)] & PTE_P) 
                    || !(uvpt[PGNUM(addr)] & PTE_P)) {
                continue;
            } else if (uvpt[PGNUM(addr)] & (PTE_W|PTE_COW|PTE_SHARE)) {
                if ((r = duppage(envid, PGNUM(addr))) < 0) {
	            panic("fork->duppage");
                    return r;
                }
            } else /*if (!(uvpt[PGNUM(addr)] & PTE_COW))*/ {
                // other present pages
                // FIXME: read-only ?
                int perm = uvpt[PGNUM(addr)] & PTE_SYSCALL;
                if ((r = sys_page_map(thisenv->env_id, addr, envid, addr, perm)) < 0) {
                    panic("fork->sys_page_map at %x: %e", addr, r);
                }
            }
        }
    
        if ((r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)) < 0) {
            panic("fork->sys_env_set_pgfault_upcall: %e", r);
            return r;
        }

        if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
            panic("fork->sys_env_set_status: %e", r);
            return r;
        }

        return envid;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
