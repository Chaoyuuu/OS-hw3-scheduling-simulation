#include "scheduling_simulator.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>


#define MAX_SIZE 10000
#define L_time 20//ms
#define S_time 10//ms

int in_shell;
int count_PID;
int n_pid;
void shell_mode(void);
void simulate_mode(void);
void myHandler(int signal_number);
void alarm_fun(int a);
int set_timer(int a, int b);

static ucontext_t uc[40];

//Queue
struct nodes{
    char* node_name;  
    char* node_time;	
    char* node_priority;
    char* node_state;
    int Queuing_T;
    int Waiting_T;
    int node_PID;
    struct nodes* node_next;
    struct nodes* node_pre;
};

struct nodes * ready_H_head = NULL;
struct nodes * ready_H_tail = NULL;
struct nodes * ready_L_head = NULL;
struct nodes * ready_L_tail = NULL;
struct nodes * wait_head = NULL;
struct nodes * wait_tail = NULL;
struct nodes * all_head = NULL;
struct nodes * all_tail = NULL;
struct nodes * node_run = NULL;
struct nodes * node_tmp = NULL;
int ready_H = 0;
int ready_L = 0;
int wait_num = 0;
int all_num = 0;

void Push(struct nodes * input_node, int a){
	// printf("in push\n");
	struct nodes * head = NULL;
	struct nodes * tail = NULL;

	//initial + which Q
	if(a == 0){ //push in all
		head = all_head;
		tail = all_tail;
		input_node->node_state = "TASK_READY"; //////////////////////////////task_readya
	}else if(a == 1){//push in H/L
		input_node->node_state = "TASK_READY";

		if(strcmp(input_node->node_priority, "H") == 0){
			head = ready_H_head;
			tail = ready_H_tail;
		}else if(strcmp(input_node->node_priority, "L") == 0){
			head = ready_L_head;
			tail = ready_L_tail;
		}else{
			printf("error node_priority\n");
		}
		
	}else if(a == 2){
		head = wait_head;
		tail = wait_tail;
		input_node->node_state = "TASK_WAITING";
	}


	//do push
	if(head == NULL) {	    	
	    	input_node->node_pre = NULL;
	        input_node->node_next = NULL;
	        head = input_node;	        
	        tail = head;

	} else {
	        input_node->node_pre = tail;
	        input_node->node_next = NULL;
	        tail->node_next = input_node;
	        tail = input_node;
	}

	if(a == 0){ //push in all
		all_head = head;
		all_tail = tail;
		all_num++;
		
	}else if(a == 1){//push in H/L
		if(strcmp(input_node->node_priority, "H") == 0){
			ready_H_head = head;
			ready_H_tail = tail;
			ready_H++;
		}else if(strcmp(input_node->node_priority, "L") == 0){
			ready_L_head = head;
			ready_L_tail = tail;
			ready_L++;
		}else{
			printf("error node_priority\n");
		}
		
	}else if(a == 2){
		wait_head = head;
		wait_tail = tail;
		// printf("wait head = %s, %d\n", wait_head->node_name, wait_head->node_PID);
		// printf("wait tail = %s, %d\n", wait_tail->node_name, wait_tail->node_PID);
		wait_num++;
	}	
}

void Pop(void)
{
	// printf("++++in Pop ");
	if(ready_H != 0){
		node_run = ready_H_head;
		if(ready_H == 1){
			ready_H_head = NULL;
			ready_H_tail = NULL;
		}else{
			ready_H_head = node_run->node_next;
			ready_H_head->node_pre = NULL;
		}
	    ready_H--;
	}else if(ready_L != 0){
		node_run = ready_L_head;
		// printf("1\n");
		if(ready_L == 1){
			ready_L_head = NULL;
			ready_L_tail = NULL;
			// printf("2\n");
		}else{
			ready_L_head = node_run->node_next;
			node_run->node_next->node_pre = NULL;
			// printf("3\n");

		}
		// printf("4\n"); 
	    ready_L--;
	}else{
		// printf("nothing in H/L queue\n");
		node_run = NULL;
	}

	// printf(": %s, ready_H = %d\n", node_run->node_name, ready_H);
    //printf("pid = %d, result = %s, time = %s, prtority = %s\n",node_run->node_PID, node_run->node_name, node_run-> node_time,  node_run-> node_priority);
}

