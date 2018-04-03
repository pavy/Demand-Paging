/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	unsigned long	savsp, *pushsp;
	STATWORD 	ps;    
	int		pid;		/* stores new process id	*/
	struct	pentry	*pptr;		/* pointer to proc. table entry */
	int		i,j;
	unsigned long	*a;		/* points to list of args	*/
	unsigned long	*saddr;		/* stack address		*/
	int		INITRET();

	disable(ps);

	pid = create(procaddr,ssize,priority,name,nargs,args);
	pptr = &proctab[pid];

	hsize = (int) roundew(hsize);
	int bs_id;
	if(hsize<0 || hsize>256 || get_bsm(&bs_id)==SYSERR){
		restore(ps);
		return(SYSERR);
	}
	bsm_map(pid,4096,bs_id,hsize);
	pptr->store=bs_id;                  /* backing store for vheap      */
	pptr->vhpno=4096;                  /* starting pageno for vheap    */
	pptr->vhpnpages=hsize;              /* vheap size                   */
	pptr->vmemlist=(struct mblock *) getmem(sizeof(struct mblock));
	pptr->vmemlist->mnext=NULL;
	pptr->vmemlist->mlen=hsize*NBPG;
	//pptr->vmemlist->mnext=(struct mblock *) (4096*NBPG);



	
	restore(ps);

	return(pid);
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
