#ifndef WEBIX_DEMO_SERVER_APP_H_
#define WEBIX_DEMO_SERVER_APP_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <limits.h>
#include <libsoup/soup.h>
#include <json-c/json.h>
#include <db.h>

#ifndef json_get_value
typedef char * string;
#define json_get_value(jobj, type, key) ({	\
		type value = (type)0;	\
		json_object * jvalue = NULL;	\
		json_bool ok = json_object_object_get_ex(jobj, #key, &jvalue);	\
		if(ok) value = (type)json_object_get_##type(jvalue);	\
		value;	\
	})
#endif

#define DEFAULT_LISTEN_PORT	(8081)
typedef struct http_server
{
	void * user_data;
	void * priv;
	char document_root[PATH_MAX];
	unsigned int port;
	SoupServer * server;
}http_server_t;
http_server_t * http_server_init(http_server_t * http, void * user_data);
void http_server_cleanup(http_server_t * http);

typedef struct db_helpler
{
	void * user_data;
	void * priv;
	
	char db_home[PATH_MAX];
	DB_ENV * env;
	struct { // users_db with indexes
		DB * users_db;		// primary db, key ==> "user_uuid"
		union
		{
			struct {
				DB * user_emails_sdb;	// index db, sorted by email
				DB * user_names_sdb;	// index db, sorted by username
				DB * user_phones_sdb;	// index db, sorted by phone number
				// ...
			};
			DB * users_sdbs[0];	// variable length
		};
	};

	struct {
		DB * groups_db;
		DB * group_users_db;
		DB * users_group_sdb;
	};
	
	struct {
		DB * roles_db;
		DB * role_users_db;
		DB * users_role_sdb;
	};
}db_helpler_t;
db_helpler_t * db_helpler_init(db_helpler_t * db, void * user_data);
void db_helpler_cleanup(db_helpler_t * db);

typedef struct app_context
{
	void * priv;
	void * user_data;
	json_object * jconfig;
	
	struct http_server http[1];
	struct db_helpler db[1];
	
	GMainLoop * loop;
	int is_running;
}app_context_t;
app_context_t * app_context_init(app_context_t * app, int argc, char ** argv, void * user_data);
void app_context_cleanup(app_context_t * app);

#ifdef __cplusplus
}
#endif
#endif
