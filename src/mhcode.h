#pragma once

#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


	/*
	utility function for mechine code, memory address hook etc.
	*/





	//for mhcode_mprotect()
#ifdef _WIN32
#define PROT_NONE  0
#define PROT_READ  1
#define PROT_WRITE 2
#define PROT_EXEC  4
#endif

	/** alloc memory for read/write/execute, this memory buffer can write mechine code for execute. (memory alloc by malloc can not execute) */
	void* mhcode_malloc(size_t sz);

	/** free memory alloced by mhcode_malloc() */
	void mhcode_free(void* mem);

	/** modify permission of memory */
	int mhcode_mprotect(void* mem, size_t sz, int prot);

	/** compare memory (support no read permision) */
	int mhcode_memcmp(void* mema, void* memb, size_t len);



	/** x86 context */
	typedef struct mhcode_context_x86 {
		intptr_t edi;
		intptr_t esi;
		intptr_t ebp;
		intptr_t esp;
		intptr_t ebx;
		intptr_t edx;
		intptr_t ecx;
		intptr_t eax;
	}mhcode_context_x86;


	/** context passed to hook_function_t */
	typedef mhcode_context_x86 mhcode_context;

	typedef void(__cdecl *mhcode_context_handler)(void* context, void* udata);

	/** make a function to call mhcode_context_handler_t and pass current thread context, return writed code len	
	this codebuf can pass to other hook library	when append code to jmp trampoline
	*/
	int mhcode_make_context_handler(void* codebuf, mhcode_context_handler func, void* udata);

	/** make 'jmp' mechine code, return writed code len */
	int mhcode_make_jmp(void* codebuf, void* jmpto);

	/** make 'call' mechine code, return writed code len */
	int mhcode_make_call(void* codebuf, void* callto);

	/** get stack value in hook function */
	intptr_t mhcode_get_stack_value(void* context, int offset);

	/** set stack value in hook function */
	void mhcode_set_stack_value(void* context, int offset, intptr_t value);


	/** call address as function */
	intptr_t mhcode_call_cdecl(void* addr, int argc, intptr_t* argv);
	intptr_t mhcode_call_stdcall(void* addr, int argc, intptr_t* argv);	
	intptr_t mhcode_call_thiscall(void* addr, void* pthis, int argc, intptr_t* argv);


	typedef void* mhcode_hook_t;

	/** create hook on any address,
	it will replace 'codelen' of mechine code in 'targetaddr'
	replaced code can not be partial of mechine code
	'codelen' must > 5(x86)
	use disassembly tools to known 'codelen' of target
	*/
	mhcode_hook_t mhcode_hook_create(void* targetaddr, size_t codelen, mhcode_context_handler func, void* udata);

	/** destroy hook */
	void mhcode_hook_destroy(mhcode_hook_t hook);

#ifdef __cplusplus
}
#endif
