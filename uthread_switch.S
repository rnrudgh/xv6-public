	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
  pushal

  /* regist eax current thread */
  /* thread's stack pointer equal to thread address*/
  movl  current_thread, %eax
  movl  %esp, (%eax)

  movl next_thread, %eax

// switch next_thread to current_thread

  movl %eax, current_thread

  movl (%eax), %esp

    
  // next_thread initialize
  movl $0, next_thread

  // pop all register
  popal

	ret				/* pop return address from stack */
