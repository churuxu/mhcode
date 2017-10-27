#include <stdio.h>
#include <windows.h>

#include "mhcode.h"
#include "detours.h"

#ifdef _MSC_VER
#define NOINLINE  __declspec(noinline)
#endif



int DetourAttachAddress(PVOID* target, mhcode_context_handler func, PVOID* outctx, void* udata) {
	LONG ret = 0;
	int offset = 0;
	PDETOUR_TRAMPOLINE trampoline = NULL;
	void* code = mhcode_malloc(64);
	offset = mhcode_make_context_handler(code, func, udata);
	ret = DetourAttachEx(target, code, &trampoline, NULL, NULL);
	if (ret)return ret;
	mhcode_make_jmp((char*)code + offset, trampoline);
	*outctx = code;
	return 0;
}

void DetourDetachAddress(PVOID* target, PVOID ctx) {
	DetourDetach(target, ctx);
	mhcode_free(ctx);
}




NOINLINE static void __cdecl CdeclFunction(int a, float b, char* c) {
	printf("CdeclFunction\t\t(%d, %f, '%s')\n", a, b, c);
}


static void FunctionHooked(void* ctx, void* udata) {
	int v1 = (int)mhcode_get_stack_value(ctx, 4);
	intptr_t iv2 = mhcode_get_stack_value(ctx, 8);
	char* v3 = (char*)mhcode_get_stack_value(ctx, 12);
	float* pv2 = (float*)&iv2;
	float v2 = *pv2;
	printf("get value in hooked function: %d %f %s udata:%p\n", v1, v2, v3, udata);
	mhcode_set_stack_value(ctx, 12, (intptr_t)"hooked");
}


static void* hookctx_;
static void* oldfunc_;

void Hook() {
	oldfunc_ = (void*)CdeclFunction;

	
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourAttachAddress((PVOID*)&oldfunc_, FunctionHooked, &hookctx_, (void*)0x12345678);

	DetourTransactionCommit();	
}

void Unhook() {
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetachAddress((PVOID*)&oldfunc_, hookctx_);

	DetourTransactionCommit();
	
}

int main() {
		
	printf("=====befor hook=====\n");

	CdeclFunction(111, 1.11f, "hello1");
	
	Hook();
	

	printf("\n=====after hook=====\n");
	
	CdeclFunction(111, 1.11f, "hello1");
	
	Unhook();

	printf("\n=====destroy hook=====\n");

	CdeclFunction(111, 1.11f, "hello1");
	
	return 0;
}
