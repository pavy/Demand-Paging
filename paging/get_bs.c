#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
	STATWORD ps;

    //kprintf("To be implemented!\n");
    if(bs_id>=8 || npages==0 || npages>256)
    	return SYSERR;

    if(bsm_tab[bs_id].bs_status==BSM_UNMAPPED)	
    	bsm_map(-1,-1,bs_id,npages);   //if context switch happens here and some other process gets vheap or maps lesser pages the following conditions checks will prevent get_bs return value corruptions.

    //while(bsm_tab[bs_id].bs_sem)  Implement a lock here

    if(bsm_tab[bs_id].bs_status==BSM_MAPPED && bsm_tab[bs_id].bs_pid!=-1 && bsm_tab[bs_id].bs_pid!=currpid) // private heap case
    	return SYSERR;

    if(bsm_tab[bs_id].bs_status==BSM_MAPPED)
    	return bsm_tab[bs_id].bs_npages;
    
    /*bsm_tab[bs_id].bs_status=BSM_MAPPED;
    bsm_tab[bs_id].bs_npages=npages;
    bsm_tab[bs_id].bs_vpno=-1;
    bsm_tab[bs_id].bs_pid=-1; // pid in bsm table is set only for private heap--- also in bsm_lookup
    */
    ////private heap check???
    return npages;

}


