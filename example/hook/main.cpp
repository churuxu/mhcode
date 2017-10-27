#include <stdio.h>

#include "mhcode.h"

#ifdef _MSC_VER
#define NOINLINE  __declspec(noinline)
#endif

NOINLINE static void __cdecl CdeclFunction(int a, float b, char* c) {
	printf("CdeclFunction\t\t(%d, %f, '%s')\n", a, b, c);
}


NOINLINE static void __stdcall StdcallFunction(int a, float b, char* c) {
	printf("StdcallFunction\t\t(%d, %f, '%s')\n", a, b, c);
}

class SomeClass {
public:
	NOINLINE void ThiscallFunction(int a, float b, char* c) {
		printf("(%p)->ThiscallFunction\t(%d, %f, '%s')\n", this, a, b, c);
	}
};


static void FunctionHooked(void* ctx, void* udata) {
	int v1 = (int)mhcode_get_stack_value(ctx, 4);
	intptr_t iv2 = mhcode_get_stack_value(ctx, 8);
	char* v3 = (char*)mhcode_get_stack_value(ctx, 12);
	float* pv2 = (float*)&iv2;
	float v2 = *pv2;
	printf("get value in hooked function: %d %f %s  udata:%p\n", v1, v2, v3, udata);
	mhcode_set_stack_value(ctx, 12, (intptr_t)"hooked");
}


int main() {
	mhcode_hook_t hook1;
	mhcode_hook_t hook2;
	mhcode_hook_t hook3;
	SomeClass c;

	void (SomeClass::* member)(int a, float b, char* c) = &SomeClass::ThiscallFunction;

	//exit(0x12345678);

	printf("=====befor hook=====\n");

	CdeclFunction(111, 1.11f, "hello1");
	StdcallFunction(222, 22.2f, "hello2");
	c.ThiscallFunction(333, 0.333f, "hello3");

	hook1 = mhcode_hook_create(CdeclFunction, 6, FunctionHooked, (void*)0x12345678);
	hook2 = mhcode_hook_create(StdcallFunction, 6, FunctionHooked, (void*)0x87654321);
#ifdef _DEBUG
	hook3 = mhcode_hook_create(*(void**)&member, 11, FunctionHooked, NULL);
#else
	hook3 = mhcode_hook_create(*(void**)&member, 6, FunctionHooked);
#endif
	

	printf("\n=====after hook=====\n");
	
	CdeclFunction(111, 1.11f, "hello1");
	StdcallFunction(222, 22.2f, "hello2");	
	c.ThiscallFunction(333, 0.333f, "hello3");


	mhcode_hook_destroy(hook1);
	mhcode_hook_destroy(hook2);
	mhcode_hook_destroy(hook3);

	printf("\n=====destroy hook=====\n");

	CdeclFunction(111, 1.11f, "hello1");
	StdcallFunction(222, 2.22f, "hello2");
	c.ThiscallFunction(333, 3.33f, "hello3");

	return 0;
}
