//
// Created by t9092556 on 22/04/2023.
//

#ifndef _THREADS_H_
#define _THREADS_H_
typedef unsigned long address_t;
#include <stdlib.h>
#include <list>
#include "stdlib.h"
#include <setjmp.h>
#include <list>
#include "iostream"
#include <sys/time.h>
#include <signal.h>
typedef void (*thread_entry_point)(void);

typedef struct extend_thread{
    int time_left_sleep; //non zero if asleep
    bool is_asleep;
    bool is_blocked;
    sigjmp_buf my_env;
    int tid_thread; //num thread
    int total_run_time; //time run till now
    char* stack_pointer;
} extend_thread;

address_t translate_address(address_t addr);

bool is_valid_thread(bool* status_number,int tid,int num);
int get_new_thread_id(bool* status_number,int num);
int get_and_pop_next_thread_to_run(std::list<int>& ready_queue);
int get_number_of_threads(bool* status_number,int num);

void write_system_error(std::string const& message);
void write_library_error(std::string const& message);

void setup_thread(sigjmp_buf& env, char *stack, thread_entry_point
entry_point);

#endif //_THREADS_H_
