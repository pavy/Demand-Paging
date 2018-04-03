/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  //kprintf("xmmap - to be implemented!\n");
  if(source>=8 || bsm_tab[source].bs_status==BSM_UNMAPPED || get_bs(source,npages)<npages || npages==0 || npages>256)
  	return SYSERR;
  if(virtpage<4096 || virtpage>=(1024*1024)) // virtual address - 4G. Therefore, 1M  maximum pages
  	return SYSERR;
  //if(virtpage+npages>1024)
  	//return SYSERR;
  STATWORD ps;
  disable(ps);
  proctab[currpid].bsm_tab_proc[source].bs_status=BSM_MAPPED;
  proctab[currpid].bsm_tab_proc[source].bs_npages=npages;
  proctab[currpid].bsm_tab_proc[source].bs_vpno=virtpage;
  proctab[currpid].bsm_tab_proc[source].bs_pid=currpid;
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  //kprintf("To be implemented!");
  int i;
  STATWORD ps;
  disable(ps);

  for(i=0;i<8;i++){
  	if(proctab[currpid].bsm_tab_proc[i].bs_status==BSM_MAPPED && proctab[currpid].bsm_tab_proc[i].bs_vpno==virtpage){
		proctab[currpid].bsm_tab_proc[i].bs_status=BSM_UNMAPPED;
  	}
  }
  restore(ps);
  return OK;
}
