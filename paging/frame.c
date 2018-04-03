/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
fr_map_t frm_tab[NFRAMES];
extern int grpolicy();

second_chance_queue *sc_queue_head;
second_chance_queue *sc_queue_tail;
second_chance_queue *sc_clock;
void sc_enqueue(second_chance_queue *newframe){
  if(sc_queue_head==NULL){
    sc_queue_head=newframe;
    sc_queue_tail=newframe;
    sc_clock=sc_queue_head;
  }
  else{
    sc_queue_tail->next=newframe;
    sc_queue_tail=newframe;
  }
}
void sc_remove(int frame_id){
  second_chance_queue *ptr=sc_queue_head;
  second_chance_queue *garbage;
//kprintf("remove %d\n",frame_id);
  if(sc_queue_head==NULL)
    return;
  if(sc_queue_head->frame_id==frame_id){
    garbage=sc_queue_head;
    sc_queue_head=sc_queue_head->next;
    freemem((second_chance_queue *)garbage,sizeof(second_chance_queue));
    return;
  }
  while(ptr->next!=NULL && ptr->next->frame_id!=frame_id){
    ptr=ptr->next;
  }
  if(ptr->next==NULL)
    return;
//kprintf("removing %d\n",ptr->next->frame_id);
  if(ptr->next==sc_queue_tail){
    sc_queue_tail=ptr;
  }
  garbage=ptr->next;
  ptr->next=ptr->next->next;
  freemem((second_chance_queue *)garbage,sizeof(second_chance_queue));
}

int get_frame_sc();


aging_queue *aging_head;
aging_queue *aging_tail;

void aging_enqueue(aging_queue *newframe){
  if(aging_head==NULL){
    aging_head=newframe;
    aging_tail=newframe;
  }
  else{
    aging_tail->next=newframe;
    aging_tail=newframe;
  }
}

void aging_remove(int frame_id){
  aging_queue *ptr=aging_head;
  aging_queue *garbage;
//kprintf("remove %d\n",frame_id);
  if(aging_head==NULL)
    return;
  if(aging_head->frame_id==frame_id){
    garbage=aging_head;
    aging_head=aging_head->next;
    freemem((aging_queue *)garbage,sizeof(aging_queue));
    return;
  }
  while(ptr->next!=NULL && ptr->next->frame_id!=frame_id){
//kprintf("N %d ",ptr->next->frame_id);
    ptr=ptr->next;
  }
  if(ptr->next==NULL)
    return;
  if(ptr->next==aging_tail){
    aging_tail=ptr;
  }
//kprintf("removing %d\n",ptr->next->frame_id);
  garbage=ptr->next;
  ptr->next=ptr->next->next;
  freemem((aging_queue *)garbage,sizeof(aging_queue));
}
int get_frame_aging();

SYSCALL init_frm()
{
  //kprintf("To be implemented!\n");
  STATWORD ps;
  disable(ps);
  int i;
  sc_queue_head = NULL;
  sc_queue_tail = NULL;
  sc_clock=NULL;
  aging_head=NULL;
  aging_tail=NULL;

  for(i=0;i<NFRAMES;i++){
  	frm_tab[i].fr_status=FRM_UNMAPPED;
    frm_tab[i].fr_pid=-1;
    frm_tab[i].fr_vpno=-1;
    frm_tab[i].fr_refcnt=0;
    frm_tab[i].fr_type=-1;
    frm_tab[i].fr_dirty=0;
  }
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
//  kprintf("To be implemented!\n");
  STATWORD ps;
  disable(ps);
  int i;
  //Search inverted page table for an empty frame. If one exists stop.
  int policy = grpolicy();

  for(i=0;i<NFRAMES;i++){
  	if(frm_tab[i].fr_status==FRM_UNMAPPED){
  		frm_tab[i].fr_pid=currpid;
  		frm_tab[i].fr_status=FRM_MAPPED;
  		*avail=i;
		  restore(ps);
		  return OK;
  	}
  }
  
  //Pick a page to replace. 

  int page;
  if(policy == SC){
    page=get_frame_sc();
  }
  else{
    page=get_frame_age();
  }
  kprintf("Replaced page %d\n",page); 
  //Using the inverted page table, get vp, the virtual page number of the page to be replaced.
  unsigned long vp =  frm_tab[page].fr_vpno;
  //Let a be vp*4096 (the first virtual address on page vp).
  unsigned long a = vp*NBPG;
  //Let p be the high 10 bits of a. Let q be bits [21:12] of a.
  unsigned long p = (a & 0xffc00000)>>22;
  unsigned long q = (a & 0x003ff000)>>12;
  //Let pid be the pid of the process owning vp.
  int pid = frm_tab[page].fr_pid;
  //Let pd point to the page directory of process pid.
  pd_t *pd = (pd_t *) proctab[pid].pdbr;
  //Let pt point to the pid's p-th page table.
  pt_t *pt = (pt_t *) ((pd[p].pd_base)*NBPG);
  //Mark the appropriate entry of pt as not present.
  pt[q].pt_pres = 0;
 
  //If the page being removed belongs to the current process, invalidate the TLB entry for the page vp using the invlpg instruction (see Intel Volume III/II).
  if(pid==currpid){
    asm volatile ( "invlpg (%0)" : : "b"(vp) : "memory" );
  }
 
  //In the inverted page table decrement the reference count of the frame occupied by pt. If the reference count has reached zero, you should mark the appropriate entry in pd as being not present. This conserves frames by keeping only page tables which are necessary.
  frm_tab[(pd[p].pd_base-1024)].fr_refcnt--;
  if(frm_tab[(pd[p].pd_base-1024)].fr_refcnt==0){
    pd[p].pd_pres=0;
kprintf("came here %d",(pd[p].pd_base-1024));
    free_frm((pd[p].pd_base-1024));
  }
 
  //If the dirty bit for page vp was set in its page table you must do the following:
  if(frm_tab[page].fr_dirty==1){
    //Use the backing store map to find the store and page offset within store given pid and a. If the lookup fails, something is wrong. Print an error message and kill the process pid.
    int store,storeoffset;
    if(bsm_lookup(pid,a,&store,&storeoffset)==SYSERR)
    {
      kprintf("Virtual address not in page directory.\n");
      kill(pid);
      restore(ps);
      return SYSERR;
    }  
    //Write the page back to the backing store.  
    char *frameptr=(char *) ((1024+page)*NBPG);
    write_bs(frameptr,store,storeoffset);
  }
  
  frm_tab[page].fr_pid=currpid;
  frm_tab[page].fr_status=FRM_MAPPED;
  *avail=page;
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  STATWORD ps;
  if(i>=1024 || i<0 ||frm_tab[i].fr_status==FRM_UNMAPPED)
  	return SYSERR;
  disable(ps);


  //delete from queues
  if(frm_tab[i].fr_type==FR_PAGE){
    int policy = grpolicy();
    if(policy == SC){
      sc_remove(i);    
    }
    else{
      aging_remove(i);
    }
  }
/*  int s,p;
  char *frameptr=(1024+i)*NBPG;
  unsigned long vaddr=frm_tab[i].fr_vpno*NBPG;
  if(frm_tab[i].fr_status==FRM_MAPPED && frm_tab[i].fr_type==FR_PAGE && frm_tab[i].fr_dirty==1 && frm_tab[i].fr_vpno>=4096 && bsm_lookup(frm_tab[i].fr_pid,vaddr,&s,&p)!=SYSERR){

kprintf("freeing %d %d",s,p);
  	write_bs(frameptr,s,p);
  }
*/
//if(frm_tab[i].fr_type==FR_PAGE ){
//
//kprintf("kill frame %d\n",i);
  frm_tab[i].fr_status=FRM_UNMAPPED;
  frm_tab[i].fr_pid=-1;
  frm_tab[i].fr_vpno=-1;
  frm_tab[i].fr_refcnt=0;
  frm_tab[i].fr_type=-1;
  frm_tab[i].fr_dirty=0;
//}
  restore(ps);
  return OK;
}