void Remove(int PID){
	// printf("in remove func\n");

	int flag = 0;
	char tmp[20];
	char tmp1[5];
	memset(tmp, 0, sizeof(tmp));
	memset(tmp1, 0, sizeof(tmp1));
	//remove all
	struct nodes * ptr1 = all_head;
	int a = all_num;
	while(a != 0){
		if(ptr1->node_PID == PID){
			break;
		}
		a--;
		ptr1 = ptr1->node_next;
	}
	// printf("%d, ptr: %d\n",PID, ptr1->node_PID );
	strcat(tmp, ptr1->node_state);
	strcat(tmp1, ptr1->node_priority);
	// printf("State = %s, pri = %s\n", tmp, tmp1);

	if(PID == n_pid){
		flag = 1;
		//resechedule
		Pop();
		// printf(" = = %d\n", node_run->node_PID);

	}else if(strcmp(ptr1->node_state, "TASK_TERMINATED") == 0){
		flag = 1;
	}
	// printf("hii\n");
	if(ptr1->node_PID == PID){
		//do remove
		if(ptr1->node_pre == NULL && ptr1->node_next != NULL){
			//is the head, delete
			//printf("hii\n");
			all_head = ptr1->node_next;
			ptr1->node_next->node_pre = NULL;

		}else if(ptr1->node_pre == NULL && ptr1->node_next == NULL){
			//only one node
			//printf("only one node\n");
			all_head = NULL;
			all_tail = NULL;
			
		}else if(ptr1->node_next == NULL){
			//is the tail, delete
			ptr1->node_pre->node_next = NULL;
			all_tail = ptr1->node_pre;

		}else{  
			//middle node
			ptr1->node_pre->node_next = ptr1->node_next;
			ptr1->node_next->node_pre = ptr1->node_pre;
		}
		free(ptr1);
		all_num--;
		//printf("end remove\n");
	}
	

	if(flag != 1){

		//remove L/H
		// printf("in flag\n");
		struct nodes * ptr =NULL;
		struct nodes * head = NULL;
		struct nodes * tail = NULL;
		int b = 0;
		if(strcmp(tmp, "TASK_READY") == 0){
			if(strcmp(tmp1, "H") == 0){     //ready_H
				ptr = ready_H_head;
				head = ready_H_head;
				tail = ready_H_tail;
				b = ready_H;
				// printf("remove High\n");

			}else if(strcmp(tmp1, "L") == 0){  //ready_L
				ptr = ready_L_head;
				head = ready_L_head;
				tail = ready_L_tail;
				b = ready_L;
				// printf("remove Low\n");

			}else{
				printf("wrong prtority\n");
			}
		}else if(strcmp(tmp, "TASK_WAITING") == 0){  //waiting
			ptr = wait_head;
			head = wait_head;
			tail = wait_tail;
			b = wait_num;
			// printf("remove Wait\n");

		}else{
			printf("wrong prtority\n");
		}

		while(b != 0){
			if(ptr->node_PID == PID){
				break;
			}
			b--;
			ptr = ptr->node_next;
		}

		// printf("ptr * %d, head * %d\n",ptr->node_PID, head->node_PID );

		if(ptr->node_PID == PID){
			//do remove			
			if(ptr->node_pre == NULL && ptr->node_next != NULL){
				//is the head, delete
				head = ptr->node_next;
				ptr->node_next->node_pre = NULL;

			}else if(ptr->node_pre == NULL && ptr->node_next == NULL){
				//only one node
				// printf("only one node\n");
				head = NULL;
				tail = NULL;
					
			}else if(ptr->node_next == NULL){
				//is the tail, delete
				// printf("is tail\n");
				ptr->node_pre->node_next = NULL;
				tail = ptr->node_pre;

			}else{  
				//middle node
				// printf("is middle\n");

				ptr->node_pre->node_next = ptr->node_next;
				ptr->node_next->node_pre = ptr->node_pre;
			}
				// printf("end remove\n");
		}

		if(strcmp(tmp, "TASK_READY") == 0){
			if(strcmp(tmp1, "H") == 0){     //ready_H
				ready_H_head = head;
				ready_H_tail = tail;
				ready_H --;
			}else if(strcmp(tmp1, "L") == 0){  //ready_L
				ready_L_head = head;
				ready_L_tail = tail;
				ready_L --;
			}		
		}else if(strcmp(tmp, "TASK_WAITING") == 0){  //waiting
			wait_head = head;
			wait_tail = tail;
			wait_num --;

		}	
		 free(ptr);
		// free(head);
		// free(tail);
	}

}

