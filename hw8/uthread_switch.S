	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register, which should be caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	/* TODO: chky */
	/* save current thread */
	pushal
	movl current_thread, %eax
	movl %esp, (%eax)

	/* switch to next thread */
	movl next_thread, %eax
	movl %eax, current_thread
	movl (%eax), %esp
	popal

	/* chky end */
	ret				/* pop return address from stack */
