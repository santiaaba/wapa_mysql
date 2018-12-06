#include "task.h"

void random_task_id(T_taskid value){
	/*Genera un string random para task_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;
	
	//for(j=0;j<TASKID_SIZE-1;j++){
	for(j=0;j<TASKID_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
	value[TASKID_SIZE] = '\0';
}

void random_token(T_tasktoken value){
	/*Genera un string random para token_id */
	char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i,j;

	for(j=0;j<TOKEN_SIZE;j++){
		i = rand() % 62;
		value[j] = string[i];
	}
	value[TOKEN_SIZE] = '\0';
}

int valid_task_id(char *s){
	int ok=1;
	int size;
	int i=0;

	if(s == NULL)
		return 0;
	if(strlen(s) != TASKID_SIZE)
		return 0;
	while(ok && i<TASKID_SIZE){
		ok = ((47 < (int)s[i] && (int)s[i] < 58) ||     //numeros
		     (64 < (int)s[i] && (int)s[i] < 91) ||      //letras mayusculas
		     (96 < (int)s[i] && (int)s[i] < 123));       //letras minusculas
		i++;
	}
	return ok;
}

T_task_type task_c_to_type(char c){
	/* Hay dos restricciones.
 	 * 't': Es utilizado cuando el core solicita el estado de un task
 	 * 'c': Es utilizado cuando el core solicita un chequeo al controller
 	 */
	switch(c){
		case '0': return T_SUSC_ADD;
		case '1': return T_SUSC_DEL;
		case '2': return T_SUSC_STOP;
		case '3': return T_SUSC_START;
		case '4': return T_SUSC_SHOW;

		case 'l': return T_DB_LIST;
		case 's': return T_DB_SHOW;
		case 'a': return T_DB_ADD;
		case 'm': return T_DB_MOD;
		case 'd': return T_DB_DEL;
		case 'k': return T_DB_STOP;
		case 'e': return T_DB_START;
	}
}

/****************************************
	     Verificadiones
*****************************************/

/* Antes por cada tarea verificabamos que el diccionario en data
 * tuviese todos los datos necesarios. Esa responsabilidad se la
 * pasamos a server.c server.h. Los mismos no generan un task
 * si no estan todos los datos necesarios
*/

/*****************************
	     TASK 
******************************/
void task_init(T_task *t, T_task_type type, T_dictionary *data){
	random_task_id(t->id);
	//random_token(t->token);
	t->time = time(NULL);
	t->type = type;
	t->data = data;
	t->result = NULL;
}

void task_destroy(T_task **t){
	if((*t)->data != NULL){
		dictionary_destroy(&((*t)->data));
	}
	free((*t)->result);
	free(*t);
}

time_t task_get_time(T_task *t){
	return t->time;
}

char *task_get_token(T_task *t){
	return t->token;
}

char *task_get_result(T_task *t){
	return t->result;
}

char *task_get_id(T_task *t){
	return t->id;
}

void task_done(T_task *t, char *message){
	t->status = T_DONE;
	t->time = time(NULL);
	t->result=(char *)realloc(t->result,strlen(message) + 1);
	strcpy(t->result,message);
}

void task_db_list(T_task *t, T_db *db, T_logs *logs){
	/* Lista sitios de una suscripcion dada */
	char *susc_id;

	susc_id = dictionary_get(t->data,"susc_id");
	db_db_list(db,&(t->result),susc_id);
}

void task_db_show(T_task *t, T_db *db, T_logs *logs){
	char *db_id;
	char *susc_id;

	db_id = dictionary_get(t->data,"site_id");
	susc_id = dictionary_get(t->data,"susc_id");
	db_db_show(db,&(t->result),db_id,susc_id);
}

int task_db_add(T_task *t, T_db *db, T_config *config, T_logs *logs){
	/* Agrega un sitio a la solucion */

	dictionary_print(t->data);
	name = dictionary_get(t->data,"name");
	susc_id = dictionary_get(t->data,"susc_id");

	/* Verificamos que no se supere el limite de base de datos establecidos
	   para la suscripcion */
	if(0 == db_limit_dbs(db, susc_id, &db_fail)){
		if(db_fail)
			task_done(t,ERROR_FATAL);
		else
			task_done(t,"300|\"code\":\"300\",\"info\":\"Se supera el limite de bases de datos mysql\"");
		return 0;
	}
	
	// Alta en la base de datos 
	if(!db_db_add(db,name,atoi(susc_id),error,&db_fail)){
		if(db_fail){
			task_done(t,ERROR_FATAL);
			logs_write(logs,L_ERROR,"task_site_add", "DB_ERROR");
		} else {
			task_done(t,error);
		}
		printf("Paso\n");
		return 0;
	}

	task_done(t,"200|\"code\":\"201\",\"info\":\"base agregada correctamente\"");
	return 1;
}