void move_wait(int PID){
	struct nodes * ptr = wait_head;
	// printf("in remove wait func\n");
	int a = wait_num;
	// printf("%d, w=%d\n", PID, a);

	while(a != 0){
		if(ptr->node_PID == PID){
			node_tmp = ptr;
			break;
		}
		a--;
		ptr = ptr->node_next;
	}
	
	// printf("Find PID = %d\n", ptr->node_PID);
	if(node_tmp != NULL){
		//do remove
		if(ptr->node_pre == NULL && ptr->node_next != NULL){//is the head, delete
			wait_head = ptr->node_next;
			ptr->node_next->node_pre = NULL;
		}else if(ptr->node_pre == NULL && ptr->node_next == NULL){//only one node
			wait_head = NULL;
			wait_tail = NULL;
		}else if(ptr->node_next == NULL){//is the tail, delete
			ptr->node_pre->node_next = NULL;
			wait_tail = ptr->node_pre;
		}else{ 							//middle node
			ptr->node_pre->node_next = ptr->node_next;
			ptr->node_next->node_pre = ptr->node_pre;
		}
		//free(ptr1);
		wait_num--;
		//printf("end remove\n");
	}else{
		printf("no pid\n");
	}
	
}

void change_state(int a, int b){ 
	// printf("in change state\n");
	struct nodes * ptr = all_head;
	
	if(b == 10){  //node_run
		while(ptr->node_PID != node_run->node_PID){
			ptr = ptr->node_next;		
		}
		if(a == 0){ //running state
			node_run->node_state = "TASK_RUNNING";
			ptr->node_state = "TASK_RUNNING";
		}else if(a == 1){//waiting state			
			node_run->node_state = "TASK_WAITING";
			ptr->node_state = "TASK_WAITING";
		}else if(a == 2){//ready state
			node_run->node_state = "TASK_READY";
			ptr->node_state = "TASK_READY";
		}else if(a == 3){//terminate state
			node_run->node_state = "TASK_TERMINATED";
			ptr->node_state = "TASK_TERMINATED";
		}else{
			printf("no state\n");
		}
	}else if(b == 15) {//node_tmp
		while(ptr->node_PID != node_tmp->node_PID){
			ptr = ptr->node_next;
		}
		//waiting to ready
		node_tmp->node_state = "TASK_READY";
		ptr->node_state = "TASK_READY";
	}
	 // printf("end change\n");
}



