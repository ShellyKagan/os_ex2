

#include "utils.h"

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
extern int MAX_THREAD_NUM;
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
  address_t ret;
  asm volatile("xor    %%gs:0x18,%0\n"
               "rol    $0x9,%0\n"
  : "=g" (ret)
  : "0" (addr));
  return ret;
}


#endif

#define SECOND 1000000
#define STACK_SIZE 4096






void write_system_error(std::string const& message)
{
  std::cout << "system error: " << message << std::endl;
}

void write_library_error(std::string const& message)
{
  std::cout << "thread library error: " << message << std::endl;
}

bool is_valid_thread(bool* status_number,int tid,int num_max_thread)
{
  if ((tid < 0) || (tid >= num_max_thread))
  {
    return false;
  }
  return status_number[tid];
}

int get_new_thread_id(bool* status_number,int num_max_thread)
{
  for (int i = 0; i < num_max_thread; i++)
  {
    if (!is_valid_thread (status_number,i,num_max_thread))
    {
      return i;
    }
  }
  return -1;
}

int get_number_of_threads(bool* status_number,int num_max_thread)
{
  int total = 0;
  for (int i = 0; i < num_max_thread; i++)
  {
    if (is_valid_thread (status_number,i,num_max_thread))
    {
      total++;
    }
  }
  return total;
}



void setup_thread(sigjmp_buf& env, char *stack,
                  thread_entry_point entry_point)
{
  // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
  // siglongjmp to jump into the thread.
  address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
  address_t pc = (address_t) entry_point;
  sigsetjmp(env, 1);
  (env->__jmpbuf)[JB_SP] = translate_address(sp);
  (env->__jmpbuf)[JB_PC] = translate_address(pc);
  sigemptyset(&env->__saved_mask);
}


int get_and_pop_next_thread_to_run(std::list<int>& ready_queue)
{
  int next_state = ready_queue.front();
  ready_queue.pop_front();
  return next_state;
}





















