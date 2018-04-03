/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
extern void sc_enqueue(second_chance_queue *);
extern void aging_enqueue(aging_queue *);
extern int grpolicy();

SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);
  /*
A page fault indicates one of two things: the virtual page on which the faulted address exists is not present or the page table which contains the entry for the page on which the faulted address exists is not present. To deal with a page fault you must do the following:
Get the faulted address a.
*/
  unsigned long a = read_cr2();
/*
Let vp be the virtual page number of the page containing the faulted address.
*/
  unsigned long vp =  (a & 0xfffff000)>>12;
/*
Let pd point to the current page directory.
*/
  pd_t *pd = proctab[currpid].pdbr;
/*
Check that a is a legal address (i.e. that it has been mapped in pd). a that triggered the page-fault interrupt will only be either mapped in some backing stores or illegal. If it is not mapped to backing store, print an error message and kill the process.
*/
  int s,o;
  if(bsm_lookup(currpid,a,&s,&o)==SYSERR){
  	kprintf("Virtual address not in page directory pid %d vp 0x%08x.\n",currpid,a);
  	kill(currpid);
  	restore(ps);
  	return SYSERR;
  }
/*
Let p be the upper ten bits of a. It represents the offset in page directory.
*/
  unsigned long p =  (a & 0xffc00000)>>22;
/*
Let q be the bits [21:12] of a. It represents the offset in page table where the virtual page is located.
*/
  unsigned long q =  (a & 0x003ff000)>>12;
/*
Let pt point to the p-th page table. If the p-th page table does not exist, obtain a frame for it and initialize it.
*/
  int frame_id;
  pt_t *pt;
  if(pd[p].pd_pres==0){
    get_frm(&frame_id);
	//kprintf("%d",frame_id);
  	frm_tab[frame_id].fr_vpno=1024+frame_id;
  	frm_tab[frame_id].fr_type=FR_TBL;
  	frm_tab[frame_id].fr_dirty=1; 
  
  	pd[p].pd_base=1024+frame_id; 
  	pd[p].pd_pres=1;
  	int j;
  	pt=(pd[p].pd_base)*NBPG;
    	for(j=0;j<1024;j++){
            pt[j].pt_base=0;
            pt[j].pt_pres=0;
            pt[j].pt_write=1;
            pt[j].pt_user=0;
            pt[j].pt_pwt=0;
            pt[j].pt_pcd=0;
            pt[j].pt_acc=0;
            pt[j].pt_dirty=0;
            pt[j].pt_mbz=0;
            pt[j].pt_global=0;
            pt[j].pt_avail=0;
          }

  }
  else
	  pt=(pd[p].pd_base)*NBPG;




//To page in the faulted page do the following:
  /*
  Using the backing store map, find the store s and page offset o which correspond to vp.
  -- This is already found before when we did bsm_lookup
  */
  
  /*
  In the inverted page table, increase the reference count of the frame that holds pt. This indicates that one more of pt's entries is   marked as "present."  
  */  
  frm_tab[(((int)pt/NBPG)-1024)].fr_refcnt++;
  /* =0;
  Obtain a free frame, f.  
  */
  get_frm(&frame_id);//Now frame_id is location of page in physical memory
  int policy = grpolicy();
  second_chance_queue *newframe;
  aging_queue  *newagingframe;

  if(policy == SC){
    newframe=(second_chance_queue *) getmem(sizeof(second_chance_queue));
    newframe->frame_id=frame_id;
    newframe->next=NULL;
    sc_enqueue(newframe);
  }
  else{
    newagingframe=(aging_queue *) getmem(sizeof(aging_queue));
    newagingframe->frame_id=frame_id;
    newagingframe->counter=0;
    newagingframe->next=NULL;
    aging_enqueue(newagingframe);
  }
  frm_tab[frame_id].fr_vpno=vp; // change this
  frm_tab[frame_id].fr_type=FR_PAGE;
  frm_tab[frame_id].fr_dirty=1; //??What should be this? Check

  /*  
  Copy the page o of store s to f.  
  */
  char *frameptr=(1024+frame_id)*NBPG;
  read_bs(frameptr,s,o);

  /*  
  Update pt to mark the appropriate entry as present and set any other fields. Also set the address portion within the entry to point to frame f.
    */
  pt[q].pt_pres=1;
  pt[q].pt_base=1024+frame_id;
  //frm_tab[frame_id].fr_refcnt++;
  restore(ps);
  write_cr3(proctab[currpid].pdbr);
  return OK;
}