void Status(void){
	
	struct nodes * ptr = all_head;
	// printf("all = %d\n", all_num);
	int c = all_num;
	if(c == 0){
		// printf("no node in Q\n");
	}

	while(c != 0){
		printf("%-2d %s %-14s %-4d %s %s \n", ptr->node_PID, ptr->node_name, ptr->node_state, ptr->Queuing_T, ptr->node_priority, ptr->node_time);
		c--;
		ptr = ptr->node_next;

	}

	//test
	/*
	printf("--------------------\n");
	struct nodes * ptr1 = ready_H_head;
	// printf("%d\n", ready_H);
	int b = ready_H;
	if(b == 0){
		printf("no node in Q\n");
	}
	while(b != 0){
		//printf("hiiiiiii\n");
		printf("%-2d %s %-14s %3d %s %s \n", ptr1->node_PID, ptr1->node_name, ptr1->node_state, ptr1->Queuing_T, ptr1->node_priority, ptr1->node_time);
		b--;
		ptr1 = ptr1->node_next;
	}
	printf("--------------------\n");
	struct nodes * ptr4 = ready_L_head;
	// printf("%d\n", ready_H);
	int d = ready_L;
	if(d == 0){
		printf("no node in Q\n");
	}
	while(d != 0){
		//printf("hiiiiiii\n");
		printf("%-2d %s %-14s %3d %s %s \n", ptr4->node_PID, ptr4->node_name, ptr4->node_state, ptr4->Queuing_T, ptr4->node_priority, ptr4->node_time);
		d--;
		ptr4 = ptr4->node_next;
	}
	
	printf("--------------------\n");
	struct nodes * ptr2 = wait_head;
	int a = wait_num;
	//printf("%d\n", a);
	if(a == 0)
		printf("no node in Q\n");
	while(a != 0){
		a--;
		sleep(1);
		printf("%d %s %-14s %2d %s %s \n", ptr2->node_PID, ptr2->node_name, ptr2->node_state, ptr2->Waiting_T, ptr2->node_priority, ptr2->node_time);
		ptr2 = ptr2->node_next;

	}
	// printf("end\n");
	*/
}

void hw_suspend(int msec_10)
{
	set_timer(0, 78);
	//running to waiting
	struct nodes* s = (struct nodes*)malloc(sizeof(struct nodes));
	s = node_run;
	s->Waiting_T = msec_10*10;
	Push(s, 2);
	// printf("num in suspend = %d\n", wait_num);
	change_state(1, 10);
	swapcontext(&uc[node_run->node_PID], &uc[0]);
	//reschedule
	//int time = msec_10*10;
	//goto ready
	return;
}


void hw_wakeup_pid(int pid)
{
	//waiting to ready
	// printf("in wakeup pid\n");
	// Status();
	move_wait(pid);
	// printf("in wakeup_pid, %d will be removed\n", node_tmp->node_PID);

	Push(node_tmp, 1);
	change_state(2, 15);
	node_tmp = NULL;
	//if runing -> error
	//reschedule if needed
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	//change all the task with task_name from waiting to ready
	//return how many task is wake up
    //reschedule if needed
    // printf("in wake up task name : %s\n", task_name);

    int num_wakeup_task = 0;
    int arr[20];
	struct nodes * ptr = wait_head;
	int a = wait_num;
	while(a != 0){
		// printf("%d %s %s\n", ptr->node_PID, ptr->node_state, ptr->node_name);
		if(strcmp(ptr->node_name, task_name) == 0 ){
			//remove the node to ready
			num_wakeup_task ++;
			arr[num_wakeup_task] = ptr->node_PID;
			// printf("name = %s, pid = %d, arr[%d]\n",ptr->node_name, ptr->node_PID, arr[num_wakeup_task] );						
		}
		a--;
		ptr = ptr->node_next;
	}

	int ans = num_wakeup_task;
	while(num_wakeup_task != 0){
		move_wait(arr[num_wakeup_task]);
		Push(node_tmp, 1);
		change_state(2, 15);
		node_tmp = NULL;
		num_wakeup_task--;
	}

    return ans;

}

