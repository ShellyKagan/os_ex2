/**********************************************
 * Test 1: correct threads ids
 *
 **********************************************/

#include <cstdio>
#include <cstdlib>
#include "../uthreads.h"


#define GRN "\e[32m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

void halt()
{
    while (true)
    {}
}

void wait_next_quantum()
{
    int quantum = uthread_get_quantums(uthread_get_tid());
    while (uthread_get_quantums(uthread_get_tid()) == quantum)
    {}
    return;
}

void thread1()
{
    uthread_block(uthread_get_tid());
}

void thread2()
{
    halt();
}

void error()
{
    printf(RED "ERROR - wrong id returned\n" RESET);
    exit(1);
}

int main()
{
    printf(GRN "Test 1:    " RESET);
    fflush(stdout);

	int q[2] = {10, 20};
	uthread_init(8000);

    if (uthread_spawn(thread1) != 1)
    {
      printf ("1" RESET);
      error ();
    }
    int a=uthread_spawn(thread2);
    if (a != 2)
    {
      printf("%d",a);
      printf ("2" RESET);
      error ();
    }
    if (uthread_spawn(thread2) != 3)
    {
      printf ("3" RESET);
      error ();
    }
    if (uthread_spawn(thread1) != 4)
    {
      printf ("4" RESET);
      error ();
    }
    if (uthread_spawn(thread2) != 5)
    {
      printf ("5" RESET);
      error ();
    }
    if (uthread_spawn(thread1) !=6){
      printf("6" RESET);
      error();
    }
    uthread_terminate(5);
    if (uthread_spawn(thread2) != 5){
        printf("7" RESET);
        error();
    }
    wait_next_quantum();
    wait_next_quantum();

    uthread_terminate(5);
    if (uthread_spawn(thread2) != 5){
      printf("8" RESET);
      error ();
    }
    uthread_terminate(2);
    if (uthread_spawn(thread2) != 2){
      printf("9" RESET);
      error ();
    }
    uthread_terminate(3);
    uthread_terminate(4);
    if (uthread_spawn(thread2) != 3){
      printf("10" RESET);
      error ();
    }
    if (uthread_spawn(thread1) != 4){
      printf("11" RESET);
      error ();
    }
    printf(GRN "SUCCESS\n" RESET);
    uthread_terminate(0);

}