int task_db_del(T_task *t, T_db *db, T_config *c, T_logs *logs){

	char error[200];
	char *db_id;
	int db_fail;
	int ok;

	db_id = dictionary_get(t->data,"db_id");

	task_done(t,"200|\"code\":\"202\",\"info\":\"Base de datos borrada\"");
	logs_write(logs,L_INFO,"task_db_del","Base borrada");
	return 1;
}


int task_susc_show(T_task *t, T_db *db, T_logs *logs){
	/* Retorna en formato Json informacion de la suscripcion */
	int db_fail;
	char *message=NULL;

	if(db_susc_show(db,dictionary_get(t->data,"susc_id"),&message,&db_fail)){
		task_done(t,message);
	} else {
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	}
	
}

void task_susc_add(T_task *t, T_db *db, T_logs *logs){
	/* Agrega una suscripcion */
	int db_fail;

	if(db_susc_add(db,t->data,&db_fail)){
		task_done(t,"200|\"code\":\"202\",\"info\":\"Suscripcion Agregada\"");
	} else {
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
	}
}

void task_susc_del(T_task *t, T_db *db, T_config *c, T_logs *logs){
	/* Elimina una suscripcion y todos sus datos */
	/* Retorna 1 si pudo borrar todo. Caso contrario retorna 0 */

	int db_ids[256];	//256 es la maxima cantidad de bases por suscripcion
	int db_ids_len = 0;	//Cantidad de elementos del array db_ids
	T_task *task_aux=NULL;
	T_dictionary *data_aux=NULL;
	int ok=1;
	char db_id[100];
	char *susc_id;
	char error[200];
	int db_fail;

	susc_id = dictionary_get(t->data,"susc_id");
	task_aux = (T_task *)malloc(sizeof(T_task));
	data_aux = (T_dictionary *)malloc(sizeof(T_dictionary));
	dictionary_init(data_aux);
	task_init(task_aux,T_DB_DEL,data_aux);
	if(!db_get_db_id(db,susc_id,db_ids,&db_ids_len,error,&db_fail)){
		if(db_fail)
			task_done(t,ERROR_FATAL);
		else
			task_done(t,error);
	} else {
		while((db_ids_len > 0) && ok){
			sprintf(db_id,"%i",db_ids[db_ids_len - 1 ]);
			dictionary_add(task_aux->data,"db_id",db_id);
			dictionary_print(task_aux->data);
			ok &= task_db_del(task_aux,db,c,logs);
			dictionary_remove(task_aux->data,"db_id");
			db_ids_len--;
		}
	}
	task_destroy(&task_aux);
	printf("OK vale=%i\n",ok);
	if(ok){
		/* Eliminadas las bases, borramos la suscripcion de la base de datos */
		if(!db_susc_del(db,susc_id))
			task_done(t,ERROR_FATAL);
		else
			task_done(t,"200");
	} else
		task_done(t,"300|\"code\":\"300\",\"info\":\"ERROR FATAL\"");
}

void task_susc_stop(T_task *t, T_db *db){
	/* Implementar */
}
	
void task_susc_start(T_task *t, T_db *db){
	/* Implementar */
}

int task_db_stop(T_task *t, T_db *db, T_logs *logs){
	/* Implementar */
}

int task_db_start(T_task *t, T_db *db, T_logs *logs){
	/* Implementar */
}

void task_db_mod(T_task *t, T_list_site *l, T_db *db, T_logs *logs){
	/* Implementar */
}


void task_run(T_task *t, T_db *db, T_config *config, T_logs *logs){
	/* Ejecuta el JOB */
	t->status = T_RUNNING;

	printf("TASK_RUN: %i\n",t->type);
	switch(t->type){
		case T_SUSC_ADD:
			task_susc_add(t,db,logs); break;
		case T_SUSC_DEL:
			task_susc_del(t,db,config,logs); break;
		case T_SUSC_STOP:
			task_susc_stop(t,db); break;
		case T_SUSC_START:
			task_susc_start(t,db); break;
		case T_SUSC_SHOW:
			task_susc_show(t,db,logs); break;

		case T_DB_LIST:
			task_db_list(t,db,logs); break;
		case T_DB_SHOW:
			task_db_show(t,db,logs); break;
		case T_DB_ADD:
			task_db_add(t,db,config,logs); break;
		case T_DB_DEL:
			task_db_del(t,db,config,logs); break;
		case T_DB_MOD:
			task_db_mod(t,db,logs); break;
		case T_DB_STOP:
			task_db_stop(t,db,logs); break;
		case T_DB_START:
			task_db_start(t,db,logs); break;

		default:
			printf("ERROR FATAL. TASK_TYPE indefinido\n");

	}
}