int hw_task_create(char *task_name)
{
	//create task
	// printf("in task create, name = %s\n", task_name);

	count_PID++;
	struct nodes* r = (struct nodes*)malloc(sizeof(struct nodes));
	struct nodes* r1 = (struct nodes*)malloc(sizeof(struct nodes));
	
	char * t = malloc(sizeof(char));
	char * p = malloc(sizeof(char));
	char * n = malloc(10*sizeof(char));
	char * t1 = malloc(sizeof(char));
	char * p1 = malloc(sizeof(char));
	char * n1 = malloc(10*sizeof(char));

	t = "S";
	p = "L";
	r->node_time = t;
	r->node_priority = p;
	r->node_PID = count_PID;

	t1 = "S";
	p1 = "L";
	r1->node_time = t1;
	r1->node_priority = p1;
	r1->node_PID = count_PID;
			
	if(strcmp(task_name, "task1") == 0){
		n = "task1";
		n1 = "task1";
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task1, 0);
	}else if(strcmp(task_name, "task2") == 0){
		n = "task2";
		n1 = "task2";
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task2, 0);
	}else if(strcmp(task_name, "task3") == 0){
		n = "task3";
		n1 = "task3";
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task3, 0);
	}else if(strcmp(task_name, "task4") == 0){
		n = "task4";
		n1 = "task4";	
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task4, 0);
	}else if(strcmp(task_name, "task5") == 0){
		n = "task5";
		n1 = "task5";	
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task5, 0);
	}else if(strcmp(task_name, "task6") == 0){
		n = "task6";
		n1 = "task6";
		getcontext(&uc[count_PID]);
		makecontext(&uc[count_PID], task6, 0);
	}else{
		return -1;
	}

	r->node_name = n;
	r1->node_name = n1;
    //printf("pid = %d, result = %s, time = %s, prtority = %s\n",r->node_PID, r->node_name, r-> node_time, r-> node_priority);
	Push(r, 1);	
	Push(r1, 0);
	return count_PID;
	//return PID is create else return -1
    //reschedule if needed
}

void Time_queuing(void){
	// printf("in time Queuing\n");
	//printf("node_PID, %d\n", node_run->node_PID);
	struct nodes * ptr = ready_H_head;
	int num = ready_H;
	// printf("high num = %d\n", num);
	while(num != 0){
		num--;
		// printf("h high\n");
		// printf("%d \n", ptr->node_PID);
		ptr->Queuing_T = ptr->Queuing_T + 10;
		ptr = ptr->node_next;
	}

	// printf("hi time\n");

	struct nodes * ptr1 = ready_L_head;
	int num1 = ready_L;
	
	while(num1 != 0){
		num1--;
		// printf("h low\n");

		ptr1->Queuing_T = ptr1->Queuing_T + 10;
		ptr1 = ptr1->node_next;
	}
	// printf("hi time\n");

	struct nodes * ptr2 = all_head;
	int num2 = all_num;
	// printf("%d\n", node_run->node_PID);
	while(num2 != 0){
		num2--;
		// printf("h all\n");
		//printf("%d %s\n", ptr2->node_PID, ptr2->node_state);
		if(strcmp(ptr2->node_state, "TASK_READY") == 0 ){//&& ptr2->node_PID != node_run->node_PID){  
			// printf("++\n");
			ptr2->Queuing_T = ptr2->Queuing_T + 10;
		}		

		ptr2 = ptr2->node_next;
	}
	// printf("hi time\n");

	struct nodes * ptr3 = wait_head;
	int num3 = wait_num;
	int zero_pid = 0;
	while(num3 != 0){
		num3--;
		ptr3->Waiting_T = ptr3->Waiting_T - 10;
		if(ptr3->Waiting_T == 0){
			zero_pid = ptr3->node_PID;
		}
		ptr3 = ptr3->node_next;
	}

	if(zero_pid != 0){
		// printf("------------------------zero up\n");
		move_wait(zero_pid);
		Push(node_tmp, 1);
		change_state(2, 15);
		node_tmp = NULL;
	}
}

