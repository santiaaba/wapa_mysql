#include <stdio.h>
#include <pthread.h>
#include "parce.h"
#include "config.h"
#include "dictionary.h"
#include "db.h"
#include "logs.h"
#include <time.h>

#ifndef TASK_H
#define TASK_H

#define TOKEN_SIZE		25
#define TASKID_SIZE		25
#define TASKRESULT_SIZE		200

#define ERROR_FATAL		"300|\"code\":\"300\",\"info\":\"ERROR Fatal\""

#define SYSTEM_DO	if(system(command) != 0){ \
				task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\""); \
				return 0; \
			}

typedef enum {	
		T_TASK_SHOW,

		T_SUSC_SHOW,
		T_SUSC_ADD,
		T_SUSC_DEL,
		T_SUSC_MOD,
		T_SUSC_STOP,
		T_SUSC_START,

		T_DB_LIST,
		T_DB_SHOW,
		T_DB_ADD,
		T_DB_MOD,
		T_DB_DEL,
		T_DB_ALLDEL,
		T_DB_STOP,
		T_DB_START,

} T_task_type;

typedef enum { T_WHAITING,
		T_RUNNING,
		T_DONE,
} T_task_status;

typedef enum {
		T_ADMIN,
		T_TENNANT
} T_task_user;

typedef struct heap_task T_heap_task;
typedef struct bag_task T_bag_task;

typedef char T_taskid[TASKID_SIZE];
typedef char T_tasktoken[TOKEN_SIZE];

/*****************************
		TASK	
******************************/
typedef struct {
        T_taskid id;
        T_tasktoken token;
	T_task_user user;		//determina el usuario que es y por las acciones permitidas
	T_task_type type;		//tipo de accion a realizar
	T_task_status status;		//estado del task
	T_dictionary *data;		//datos necesarios para realizar la accion
	time_t time;			//Instante de tiempo en que la tarea ingresa o finaliza
	char *result;			//resultado a retornar
} T_task;

T_task_type task_c_to_type(char c);
int valid_task_id(char *s);
void task_init(T_task *t, T_task_type type, T_dictionary *data);
time_t task_get_time(T_task *t);
void task_destroy(T_task **t);
void task_run(T_task *t, T_list_site *sites, T_list_worker *workers,
	      T_list_proxy *proxys,T_db *db, T_config *config, T_logs *logs);
void task_done(T_task *t, char *message);
char *task_get_token(T_task *t);
char *task_get_id(T_task *t);
char *task_get_result(T_task *t);
void task_show(T_task *t);

/*****************************
         Cola de tareas
******************************/
typedef struct j_h_node {
        T_task *data;
        struct j_h_node *next;
} heap_t_node;

struct heap_task {
        unsigned int size;
        heap_t_node *first;
        heap_t_node *last;
};

void heap_task_init(T_heap_task *h);
void heap_task_push(T_heap_task *h, T_task *t);
int  heap_task_exist(T_heap_task *h,T_taskid id);
T_task *heap_task_pop(T_heap_task *h);
unsigned int heap_task_size(T_heap_task *h);
void heap_task_print(T_heap_task *h);

/****************************
 	BAG Jobs
*****************************/

typedef struct j_b_node {
        T_task *data;
        struct j_b_node *next;
} bag_t_node;

struct bag_task {
        unsigned int size;
        bag_t_node *first;
        bag_t_node *last;
        bag_t_node *actual;
};

void bag_task_init(T_bag_task *b);
void bag_task_add(T_bag_task *b, T_task *t);
T_task *bag_task_pop(T_bag_task *b, T_taskid *id);
unsigned int bag_task_size(T_bag_task *b);
void bag_task_timedout(T_bag_task *b, int d);
void bag_task_print(T_bag_task *b);

#define JOB_H
#endif
