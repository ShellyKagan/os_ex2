#include <csetjmp>
#include <sys/time.h>
#include <cstdlib>
#include <cstdio>
#include "uthreads.h"
#include <csetjmp>
#include <csignal>
#include <vector>
#include <queue>
#include <algorithm>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>

#define FAIL -1
#define SUCCESS 0

#define JB_SP 6
#define JB_PC 7

typedef void (*sig_handler) (int);

typedef unsigned long address_t;

using namespace std;

enum STATE
{
    RUN, READY, BLOCKED, NOTEXISTS, SLEEPING
};

class Thread
{
 public:
  int _id;
  STATE _state;
  char *_stack;
  thread_entry_point _entry_point;
  int _quantums;
  int _remain_sleep_time; // in quantums
  sigjmp_buf _env;
 public:
  Thread ()
  {
    _id = 0;
    _state = NOTEXISTS;
    _stack = nullptr;
    _entry_point = nullptr;
    _quantums = 1;
    _remain_sleep_time = 0;
  }
  Thread (int id, STATE state, char *stack, thread_entry_point entry_point,
          int quantums, int remain_sleep_time)
  {
    _id = id;
    _state = state;
    _stack = stack;
    _entry_point = entry_point;
    _quantums = quantums;
    _remain_sleep_time = remain_sleep_time;
  }
  ~Thread ()
  {
    delete _stack;
  }
};

Thread * threads[MAX_THREAD_NUM];
vector<int> readies;
vector<int> sleepings;
int running_process_id = 0;
int current_threads_amount = 0;
int total_tick = 0;
int quantum_len = 0;

// ---------------------- jumping ------------------------

void jump_to_thread (int tid)
{
  running_process_id = tid;
  siglongjmp (threads[tid]->_env, 1);
}
void yield(int jump_tid)
{
  int ret_val = sigsetjmp(threads[running_process_id]->_env, 1);
  printf("yield: ret_val=%d\n", ret_val);
  if (ret_val == 0)
  {
    jump_to_thread(jump_tid);
  }
}
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


// ---------------------- inner ------------------------

int is_exists (int tid)
{
  printf ("ASASAS enter function is_exists. running is %d. parameter: %d\n",
          running_process_id, tid);
  if (threads[tid]->_state == NOTEXISTS)
  {
    printf ("thread library error: thread does`nt exists\n");
    printf ("thread library error: thread does`nt exists\n");
    fflush (stderr);
    return FAIL;
  }
  return SUCCESS;
}

/**
 * moves the current process to "current_new_state" and activates the first
 * thread in the ready queue.
 * @param current_new_state the state to which the current state will be
 * transfered
 * @return 0 on success, -1 on fail
 */
// todo: if ready = -1, return error
int schedule (STATE current_new_state)
{
  printf ("ASASAS entered function schedule. running is %d. parameter: %d\n",
          running_process_id, current_new_state);
  fflush (stdout);

  threads[running_process_id]->_state = current_new_state;
  if (current_new_state == READY)
  {
    readies.insert (readies.begin (), running_process_id);
  }
  if (current_new_state != RUN)
  {
    running_process_id = readies.back ();
    readies.pop_back ();
    if (running_process_id == FAIL)
    {
      printf ("thread library error: there are no threads to run\n");
      fflush (stderr);
      return FAIL;
    }
    threads[running_process_id]->_state = RUN;
    jump_to_thread (running_process_id);
  }
  fflush (stdout);
  return SUCCESS;

}

void manage_sleepers ()
{
  printf ("ASASAS enter function manage_sleepers. running is %d. sleepers "
          "amount: %zu\n",
          running_process_id, sleepings.size ());
  fflush (stdout);

  for (auto sleepy: sleepings)
  {
    if (--threads[sleepy]->_remain_sleep_time <= 0)
    {
      if (threads[sleepy]->_state != BLOCKED)
      {
        threads[sleepy]->_state = READY;
        readies.insert (readies.begin (), sleepy);
      }
      sleepings.erase (std::remove (sleepings.begin (), sleepings.end (), sleepy),
                       sleepings.end ());
    }
  }
}

