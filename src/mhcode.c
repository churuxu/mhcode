#include "mhcode.h"
#ifdef _WIN32
#include <windows.h>
#else

#endif


void* mhcode_malloc(size_t sz) {
	return VirtualAlloc(NULL, sz, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}


void mhcode_free(void* mem) {
	VirtualFree(mem, 0, MEM_RELEASE);
}


int mhcode_mprotect(void* mem, size_t sz, int prot) {
	DWORD oldacc = 0;
	DWORD newacc = 0;
	BOOL bret = 0;
	switch (prot) {
	case (PROT_READ): newacc = PAGE_READONLY; break;
	case (PROT_WRITE): newacc = PAGE_READWRITE; break;
	case (PROT_EXEC): newacc = PAGE_EXECUTE; break;
	case (PROT_READ | PROT_WRITE): newacc = PAGE_READWRITE; break;
	case (PROT_READ | PROT_EXEC): newacc = PAGE_EXECUTE_READ; break;
	case (PROT_WRITE | PROT_EXEC): newacc = PAGE_EXECUTE_READWRITE; break;
	case (PROT_READ | PROT_WRITE | PROT_EXEC): newacc = PAGE_EXECUTE_READWRITE; break;
	default: newacc = PAGE_NOACCESS; break;
	}
	bret = VirtualProtect(mem, sz, newacc, &oldacc);
	if (bret)return 0;
	return GetLastError();
}


int mhcode_memcmp(void* mema, void* memb, size_t len) {
	void* buf = malloc(len * 2);
	void* buf1 = buf;
	void* buf2 = (char*)buf + len;
	SIZE_T len1 = 0;
	SIZE_T len2 = 0;
	BOOL bret;
	bret = ReadProcessMemory(GetCurrentProcess(), mema, buf1, len, &len1);
	bret = bret && ReadProcessMemory(GetCurrentProcess(), memb, buf2, len, &len2);
	if (bret && len1 == len2 && len == len1) {
		return memcmp(buf1, buf2, len1);
	}
	return -1;
}



const unsigned char x86_handler_trampoline_[] = {
	0x60,                   // pushad
	0x9C,                   // pushfd
	0x8B, 0xC4,             // mov         eax,esp
	0x83, 0xC0,0x04,        // add         eax,4
	0x50,                   // push        eax
	0xE8, 0x90,0x90,0x90,0x90,//call       context handler function
	0x58,                   //pop         eax
	0x9D,                   //popfd
	0x61,                   //popad
};

const size_t x86_handler_func_offset_ = 8;

#define handler_trampoline_ x86_handler_trampoline_
#define handler_func_offset_ x86_handler_func_offset_


int mhcode_make_context_handler(void* codebuf, mhcode_context_handler func) {
	memcpy(codebuf, handler_trampoline_, sizeof(handler_trampoline_));
	mhcode_make_call((char*)codebuf + handler_func_offset_, func);
	return sizeof(handler_trampoline_);
}


intptr_t mhcode_get_stack_value(void* context, int offset) {
	mhcode_context_x86* ctx = context;
	char* stackaddr = (char*)(ctx->esp);
	stackaddr += offset;
	return *(intptr_t*)stackaddr;
}

void mhcode_set_stack_value(void* context, int offset, intptr_t value) {
	mhcode_context_x86* ctx = context;
	char* stackaddr = (char*)(ctx->esp);
	stackaddr += offset;
	*(intptr_t*)stackaddr = value;
}

// x86 code
int mhcode_make_jmp(void* addr, void* jmpto) {
	intptr_t offset = (intptr_t)jmpto - (intptr_t)addr - 5;
	unsigned char* buf = (unsigned char*)addr;
	buf[0] = 0xE9;
	*(intptr_t*)(buf + 1) = offset;
	return 5;
}

int mhcode_make_call(void* addr, void* jmpto) {
	intptr_t offset = (intptr_t)jmpto - (intptr_t)addr - 5;
	unsigned char* buf = (unsigned char*)addr;
	buf[0] = 0xE8;
	*(intptr_t*)(buf + 1) = offset;
	return 5;
}


typedef struct _mhcode_hook_data {
	void* target;
	size_t codelen;
	void* trampoline;
	size_t nop;
	size_t nop2;
}mhcode_hook_data;


mhcode_hook_t mhcode_hook_create(void* addr, size_t codelen, mhcode_context_handler func) {
	mhcode_hook_data* result;
	void* trampoline;
	size_t len;
	if (codelen < 5)return NULL;

	result = mhcode_malloc(sizeof(mhcode_hook_data) + sizeof(handler_trampoline_) + codelen + 32);
	if (!result) { return NULL; }
	trampoline = (char*)result + sizeof(mhcode_hook_data);

	mhcode_mprotect(addr, codelen, PROT_READ | PROT_WRITE | PROT_EXEC);

	//1 gen trampoline code
	len = mhcode_make_context_handler(trampoline, func);

	//2. run code of target
	memcpy((char*)trampoline + len, addr, codelen);

	//3. jmp bakc to target + codelen
	mhcode_make_jmp((char*)trampoline + len + codelen, (char*)addr + codelen);

	//modify target code
	//let target jmp to trampoline code
	len = mhcode_make_jmp(addr, trampoline);
	if (codelen > len) {
		memset((char*)addr + len, 0x90, codelen - len);
	}

	result->target = addr;
	result->codelen = codelen;
	result->trampoline = trampoline;
	result->nop = 0x90909090;
	result->nop2 = 0x90909090;
	return result;
}


void mhcode_hook_destroy(mhcode_hook_t hook) {
	mhcode_hook_data* result = (mhcode_hook_data*)hook;

	//restore orgin code
	void* orgincache = (char*)(result->trampoline) + sizeof(handler_trampoline_);
	memcpy(result->target, orgincache, result->codelen);

	//free memory
	mhcode_free(result);
}