int get_frame_sc(){
 
    int pid;
    pd_t *pd;
    pt_t *pt;
    int i,p,q;
    pid = frm_tab[sc_clock->frame_id].fr_pid;
    pd = (pd_t *) proctab[pid].pdbr;
    p=((frm_tab[sc_clock->frame_id].fr_vpno) & 0x000ffc00)>>10;
    pt = (pt_t *) ((pd[p].pd_base)*NBPG);
    q=(frm_tab[sc_clock->frame_id].fr_vpno) & 0x000003ff;

    while(pt[q].pt_acc==1){
      pt[q].pt_acc=0;
//kprintf("%d ",sc_clock->frame_id);
      sc_clock=sc_clock->next;
      if(sc_clock==NULL)
        sc_clock=sc_queue_head;
      pid = frm_tab[sc_clock->frame_id].fr_pid;   
      pd = (pd_t *) proctab[pid].pdbr;
      p=((frm_tab[sc_clock->frame_id].fr_vpno) & 0x000ffc00)>>10;
      pt = (pt_t *) ((pd[p].pd_base)*NBPG);
      q=(frm_tab[sc_clock->frame_id].fr_vpno) & 0x000003ff;
    }

    int fr_id=sc_clock->frame_id;
//kprintf("Removing %d\n ",fr_id);
    sc_clock=sc_clock->next;
    if(sc_clock==NULL)
        sc_clock=sc_queue_head;
    //sc_remove(fr_id);
    return fr_id;

}

int get_frame_age(){
    int pid;
    pd_t *pd;
    pt_t *pt;
    int i,p,q;
    
    aging_queue *ptr=aging_head;
    pid = frm_tab[ptr->frame_id].fr_pid;
    pd = (pd_t *) proctab[pid].pdbr;
    p=((frm_tab[ptr->frame_id].fr_vpno) & 0x000ffc00)>>10;
    pt = (pt_t *) ((pd[p].pd_base)*NBPG);
    q=(frm_tab[ptr->frame_id].fr_vpno) & 0x000003ff;
    
    int min=256;
    int min_frame;

    while(ptr!=NULL){
      ptr->counter=(ptr->counter>>1 | (pt[q].pt_acc <<7));
//kprintf("%d:%d ",ptr->frame_id,ptr->counter);
      if(ptr->counter<min){
        min=ptr->counter;
        min_frame=ptr->frame_id;
      }
      ptr=ptr->next;
      pid = frm_tab[ptr->frame_id].fr_pid;
      pd = (pd_t *) proctab[pid].pdbr;
      p=((frm_tab[ptr->frame_id].fr_vpno) & 0x000ffc00)>>10;
      pt = (pt_t *) ((pd[p].pd_base)*NBPG);
      q=(frm_tab[ptr->frame_id].fr_vpno) & 0x000003ff;
    }
    if(min<256){
      aging_remove(min_frame);
      aging_queue  *newagingframe;
      newagingframe=(aging_queue *) getmem(sizeof(aging_queue));
      newagingframe->frame_id=min_frame;
      newagingframe->counter=0;
      newagingframe->next=NULL;
    //  aging_enqueue(newagingframe);

      return min_frame;
    }
    return -1;
}
