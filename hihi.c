#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>

ucontext_t childd[10];
int running;
ucontext_t child, main1;

void func1(int a){
    printf("Func1\n");
	swapcontext(&main1, &child);
}

void func2(){
    printf("Func2\n");
  	setcontext(&main1);
}
int main()
{
	char stack[1024*128];                           //128kB stack

	signal(SIGTSTP, func1);
    getcontext(&child);                   //獲取現在的context
	child.uc_stack.ss_sp=stack;                     //指定stack
	child.uc_stack.ss_size=sizeof(stack);           //指定stack大小
	child.uc_stack.ss_flags=0;
	child.uc_link=&main1;                            //執行完後回到main
	makecontext(&child,(void (*)(void))func2,0);    //修改context指向func1

	printf("aa\n");
	int a = 0;
	//method 1
	func1(2);
	//method 2
	while(a != 3)
		scanf("%d", &a);
	printf("aaa\n");
	return 0;
}