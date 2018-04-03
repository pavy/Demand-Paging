/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
           /* 1) store error code in a global variable */
           popl pferrcode
           /* 2) store flag register */
           pushfl
           /* 3) clear all interrupts */
           cli
           /* 4) save all general registers */
           pushal
           /* 5) page fault handling */
           call pfint
           /* 6) restore general registers */
           popal
           /* 7) restore flag register */
           popfl
           /* 8) iret */
           iret