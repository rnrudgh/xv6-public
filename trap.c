#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

struct spinlock sysprintlock;
uint sysprintstate;

void
tvinit(void)
{
  int i;

  sysprintstate = 0;
  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
  initlock(&sysprintlock, "sysprintlock");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    struct proc *curproc = myproc();
    if(curproc->killed)
      exit();
    curproc->tf = tf;
    syscall();
    if(curproc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
    case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
      
    }
    // this part only to manipulate a process's alarm ticks if there's a process running
    // and if the timer interrupt came from user space
    if(myproc() != 0 && (tf->cs & SEG_UCODE) == SEG_UCODE) {
      myproc()->ticks++;
      if( myproc()->ticks >= myproc()->alarmticks) {

         myproc()->ticks = 0;

         // regist current instruction address for return address on stack memory
         tf->esp -= 4;
         *(uint *)tf->esp = tf->eip;

         // regist trap fram's eip alarmhandler
         // if it go to user space, it execute alarmhandler
         tf->eip = (uint)myproc()->alarmhandler;
      }
    } 

    
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    {
      void* mem = kalloc();


      if (mem == 0) {
        cprintf("allocuvm out of memory\n");
        //deallocuvm(myproc()->pagedir, );
        
       myproc()->killed = 1;
        return ;
      }

      memset(mem, 0 , PGSIZE);
      if(mappages( myproc() -> pgdir ,(void *)PGROUNDDOWN( rcr2() ), PGSIZE, V2P(mem)
          ,PTE_W|PTE_U ) < 0) {
        cprintf("sdfafdafsafsfsfa\n");
        kfree(mem);
       myproc()->killed = 1;
        return ;
       }

       //myproc()->killed = 1;
       break;
    }

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