void on_tick (int sig)
{
  printf ("ASASAS enter function on_tick. running is %d\n",
          running_process_id);
  fflush (stdout);

  if (sig == SIGVTALRM)
  {
    printf ("ASASAS enter function on_tick. entered the if\n");
    fflush (stdout);
    threads[running_process_id]->_quantums++;
    manage_sleepers ();
    schedule (READY);
    total_tick++;
  }
}

int set_clock (sig_handler timer_handler, int value, int interval)
{
  struct sigaction sa = {nullptr};
  struct itimerval timer;

  sa.sa_handler = &on_tick;
  if (sigaction (SIGVTALRM, &sa, nullptr) < 0)
  {
    exit (1);
    return -1;
  }

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = value;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = interval;

  if (setitimer (ITIMER_VIRTUAL, &timer, nullptr))
  {
    printf ("system error: setitimer error.\n");
    fflush (stderr);
    exit (1);
    return -1;
  }

  return SUCCESS;
}

/**
 * finds the first free space in the threads array.
 * @return the id which was found on success, -1 otherwise.
 */
int look_for_id ()
{
  printf ("ASASAS enter function look_for_id. running is %d\n",
          running_process_id);
  fflush (stdout);

  for (int i = 1; i < MAX_THREAD_NUM; i++)
  {
    if (threads[i] == nullptr)
    {  // sign for empty
      return i;
    }
  }
  fflush (stdout);
  return -1;
}
/**
 * Display all relevant data structures status for debugging
 */
void display_status(){
  printf ("---------------THREADS ARRAY------------------\n");
  fflush(stdout);
  for(int i = 0; i<MAX_THREAD_NUM; i++){
    printf ("thread id: %d, state: %u, \n", i, threads[i]->_state);
    fflush(stdout);
  }
  printf ("-----------------READY QUEUE----------------\n");
  fflush(stdout);
  cout  << &readies << endl;
  printf ("---------------SLEEPING LIST------------------\n");
  fflush(stdout);
  cout << &sleepings << endl;
}

// --------------------- API ---------------------------


int uthread_init (int quantum_usecs)
{
  set_clock (on_tick, quantum_usecs, quantum_usecs);
  // init threads array
 // Thread default_thread = Thread ();
  // all is null
//  for (auto &thread: threads)
//  {
//    thread = default_thread;
//  } I think there might be a stack and entry_point
  threads[0] = new Thread();
  threads[0]->_state = RUN;
  return SUCCESS;
}


void setup_thread (int tid, char *stack, thread_entry_point entry_point)
{
  // initializes env[tid] to use the right stack, and to run from the function 'entry_point', when we'll use
  // siglongjmp to jump into the thread.
  address_t sp = (address_t) stack + STACK_SIZE - sizeof (address_t);
  address_t pc = (address_t) entry_point;
  sigsetjmp(threads[tid]->_env, 1);
  (threads[tid]->_env->__jmpbuf)[JB_SP] = translate_address (sp);
  (threads[tid]->_env->__jmpbuf)[JB_PC] = translate_address (pc);
  sigemptyset (&threads[tid]->_env->__saved_mask);
}

int uthread_spawn (thread_entry_point entry_point)
{
  {
    printf ("ASASAS enter function uthread_spawn. running is %d\n",
            running_process_id);
    // initialization & error checking
    if (current_threads_amount > MAX_THREAD_NUM || entry_point == nullptr)
    {
      return FAIL;
    }
    int id = look_for_id ();
    if (id == FAIL)
    {
      return FAIL;
    }

    char *stack = new char[sizeof (stack) * STACK_SIZE];

    // create the new thread and pushes it to the ready queue
    threads[id] = new Thread (id, READY, stack, entry_point, 1, 0);
    setup_thread (id, stack, entry_point);
    current_threads_amount++;
    readies.insert (readies.begin (), id);
    return SUCCESS;
  }
}