/*****************************
	  Cola de Jobs
 ******************************/

void heap_task_init(T_heap_task *h){
	h->first = NULL;
	h->last = NULL;
	h->size = 0;
}

void heap_task_push(T_heap_task *h, T_task *t){
	heap_t_node *new;
	heap_t_node *aux;

	printf("heap_task_push\n");
	new = (heap_t_node*)malloc(sizeof(heap_t_node));
	new->next = NULL;
	new->data = t;
	h->size++;

	if(h->first == NULL){
		h->first = new;
		h->last = new;
	} else {
		h->last->next = new;
		h->last = new;
	}
}

T_task *heap_task_pop(T_heap_task *h){
	heap_t_node *aux;
	T_task *taux;

	if(h->first != NULL){
		aux = h->first;
		taux = h->first->data;
		h->first = h->first->next;
		if(h->first == NULL)
			h->last = NULL;
		free(aux);
		return taux;
	} else {
		return NULL;
	}
}

unsigned int heap_task_size(T_heap_task *h){
	return h->size;
}

int heap_task_exist(T_heap_task *h, T_taskid id){
	/* indica si el trabajo existe en la cola */
	heap_t_node *aux;
	int exist=0;
	
	aux = h->first;
	while(!exist && aux!= NULL){
		exist = (strcmp(task_get_id(aux->data),id) == 0);
		aux = aux->next;
	}
	return exist;
}

void heap_task_print(T_heap_task *h){

	heap_t_node *aux;

	aux = h->first;
	while(aux!= NULL){
		printf("Job_ID: %s\n",task_get_id(aux->data));
		aux = aux->next;
	}
}

/******************************
 * 		BAG JOB
 ******************************/

void bag_task_init(T_bag_task *b){
	b->first = NULL;
	b->actual = NULL;
	b->last = NULL;
	b->size = 0;
}

void bag_task_add(T_bag_task *b, T_task *t){
	bag_t_node *new;
	bag_t_node *aux;

	printf("bag_task_add\n");
	new = (bag_t_node*)malloc(sizeof(bag_t_node));
	new->next = NULL;
	new->data = t;
	b->size++;

	if(b->first == NULL){
		b->first = new;
		b->last = new;
	} else {
		b->last->next = new;
		b->last = new;
	}
}

T_task *bag_site_remove(T_bag_task *b){
	bag_t_node *prio;
	bag_t_node *aux;
	T_task *element = NULL;

	if(b->actual != NULL){
		aux = b->first;
		prio = NULL;
		while(aux != b->actual){
			prio = aux;
			aux = aux->next;
		}
		if(prio == NULL){
			b->first = aux->next;
		} else {
			prio->next = aux->next;
		}
		if(aux == b->last){
			b->last = prio;
		}
		b->actual = aux->next;
		element = aux->data;
		free(aux);
	}
	return element;
}

T_task *bag_task_pop(T_bag_task *b, T_taskid *id){
	/* Retorna el task buscado por su id.
 	 * si no existe retorna NULL. Si existe
 	 * no solo lo retorna sino que lo elimina
 	 * de la bolsa */
	int exist = 0;
	T_task *taux = NULL;

	b->actual = b->first;
	while((b->actual != NULL) && !exist){
		printf("Comparamos -%s- con -%s-\n",id,task_get_id(b->actual->data));
		exist = (strcmp(task_get_id(b->actual->data),(char *)id)==0);
		if((!exist && (b->actual != NULL))){
			printf("	Avanzamos. No son iguales\n");
			b->actual = b->actual->next;
		}
		//sleep(5);
	}
	if(exist)
		taux = bag_site_remove(b);
	return taux;
}

unsigned int bag_task_size(T_bag_task *b){
	return b->size;
}

void bag_task_timedout(T_bag_task *b, int d){
	/* Elimina de la estructura todas las
	   tareas cuya diferencia de tiempo entre
	   finalizado y la hora actual sea lo que
	   se indica en el parametro d */

	T_task *taux = NULL;
	time_t now = time(NULL);

	printf("bag_task_timedout\n");
	b->actual = b->first;
	while((b->actual != NULL)){
		if(d < difftime(now,task_get_time(b->actual->data))){
			taux = bag_site_remove(b);
			free(taux);
		} else
			b->actual = b->actual->next;
	}
}

void bag_task_print(T_bag_task *b){

	bag_t_node *aux;

	printf("PRINT BAG\n");
	aux = b->first;
	while(aux!= NULL){
		printf("Job_ID: %s\n",task_get_id(aux->data));
		aux = aux->next;
	}
	printf("END PRINT BAG\n");
}
