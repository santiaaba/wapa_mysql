#include "db.h"

/********************************
 * 	Funciones varias	*
 ********************************/

/********************************
 * 	Funciones DB		*
 ********************************/

void db_init(T_db *db, T_config *c, T_logs *logs){
	db->status = DB_ONLINE;
	db->logs = logs;
	strcpy(db->user,c->db_user);
	strcpy(db->pass,c->db_pass);
	strcpy(db->server,c->db_server);
	strcpy(db->dbname,c->db_name);
}

int db_connect(T_db *db){
	db->con = mysql_init(NULL);
	printf("Conectando DB: %s,%s,%s,%s\n",db->server,db->user, db->pass, db->dbname);
	if (mysql_real_connect(db->con, db->server,
		db->user, db->pass, db->dbname, 0, NULL, 0)){
		db->status = DB_ONLINE;
		printf("Conecto\n");
		return 1;
	} else {
		printf("NO Conecto\n");
		db_close(db);
		return 0;
	}
}

int db_live(T_db *db){

	MYSQL_RES *result;

	if(mysql_query(db->con,"select * from version")){
		db_close(db);
		return 0;
	} else {
		result = mysql_store_result(db->con);
		return 1;
	}
}

void db_close(T_db *db){
	printf("Cerramos base de datos\n");
	mysql_close(db->con);
	db->status = DB_OFFLINE;
}

const char *db_error(T_db *db){
	return mysql_error(db->con);
}

int db_exist_db(T_db *db, char *name){
	/* Determina si una base ya existe en la base de datos con ese nombre */

	char query[200];
	int resultado;

	sprintf(query,"select name from web_db where name='%s'",name);
	mysql_query(db->con,query);
	MYSQL_RES *result = mysql_store_result(db->con);
	if(mysql_num_rows(result) == 0){
		printf("DB no existe!!!\n");
		return 0;
	} else {
		printf("DB EXISTE!!!\n");
		return 1;
	}
}

/****	Funciones para suscripciones	****/

int db_susc_show(T_db *db,char *susc_id,char **message,int *db_fail){
	/* Retorna en formato json los datos de una suscripcion */
	char query[200];
	char aux[200];
	MYSQL_ROW row;
	MYSQL_RES *result;

	sprintf(query,"select * from db_suscription where id=%s",susc_id);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_show","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	strcpy(aux,"200|\"cloud\":\"Hosting web\",\"data\":\"Nada de momento\"}");
	printf("DB_SUSC_SHOW: %s\n",aux);
	*message=(char *)realloc(*message,strlen(aux)+1);
	strcpy(*message,aux);
	return 1;
	
}