int main(int argc, char *argv[])
{
	//initialize context
	char *sstack = (char *)malloc(10000);
	uc[0].uc_stack.ss_sp = sstack;
	uc[0].uc_stack.ss_size = sizeof(sstack);
	uc[0].uc_stack.ss_flags = 0;
	uc[0].uc_link = NULL;
	// getcontext(&uc[0]);
	makecontext(&uc[0], simulate_mode, 0);

	char *tstack = (char *)malloc(10000);
	uc[40].uc_stack.ss_sp = tstack;
	uc[40].uc_stack.ss_size = sizeof(tstack);
	uc[40].uc_stack.ss_flags = 0;
	uc[40].uc_link = &uc[0]; ////////////////////////NULL
	// getcontext(&uc[40]);
	makecontext(&uc[40], shell_mode, 0);

	for(int i=1; i<40; i++){
		char *cstack = (char *)malloc(10000);
		uc[i].uc_stack.ss_sp = cstack;
		uc[i].uc_stack.ss_size = sizeof(cstack);
		uc[i].uc_stack.ss_flags = 0;		
		uc[i].uc_link = &uc[0];		
	}
	//Ctrl+Z signal
	signal(SIGTSTP, myHandler);

	//shell_mode();

	simulate_mode();	
	// swapcontext(&uc[0], &uc[40]);
	return 0;
}
void simulate_mode(void){
	getcontext(&uc[0]);
	while(1){

		// printf("simualting ...\n");
		
		if(ready_L == 0 && ready_H == 0 && wait_num == 0){  //nothing in simualtion
			//nothing in Q, 
			// 1. check waiting ?= 0 ,go back to shell
			printf("nothing in Q, go back to shell\n");
			shell_mode();
			// swapcontext(&uc[0], &uc[40]);

		}else if(ready_L == 0 && ready_H == 0 && wait_num != 0){  //nothing in H/L, somthing in waiting
			Time_queuing();
		}else{ //pop somthing
			// printf("pop somthing !\n");
			Pop();
			n_pid = node_run->node_PID;
			change_state(0, 10);
			// printf("pid %d, is going to run ! state = %s  \n",node_run->node_PID, node_run->node_state);
			Time_queuing();

			set_timer(1, 00);//timer
			swapcontext(&uc[0], &uc[node_run->node_PID]);
			set_timer(0, 45);
			// printf("end run !\n");

			if(in_shell == 1){
				in_shell = 0;
				Push(node_run, 1);
				change_state(2, 10);
				// printf("node_run Status = %s\n", node_run->node_state);
			}

			if(strcmp(node_run->node_state, "TASK_RUNNING") == 0 ){
				// printf("gg\n");
				change_state(3, 10);

			}
			// printf("huu\n");
		}
		// printf("end of while\n\n");		
	}
}

