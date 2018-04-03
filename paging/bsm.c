/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
bs_map_t bsm_tab[8];
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);
	int i;
	for(i=0;i<8;i++){
		bsm_tab[i].bs_status=BSM_UNMAPPED;
		bsm_tab[i].bs_pid=-1;
		bsm_tab[i].bs_vpno=-1;
		bsm_tab[i].bs_npages=0;
		bsm_tab[i].bs_sem=0;   //implemented as spin lock
		//bsm_tab[i].bs_pid=-1;
	}
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	int i;
	for(i=0;i<8;i++){
		if(bsm_tab[i].bs_status==BSM_UNMAPPED)
			*avail=i;
			return OK;
	}
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	if(i<0 || i>=8)
		return SYSERR;
	disable(ps);
	bsm_tab[i].bs_status=BSM_UNMAPPED;
	bsm_tab[i].bs_npages=0;
	bsm_tab[i].bs_pid=-1;
	bsm_tab[i].bs_vpno=-1;
	bsm_tab[i].bs_sem=0;
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	int i;
	long vpno=vaddr/NBPG;

	for(i=0;i<8;i++){
		if(bsm_tab[i].bs_status==BSM_UNMAPPED )//|| (bsm_tab[i].bs_pid!=-1 && bsm_tab[i].bs_pid!=pid))
			continue;
 		if(bsm_tab[i].bs_pid==pid && vpno>=bsm_tab[i].bs_vpno && vpno<bsm_tab[i].bs_vpno+bsm_tab[i].bs_npages){
 			*store=i;
			*pageth=vpno-bsm_tab[i].bs_vpno;
			return OK;
 		}
//kprintf("%d not found:%d %d\n",i,vpno,bsm_tab[i].bs_vpno);
		if(pid<NPROC && pid>=0 && bsm_tab[i].bs_pid==-1 && proctab[pid].bsm_tab_proc[i].bs_status==BSM_MAPPED && vpno>=proctab[pid].bsm_tab_proc[i].bs_vpno && vpno<proctab[pid].bsm_tab_proc[i].bs_vpno+proctab[pid].bsm_tab_proc[i].bs_npages){
			*store=i;
			*pageth=vpno-proctab[pid].bsm_tab_proc[i].bs_vpno;
			return OK;
		}
		kprintf("%d",proctab[pid].bsm_tab_proc[i].bs_vpno);
	}
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;
	disable(ps);
	if(bsm_tab[source].bs_status==BSM_UNMAPPED){
		bsm_tab[source].bs_pid=pid;
		bsm_tab[source].bs_status=BSM_MAPPED;
		bsm_tab[source].bs_vpno=vpno;
		bsm_tab[source].bs_npages=npages;
	}
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable(ps);
	int i;
	for(i=0;i<8;i++){
		if(bsm_tab[i].bs_pid==pid){
			bsm_tab[i].bs_pid=-1;
			bsm_tab[i].bs_vpno=-1;
		}
	}
	restore(ps);
	return OK;
}


