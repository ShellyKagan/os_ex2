/*
 * User-Level Threads Library (uthreads)
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */

#include "uthreads.h"
#include "utils.h"
#include <algorithm>

void timer_handler(int sig);
void context_switch();

typedef struct Controller{
    extend_thread threads_data[MAX_THREAD_NUM];
    std::list<int> ready_threads;
    bool is_thread_active[MAX_THREAD_NUM];
    int running;
    int total_running_time;
    struct itimerval timer;
} Controller;

Controller controller;

int block_clock()
{
  sigset_t adder_set;
  sigemptyset (&adder_set);
  sigaddset (&adder_set, SIGVTALRM);
  if (sigprocmask (SIG_BLOCK, &adder_set, NULL) == -1)
  {
    write_system_error ("seprocmask error.");
    return -1;
  }
  return 0;
}
int unblock_clock(){
  sigset_t adder_set;
  sigemptyset(&adder_set);
  sigaddset(&adder_set,SIGVTALRM);
  if(sigprocmask(SIG_UNBLOCK,&adder_set,NULL)==-1){
    write_system_error("seprocmask error.");
    return -1;
  }
  return 0;
}

int uthread_init(int quantum_usecs)
{

  // check input
  if (quantum_usecs <= 0){
    write_library_error("INIT: quantum_usecs need to be positive.");
    return -1;
  }

  // create thread number 0
  controller.is_thread_active[0] = true;
  controller.threads_data[0].total_run_time = 1;
  controller.total_running_time = 1;

  // set timer
  controller.timer.it_interval.tv_sec = 0;
  controller.timer.it_interval.tv_usec =  quantum_usecs;

  controller.timer.it_value.tv_sec = 0;
  controller.timer.it_value.tv_usec = quantum_usecs;

  struct sigaction sa = {0};
  sa.sa_handler = &timer_handler;
  if (sigaction(SIGVTALRM, &sa, NULL) < 0)
  {
    write_system_error ("INIT: sigaction failed");
    return -1;
  }

  if (setitimer(ITIMER_VIRTUAL, &controller.timer, NULL))
  {
    write_system_error("INIT: setitimer error.");
    return -1;
  }

  return 0;
}

int uthread_spawn(thread_entry_point entry_point)
{

  if (entry_point == nullptr)
  {
    write_library_error ("SPAWN: Invalid input");
    return -1;
  }

  // check if there is enough space to create thread
  if (get_number_of_threads(controller.is_thread_active, MAX_THREAD_NUM) >=
  MAX_THREAD_NUM)
  {
    write_library_error ("SPAWN: Can't create new threads");
    return -1;
  }

  // get the id of the new thread
  int tid = get_new_thread_id(controller.is_thread_active, MAX_THREAD_NUM);
  if (tid == -1)
  {
    write_library_error ("SPAWN: Can't create new threads");
    return -1;
  }
  if(block_clock()==-1){
    return -1;
  }
  // malloc stack space for the new thread
  char* thread_stack = (char*) malloc(STACK_SIZE);
  if (thread_stack == nullptr)
  {
    write_system_error ("SPAWN: Failed malloc");
    exit(1);
    return -1;
  }

  // create the thread
  setup_thread(controller.threads_data[tid].my_env, thread_stack,
               entry_point);
  controller.is_thread_active[tid] = true;
  controller.threads_data[tid].is_asleep = false;
  controller.threads_data[tid].is_blocked = false;
  controller.threads_data[tid].stack_pointer = thread_stack;
  controller.threads_data[tid].total_run_time = 0;

  // push thread to end of ready list
  controller.ready_threads.push_back (tid);
  if(unblock_clock()==-1){
    return -1;
  }

  return tid;
}

int uthread_terminate(int tid)
{
  // check input
  if (tid < 0 || tid >= MAX_THREAD_NUM)
  {
    write_library_error ("TERMINATE: Invalid tid");
    return -1;
  }
  if (!controller.is_thread_active[tid])
  {
    write_library_error("TERMINATE: terminated non existing thread");
    return -1;
  }
  if(block_clock()==-1){
    return -1;
  }
  // check if terminating the main thread
  if(tid==0){
    for (int i = 0; i < MAX_THREAD_NUM; i++)
    {
      if (controller.is_thread_active[i])
      {
        free(controller.threads_data[i].stack_pointer);
      }
    }
    exit(0);

  }


  // TODO understand what to do if a thread terminates itself
  if (tid == controller.running)
  {
    controller.is_thread_active[tid] = false;
    free(controller.threads_data[tid].stack_pointer);
    context_switch();
  }

  //terminate the thread
  free(controller.threads_data[tid].stack_pointer);
  if (!controller.threads_data[tid].is_asleep && !controller
  .threads_data[tid].is_blocked)
  {
    controller.ready_threads.remove (tid);
  }
  controller.is_thread_active[tid] = false;

  if(unblock_clock()==-1){
    return -1;
  }
  return 0;
}