int db_susc_add(T_db *db, T_dictionary *d, int *db_fail){
	char query[200];
	char hash_dir[6];
	char *susc_id;

	/* Generamos la entrada en web_suscription */
	random_dir(hash_dir);
	susc_id = dictionary_get(d,"susc_id");
	sprintf(query,"insert into db_suscription values (%s,'%s',%s)",
		susc_id,hash_dir,dictionary_get(d,"web_sites"));
	printf("sql %s\n",query);
	logs_write(db->logs,L_DEBUG,"db_susc_add", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_susc_add","DB_ERROR");
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	return 1;
}

int db_susc_del(T_db *db, char *susc_id){
	/* Elimina una suscripcion. Previamente se deberia eliminar
	   las bases de datos Mysql de la misma. Si falla retorna 0 y se considera
	   una falla en la base de datos. Sino retorna 1 */
	char query[200];
	sprintf(query,"delete from db_suscription where id=%s",susc_id);
	printf("query:%s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	return 1;
}

/****	Funciones para bases mysql	****/

int db_limit_dbs(T_db *db, char *susc_id, int *db_fail){
	/* retorna 0 si se supera el limite de
	 * bases mysql de la suscripcion o falla la base de datos*/
	char query[300];
	MYSQL_RES *result;
        MYSQL_ROW row;

	sprintf(query,"select (select sites_limit from web_suscription where id=%s) - (select count(id) from web_site where susc_id=%s)",susc_id,susc_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	row = mysql_fetch_row(result);
	return atoi(row[0]);
}

int db_db_add(T_db *db, char *name, unsigned int susc_id, char *error, int *db_fail){
	/* Agrega una base a la base de datos.
 	 * Si no pudo hacerlo retorna 0 sino 1 */

	char query[300];
	MYSQL_RES *result;
	MYSQL_ROW row;
	unsigned int db_id;
	int i;

	CORREGIR TODO

	sprintf(query,"insert into web_site(version,name,size,susc_id) values(1,\"%s\",1,%lu)",
		name,susc_id,dir);
	logs_write(db->logs,L_DEBUG,"db_site_add", query);

	if(mysql_query(db->con,query)){
		printf("DB_SITE_ADD errno: %i\n",mysql_errno(db->con));
		if(mysql_errno(db->con) == 1062){
			logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =0;
			logs_write(db->logs,L_ERROR,"db_site_add","Sitio con nombre repetido");
			strcpy(error,"300|\"code\":\"301\",\"info\":\"Ya existe sitio con ese nombre\"");
		} else {
			*db_fail =1;
			logs_write(db->logs,L_ERROR,"db_site_add","DB_ERROR");
		}
		printf("Pasamos\n");
		return 0;
	} else {
		site_id = mysql_insert_id(db->con);

		/*Obtenemos el directorio */
		sprintf(query,"select hash_dir from web_suscription where id=%i",susc_id);
		printf("query: %s\n",query);
		if(mysql_query(db->con,query)){
			logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
			*db_fail =1;
			return 0;
		}
		result = mysql_store_result(db->con);
		row = mysql_fetch_row(result);
		(*newsite) = (T_site *)malloc(sizeof(T_site));
		site_init(*newsite,name,site_id,row[0],1,1);
		strcpy(dir,row[0]);

		/* Poblamos los indices */
		for(i=5;i>=0;i--){
			sprintf(query,"insert into web_indexes(site_id,name,prioridad) values(%i,'%s',%i)",
				site_id,index[i],i);
			if(mysql_query(db->con,query)){
				logs_write(db->logs,L_ERROR,"db_site_add", "DB_ERROR");
				*db_fail =1;
				return 0;
			}
			index_id = mysql_insert_id(db->con);
			newindex = (T_s_e *)malloc(sizeof(T_s_e));
			s_e_init(newindex,index_id,index[i]);
			list_s_e_add(site_get_alias(*newsite),newindex);
		}
		*db_fail =0;
		return 1;
	}
}

int db_db_mod(T_db *db, T_dictionary *d, char *error, int *db_fail){
	/* IMPLEMENTAR */
	return 1;
}

int db_db_status(T_db *db, char *susc_id, char *db_id, char *status, char *error, int *db_fail){
	/* Modifica el estado de una base */

	/* IMPLEMENTAR */
	return 1;
}

int db_get_dbs_id(T_db *db, char *susc_id, int db_ids[256], int *db_ids_len, char *error, int *db_fail ){
	/* Retorna en la variable db_ids la cual es en array de int, el listado
 	 * de ids de dbs mysql de la suscripcion */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;
	
	CORREGIR TODO

	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	printf("paso\n");
	result = mysql_store_result(db->con);
	while(row = mysql_fetch_row(result)){
		site_ids[i] = atoi(row[0]);
		i++;
	}
	*site_ids_len = i;
	printf("Termino\n");
	return 1;
}

void db_db_list(T_db *db, char **data, char *susc_id){
	/* Retorna en formato json la lista de bases mysql dado el id de
 	 * una suscripcion pasado por parametro */

	const int max_c_site=300; //Los datos de una sola base no deben superar este valor
	char query[200];
	char aux[max_c_site];
	int real_size;
	int exist=0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	CORREGIR TODO

	sprintf(query,"select id,name,status from web_site where susc_id =%s",susc_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	result = mysql_store_result(db->con);

	*data=(char *)realloc(*data,max_c_site);
	real_size = max_c_site;
	strcpy(*data,"200|[");
	while(row = mysql_fetch_row(result)){
		exist = 1;
		printf("Agregando\n");
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\",\"status\":\"%s\"},",row[0],row[1],row[2]);
		if(strlen(*data)+strlen(aux)+1 > real_size){
			real_size =+ max_c_site;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}
	if(exist){
		(*data)[strlen(*data) - 1] = ']';
	} else {
		strcat(*data,"]");
	}
	// Redimencionamos para no desperdiciar memoria
	*data=(char *)realloc(*data,strlen(*data)+1);
	printf("Resultado:-%s-\n",*data);
}

void db_db_show(T_db *db, char **data, char *site_id, char *susc_id){
	/* Retorna en formato json los datos de una base mysql dada
 	 * en base al id pasado por parametro */

	char query[200];
	char aux[500];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int real_size;

	CORREGIR TODO

	sprintf(query,"select * from web_site where id =%s and susc_id=%s",site_id,susc_id);
	printf("DB_SITE_LIST: %s\n",query);
	mysql_query(db->con,query);
	if(result = mysql_store_result(db->con)){
		if(row = mysql_fetch_row(result)){
			printf("Allocamos memoria\n");
			real_size = 500;
			*data=(char *)realloc(*data,real_size);
			printf("colocamos info\n");
			sprintf(*data,"200|{\"id\":\"%s\",\"version\":\"%s\",\"name\":\"%s\",\"size\":\"%s\",\"susc_id\":\"%s\",\"status\":\"%s\",\"urls\":[",
				row[0],row[1],row[2],row[3],row[4],row[5]);
	
			/* Listado de alias */
			sprintf(query,"select id,alias from web_alias where site_id =%s",site_id);
			printf("DB_SITE_LIST: %s\n",query);
			mysql_query(db->con,query);
			result = mysql_store_result(db->con);
			while(row = mysql_fetch_row(result)){
				sprintf(aux,"{\"id\":\"%s\",\"url\":\"%s\"),",row[0],row[1]);
				//Si no entra en *data, reallocamos para que entre y un poco mas
				if(strlen(*data)+strlen(aux)+1 > real_size){
					real_size =+ 200;
					*data=(char *)realloc(*data,real_size);
				}
				strcat(*data,aux);
			}
			// Reemplazamos la última "," por "]"
			(*data)[strlen(*data) - 1] = ']';
			//Cerramos los datos con '}' y redimencionamos el string
		} else {
			real_size = 100;
			*data=(char *)realloc(*data,real_size);
			sprintf(*data,"{\"error\":\"site not exist\"");
		}
	} else {
		real_size = 100;
		*data=(char *)realloc(*data,real_size);
		sprintf(*data,"{\"error\":\"db error\"");
	}
	real_size = strlen(*data) + 2;
	*data=(char *)realloc(*data,real_size);
	strcat(*data,"}");
}

int db_db_del(T_db *db, char *db_id, uint32_t size, char *error, int *db_fail){
	/* Elimina una base mysql y todas sus componentes
	   de la base de datos dado el id */

	char query[200];

	/* Borrar entradas del ftp */
	sprintf(query,"delete from ftpuser where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	/* Liberamos de la quota de la suscripcion lo que ocupaba el sitio*/
	sprintf(query,"update ftpquotatallies set bytes_xfer_used = bytes_xfer_used - %i",size);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos alias */
	sprintf(query,"delete from web_alias where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos indices */
	sprintf(query,"delete from web_indexes where site_id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}

	/* Borramos sitio */
	sprintf(query,"delete from web_site where id=%s",site_id);
	logs_write(db->logs,L_DEBUG,"db_site_del", query);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_site_del","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	return 1;
}

int db_del_all_site(T_db *db, char *susc_id, char *error, int *db_fail){
	/* Elimina todos los sitios de una suscripcion
 	 *  dado el id de la misma */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select id from web_site where susc_id=%s",susc_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	} else {
		result = mysql_store_result(db->con);
		while(row = mysql_fetch_row(result)){
			if(!db_site_del(db,row[0],0,error,db_fail))
				return 0;
		}

		/* Eliminamos la cuota utilizada de la suscripcion */
		sprintf(query,"update ftpquotatallies set bytes_xfer_used=0 where name='g_%s'",
			susc_id);
		if(mysql_query(db->con,query)){
			*db_fail = 1;
			return 0;
		}
	}
	return 1;
}

/****	Funciones para ftp	****/
int db_limit_ftp_users(T_db *db, char *site_id, int *db_fail){
	/* retorna 0 si se supera el limite de
	 * usuarios ftp del sitio o falla la base de datos*/
	char query[300];
	MYSQL_RES *result;
        MYSQL_ROW row;

	sprintf(query,"select (select s.ftp_per_site_limit from web_suscription s inner join web_site w on (s.id = w.susc_id) where w.id=%s) - (select count(id) from ftpuser where site_id=%s)",site_id,site_id);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	row = mysql_fetch_row(result);
	return atoi(row[0]);
}

int db_ftp_add(T_db *db, T_dictionary *d, T_config *c, char *error, int *db_fail){
	/* Agrega un usuario ftp. Los siguientes datos deben venir en el diccionario
 	site_id		id del sitio
	user_id		string
	passwd		string
	*/

	char query[200];
	char homedir[200];
	MYSQL_RES *result;
	MYSQL_ROW row_site;
	MYSQL_ROW row_susc;

	/*Obtenemos mediante el site_id el nombre del sitio y el susc_id */
	printf("DB_ftp_add\n");
	*db_fail=0;
	sprintf(query,"select name, susc_id from web_site where id=%s", dictionary_get(d,"site_id"));
	if(mysql_query(db->con,query)){
		printf("DB_SITE_ADD errno: %i\n",mysql_errno(db->con));
		*db_fail=1;
		return 0;
	}
	result = mysql_store_result(db->con);
	if(mysql_num_rows(result) < 1){
		printf("Sitio inexistente\n");
		strcpy(error,"300|\"code\":\"301\",\"info\":\"Sitio inexistente para crear usuario ftp\"");
		return 0;
	}
	row_site = mysql_fetch_row(result);

	/* Obtenemos el hash_dir */
	sprintf(query,"select hash_dir from web_suscription where id=%s",row_site[1]);
	if(mysql_query(db->con,query)){
		logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		*db_fail=1;
		return 0;
	}
	result = mysql_store_result(db->con);
	if(mysql_num_rows(result) < 1){
		strcpy(error,"300|\"code\":\"301\",\"info\":\"ERROR Fatal. Sitio uerfano\"");
		return 0;
	}
	row_susc = mysql_fetch_row(result);
	
	/* Armamos el homedir */
	sprintf(homedir,"%s/%s/%s",config_webdir(c),row_susc[0],row_site[0]);
	printf("homedir armado: %s\n",homedir);

	sprintf(query,"insert into ftpuser(userid,passwd,uid,gid,homedir,site_id) values('%s/%s','%s',%s,%s,'%s',%s)",
		dictionary_get(d,"name"),row_site[0],dictionary_get(d,"passwd"),config_ftpuid(c),
		row_site[1],homedir,dictionary_get(d,"site_id"));
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		if(mysql_errno(db->con) == 1062){
			logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR: Usuario ftp repetido");
			strcpy(error,"300|\"code\":\"301\",\"info\":\"Ya existe un usuario ftp con ese nombre\"");
		} else {
			*db_fail=1;
			logs_write(db->logs,L_ERROR,"db_ftp_add","DB_ERROR");
		}
		return 0;
	}
	return 1;
}

int db_get_ftp_id(T_db *db, char *site_id, int ftp_ids[256], int *ftp_ids_len, char *error, int *db_fail ){
	/* Retorna el listado de ids de usuarios ftp de un sitio dado */
	char query[200];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;

	sprintf(query,"select id from ftpuser where site_id=%s",site_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail = 0;
	result = mysql_store_result(db->con);
	while(row = mysql_fetch_row(result)){
		ftp_ids[i] = atoi(row[0]);
		i++;
	}
	*ftp_ids_len = i;
	return 1;
}

int db_ftp_list(T_db *db, char **data, char *site_id){
	/* Retorna en **data en formato json los usuarios ftp de un sitio dado mediante el
 	 * parametro site_id. Si existen problemas con la base de datos retorna 0. Sino 1. */

	char query[200];
	char aux[200];
	MYSQL_RES *result;
        MYSQL_ROW row;
	int exist;
	int real_size = 200;

	sprintf(query,"select id, userid from ftpuser where site_id=%s",site_id);
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query))
		return 0;

	*data=(char *)realloc(*data,real_size);
	result = mysql_store_result(db->con);
	strcpy(*data,"200|[");
	while(row = mysql_fetch_row(result)){
		exist = 1;
		sprintf(aux,"{\"id\":\"%s\",\"name\":\"%s\"},",row[0],row[1]);
		if(strlen(*data)+strlen(aux)+1 > real_size){
			real_size += 100;
			*data=(char *)realloc(*data,real_size);
		}
		strcat(*data,aux);
	}

	if(exist)
		(*data)[strlen(*data) - 1] = ']';
	else
		strcat(*data,"]");

	//Acomodamos para no desperdiciar memoria
	real_size = 1 + strlen(*data);
	*data=(char *)realloc(*data,real_size);
	return 1;
}

int db_ftp_del(T_db *db, T_dictionary *d, char *error, int *db_fail){
	/* Elimina una cuenta ftp */

	char query[200];
	/* Verificamos que el usuario ftp a eliminar corresponda al sitio indicado */
	sprintf(query,"delete from ftpuser where id=%s and site_id=%s",
		dictionary_get(d,"ftp_id"),
		dictionary_get(d,"site_id"));
	printf("QEURY : %s\n",query);
	if(mysql_query(db->con,query)){
		*db_fail = 1;
		return 0;
	}
	*db_fail=0;
	if(mysql_affected_rows(db->con)==0){
		strcpy(error,"300|\"code\":\"301\",\"info\":\"Usuario ftp inexistente para el sitio\"");
		return 0;
	}
	return 1;
}

/****	Funciones para servers	****/

void db_server_stop(T_db *db, int id){
	/* Deja constancia en la base de datos del server
 	 * detenido dado el id del mismo */
	char query[200];
	sprintf(query,"update web_server set status=1 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

void db_server_start(T_db *db, int id){
	/* Deja constancia en la base de datos del server
 	 * iniciado dado el id del mismo */
	char query[200];
	sprintf(query,"update web_server set status=0 where id=%i",id);
	printf("QEURY : %s\n",query);
	mysql_query(db->con,query);
}

int db_worker_get_info(T_db *db, int id, char *data){
	/* Retorna en formato json en *data informacion guardada
	 * del worker. Retorna 0 si falla la conexion a la base */
	char query[200];
	char status[10];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select s.status, w.size from web_server s inner join web_worker w on (s.id = w.id) where s.id = %i",id);
	printf("query: %s\n",query);
	if(mysql_query(db->con,query)){
		return 0;
	}
	if(result = mysql_store_result(db->con)){
		row = mysql_fetch_row(result);
		itowstatus(atoi(row[0]),status);
		sprintf(data,"\"status\":\"%s\",\"size\":%s,",status,row[1]);
	}
	return 1;
}

int db_proxy_get_info(T_db *db, int id, char *data){
	/* Retorna en formato json en *data informacion guardada
	 * del worker. Retorna 0 si falla la conexion a la base */
	char query[200];
	char status[10];
	MYSQL_RES *result;
	MYSQL_ROW row;

	sprintf(query,"select s.status from web_server s where id = %i",id);
	printf("Query: %s\n",query);
	if(mysql_query(db->con,query))
		return 0;
	if(result = mysql_store_result(db->con)){
		row = mysql_fetch_row(result);
		itowstatus(atoi(row[0]),status);
		sprintf(data,"\"status\":%s,",status);
	}
	return 1;
}