void shell_mode(void){
	getcontext(&uc[40]);
	set_timer(0, 00);
	// printf("in shell\n");

	char bufff[100];
	while(1){		

		printf("$");
		fgets(bufff, 100, stdin);
		// printf("bufff = %s\n", bufff);//add Task1 -t H -p L

		//cut \n
		char* delim_n = "\n";
		char* buf = NULL;
		buf = strtok(bufff, delim_n);
		
		if(strncmp(buf, "add", 3) == 0){
			// printf("command add\n");

			//cut buf
			char* delim_space = " ";
			char* task_name_input = NULL; 
			char* time_input = NULL;
			char* priority_input = NULL;

			strtok(buf, delim_space);
			task_name_input = strtok(NULL, delim_space);
			strtok(NULL, delim_space);
			time_input = strtok(NULL, delim_space);
			strtok(NULL, delim_space);
			priority_input = strtok(NULL, delim_space);


			// printf("task_name_input = %s\n", task_name_input);
			// printf("time_input = %s\n", time_input);
			// printf("priority_input = %s\n", priority_input);

			//input default
			if(time_input == NULL){
				time_input = "S";
			}
			if(priority_input == NULL){
				priority_input = "L";  ////////////////////////////////////////////////////////////////////////////
			}

			//push Q
			char *tmp = malloc(10*sizeof(char));
			memset(tmp, 0, 10*sizeof(char));
			// strcat(tmp, task_name_input);

			char *tmp2 = malloc(10*sizeof(char));
			memset(tmp2, 0, 10*sizeof(char));
			strcat(tmp2, time_input);

			char *tmp3 = malloc(10*sizeof(char));
			memset(tmp3, 0, 10*sizeof(char));
			strcat(tmp3, priority_input);

			count_PID++;

			//printf("tmp = %s\n",tmp );
			//do node_uc
			if(strcmp(task_name_input, "Task1") == 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task1, 0);
				strcat(tmp, "task1");
				// printf("hiii context t1\n");

			}else if(strcmp(task_name_input, "Task2")== 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task2, 0);
				strcat(tmp, "task2");
				// printf("hiii context t2\n");

			}else if(strcmp(task_name_input, "Task3")== 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task3, 0);
				strcat(tmp, "task3");

			}else if(strcmp(task_name_input, "Task4")== 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task4, 0);
				strcat(tmp, "task4");
			
			}else if(strcmp(task_name_input, "Task5")== 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task5, 0);	
				strcat(tmp, "task5");			

			}else if(strcmp(task_name_input, "Task6")== 0){
				getcontext(&uc[count_PID]);
				makecontext(&uc[count_PID], task6, 0);
				strcat(tmp, "task6");
			}

			//ready_H/L Queue
			struct nodes* q = (struct nodes*)malloc(sizeof(struct nodes));
			q->node_name = tmp;
			q->node_time = tmp2;
			q->node_priority = tmp3;
			q->node_PID = count_PID;
			q->Queuing_T = 0;
			q->Waiting_T = 0;

			struct nodes* q1 = (struct nodes*)malloc(sizeof(struct nodes));
			q1->node_name = tmp;
			q1->node_time = tmp2;
			q1->node_priority = tmp3;
			q1->node_PID = count_PID;
			q1->Queuing_T = 0;
			q1->Waiting_T = 0;


			Push(q, 1); //push in L/H
			Push(q1, 0); //push in all


		}else if(strncmp(buf, "start", 5) == 0){
			// printf("command start\n");
			//do running state			
			// return ;
			//set_timer(1, 19);
			setcontext(&uc[0]);
			// swapcontext(&uc[40], &uc[0]);

		}else if(strncmp(buf, "remove", 6) == 0){
			// printf("command remove\n");

			char* delim_space = " ";
			char* task_PID = NULL;

			strtok(buf, delim_space);
			task_PID = strtok(NULL, delim_space);
			int PID = atoi(task_PID);
			//printf("PID = %d\n",PID );
			Remove(PID);

		}else if(strncmp(buf, "ps", 2) == 0){
			// printf("command ps\n");
			//print status
			Status();
			//Pop();
		}
	}
}

void myHandler(int signal_number)
{
    // printf("\nYou pressed Ctrl+Z\n");
    in_shell = 1;
    //swapcontext(&uc[node_run->node_PID], &uc[40]);
    setcontext(&uc[40]);
    //stop timer
}		


void alarm_fun(int a){
	// printf("\nin alarm!\n");

	//time out, Push to ready
	Push(node_run, 1);

	change_state(2, 10);

	// printf("end alarm\n\n");
	swapcontext(&uc[node_run->node_PID], &uc[0]);
}


int set_timer(int a, int b){
	struct itimerval value;
	signal(SIGALRM, alarm_fun);

	if(a == 1){
		//printf("in timer 3, %d\n", b);
		if(strcmp(node_run->node_time, "L") == 0){
			value.it_value.tv_sec = 0;	
			value.it_value.tv_usec = 20000;
		}else{
			value.it_value.tv_sec = 0;	
			value.it_value.tv_usec = 10000;
		}
	}else if(a == 0){
		//printf("in timer 0, %d\n", b);

		value.it_value.tv_usec = 0;
		value.it_value.tv_usec = 0;
	}


	//value.it_value.tv_sec = 0;	
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &value, NULL);

	return 0;
}