int uthread_block(int tid)
{

  // check input
  if (!is_valid_thread (controller.is_thread_active, tid, MAX_THREAD_NUM))
  {
    write_library_error ("BLOCK: Invalid thread id.");
    return -1;
  }
  if (tid == 0)
  {
    write_library_error ("BLOCK: Cannot block main thread.");
    return -1;
  }
  if(block_clock()==-1){
    return -1;
  }
  // set thread to blocked
  if (controller.threads_data[tid].is_blocked)
  {
    if(unblock_clock()==-1){
      return -1;
    }
    return 0;
  }
  else
  {
    controller.threads_data[tid].is_blocked = true;
    if (controller.running == tid)
    {
      context_switch();
    }
    else
    {
      if (!controller.threads_data[tid].is_asleep)
      {
        controller.ready_threads.remove(tid);
      }
    }
  }
  if(unblock_clock()==-1){
    return -1;
  }
  return 0;
}


int uthread_resume(int tid)
{

  // check input
  if (!is_valid_thread (controller.is_thread_active, tid, MAX_THREAD_NUM))
  {
    write_library_error ("RESUME: Invalid thread id.");
    return -1;
  }
  if(block_clock()==-1){
    return -1;
  }
  // check if thread is ready or running (aka not blocked or sleeping)
  if (!controller.threads_data[tid].is_blocked && !controller
  .threads_data[tid].is_asleep)
  {
    if(unblock_clock()==-1){
      return -1;
    }
    return 0;
  }

  // resume thread
  controller.threads_data[tid].is_blocked = false;

  // check if not sleeping and add back to ready
  if (!controller.threads_data[tid].is_asleep)
  {
    controller.ready_threads.push_back (tid);
  }
  if(unblock_clock()==-1){
    return -1;
  }
  return 0;
}


int uthread_sleep(int num_quantums)
{
  // check if main thread called to this function
  if (controller.running == 0)
  {
    write_library_error ("SLEEP: Cannot put main thread to sleep");
    return -1;
  }
  if(block_clock()==-1){
    return -1;
  }
  // move running thread to blocked
  int tid = controller.running;
  controller.threads_data[tid].is_asleep = true;
  controller.threads_data[tid].time_left_sleep = num_quantums;

  // schedule new thread
  int next_thread = get_and_pop_next_thread_to_run (controller.ready_threads);
  // TODO setup clock for rescheduling
  controller.running = next_thread;
  if(unblock_clock()==-1){
    return -1;
  }
  return 0;
}


int uthread_get_tid()
{
  return controller.running;
}



int uthread_get_total_quantums()
{
  return controller.total_running_time;
}



int uthread_get_quantums(int tid)
{
  // check if id is a valid thread
  if (!is_valid_thread (controller.is_thread_active, tid, MAX_THREAD_NUM))
  {
    write_library_error ("Invalid thread ID");
    return -1;
  }

  // return number of quantums
  return controller.threads_data[tid].total_run_time;
}

void context_switch()
{
  if(block_clock()==-1){
    return ;
  }
  // increase total run time
  controller.total_running_time += 1;

  // lower sleeping time of all threads, resume if necessary
  for (int i = 0; i < MAX_THREAD_NUM; i++)
  {
    if (controller.is_thread_active[i] && controller.threads_data[i].is_asleep)
    {
      controller.threads_data[i].time_left_sleep -= 1;
      if (controller.threads_data[i].time_left_sleep <= 0)
      {
        // remove from sleeping
        controller.threads_data[i].is_asleep = false;
        if (!controller.threads_data[i].is_blocked)
        {
          controller.ready_threads.push_back (i);
        }
      }
    }
  }


  // actual context switch
  if (sigsetjmp(controller.threads_data[controller.running].my_env, 1) == 0)
  {
    int current_thread = controller.running;
    if (controller.is_thread_active[current_thread] && !controller
        .threads_data[current_thread].is_blocked && !controller
        .threads_data[current_thread].is_asleep)
    {
      controller.ready_threads.push_back (controller.running);
    }

    int next_thread = get_and_pop_next_thread_to_run (controller.ready_threads);
    controller.threads_data[next_thread].total_run_time += 1;
    controller.running = next_thread;

//    if(unblock_clock()==-1){
//      return ;
//    }
    siglongjmp (controller.threads_data[next_thread].my_env, 1);
  }

  if(unblock_clock()==-1){
    return ;
  }
}

void timer_handler(int sig)
{
  // check input
  if (sig != SIGVTALRM)
  {
    write_library_error ("called timer_handler with incorrect signal.");
  }

  // context switch
  context_switch();
}