int uthread_terminate (int tid)
{
  printf ("ASASAS enter function uthread_terminate. running is %d, parameter "
          "is %d"
          "\n",
          running_process_id, tid);
  if (is_exists (tid) == FAIL)
  {
    return FAIL;
  }

  // delete from ready queue
  if (threads[tid]->_state == READY)
  {
    readies.erase (std::remove (readies.begin (), readies.end (), tid), readies.end ());
  }

  // delete from sleeping list
  if (threads[tid]->_remain_sleep_time > 0)
  {
    sleepings.erase (std::remove (sleepings.begin (), sleepings.end (), tid)
                     , sleepings.end ());
  }

  //delete from threads array
  delete threads[tid];
  current_threads_amount--;

  if (tid == 0)
  {
    printf ("ASASASAS terminating the main thread\n");
    fflush (stdout);
    exit (0);
  }
  return SUCCESS;
}

int uthread_block (int tid){
  printf("ASASAS enter function uthread_block. running is %d. parameter is "
         "%d\n",
         running_process_id, tid);
  if (tid == 0)
  {
    printf ("thread library error: cant block the main thread\n");
    fflush (stderr);
    return FAIL;
  }
  if (is_exists (tid) == FAIL)
  {
    return FAIL;
  }
  if (tid == running_process_id)
  {
    return schedule (BLOCKED);
  }
  else if (threads[tid]->_state == READY)
  {
    readies.erase (std::remove (readies.begin (), readies.end (), tid),
                   readies.end ());
    threads[tid]->_state = BLOCKED;
  }
  return SUCCESS;
}

int uthread_resume (int tid){
  printf("ASASAS enter function uthread_resume. running is %d, parameter is %d"
         "\n",
         running_process_id, tid);
  if (is_exists (tid) == FAIL)
  {
    return FAIL;
  }
  if (threads[tid]->_state == BLOCKED)
  {
    if (threads[tid]->_remain_sleep_time <= 0)
    {
      threads[tid]->_state = READY;
      readies.insert (readies.begin (), tid);
    }
    else
    {
      threads[tid]->_state = RUN; // when it will wake up it will return to
      // ready
    }
  }
  return SUCCESS;
}

int uthread_sleep (int num_quantums){
  printf("ASASAS enter function uthread_sleep. running is %d, parameter is "
         "%d\n",
         running_process_id, num_quantums);

  if (running_process_id == 0)
  {
    printf ("thread library error: cant put to sleep the main thread\n");
    fflush (stderr);
    return FAIL;
  }
  if (num_quantums <= 0)
  { // todo: needs to be positive or not-negative?
    printf ("thread library error: num_quantums should be positive\n");
    fflush (stderr);
    return FAIL;
  }
  // todo: make sure it should be +1 (since the current doesnt count)
  threads[running_process_id]->_remain_sleep_time = num_quantums + 1;
  sleepings.insert (sleepings.begin (), running_process_id);
  if (threads[running_process_id]->_state != BLOCKED)
  {
    return schedule (SLEEPING);
  }
  return schedule (BLOCKED);
}

int uthread_get_tid (){
  printf("ASASAS enter function uthread_get_tid. running is %d"
         "\n", running_process_id);
  return running_process_id;
}

int uthread_get_total_quantums (){
  printf("ASASAS enter function uthread_get_total_quantums. running is %d\n",
         running_process_id);
  // todo: what does it means "including the current"?
  return total_tick;
}

int uthread_get_quantums (int tid){
  printf("ASASAS enter function uthread_get_quantums. running is %d\n",
         running_process_id);
  if (is_exists (tid) == FAIL)
  {
    return FAIL;
  }
  return threads[tid]->_quantums;
}



