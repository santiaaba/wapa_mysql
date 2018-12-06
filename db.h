/* Posee los metodos y estructuras de datos para manejar el
 * acceso a la base de datos
 */

#include <mysql/mysql.h>
#include <stdlib.h>
#include "config.h"
#include "dictionary.h"
#include "logs.h"

#ifndef DB_H
#define DB_H

typedef enum {DB_ONLINE, DB_OFFLINE} T_DB_status;

typedef struct db {
	MYSQL *con;
	T_logs *logs;
	char user[100];
	char pass[100];
	char server[100];
	char dbname[100];
	T_DB_status status;
} T_db;

void db_init(T_db *db, T_config *c, T_logs *logs);
int db_connect(T_db *db);
void db_close(T_db *db);
int db_live(T_db *db);

const char *db_error(T_db *db);

/* Para los sitios */
int db_limit_dbs(T_db *db, char *susc_id, int *db_fail);
int db_get_dbs_id(T_db *db, char *susc_id, int db_ids[256], int *db_ids_len, char *error, int *db_fail );
int db_db_add(T_db *db, T_dictionary *d, char *error, int *db_fail);
int db_db_mod(T_db *db, T_dictionary *d, char *error, int *db_fail);
void db_db_list(T_db *db, char **data, char *susc_id);
int db_db_del(T_db *db, char *db_id, uint32_t size, char *error, int *db_fail);
uint16_t db_db_exist(T_db *db, char *susc_id, char *db_id, char *error, int *db_fail);
void db_db_show(T_db *db, char **data, char *db_id, char *susc_id);
int db_del_all_db(T_db *db, char *susc_id, char *error, int *db_fail);

/* Para las suscripcionse */
int db_susc_show(T_db *db,char *susc_id,char **message,int *db_fail);
int db_susc_add(T_db *db, T_dictionary *d, int *db_fail);
int db_susc_del(T_db *db, char *susc_id);

#endif
