/*
 * db_helpler.c
 * 
 * Copyright 2021 chehw <hongwei.che@gmail.com>
 * 
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in 
 * the Software without restriction, including without limitation the rights to 
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
 * of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <stdarg.h>
#include <db.h>
#include <uuid/uuid.h>
#include "app.h"

#define db_check_error(rc) do { \
		if(0 == rc) break; \
		fprintf(stderr, "%s()@%d: %s\n", __FUNCTION__, __LINE__, db_strerror(rc)); \
		exit(1); \
	}while(0)

static int init_databases(db_helpler_t * db, DB_ENV * env);
static void close_databases(db_helpler_t * db);
db_helpler_t * db_helpler_init(db_helpler_t * db, void * user_data)
{
	app_context_t * app = user_data;
	assert(app);
	json_object * jconfig = app->jconfig;
	assert(jconfig);
	
	if(NULL == db) db = calloc(1, sizeof(*db));
	assert(db);
	db->user_data = app;
	
	const char * db_home = json_get_value(jconfig, string, db_home);
	if(NULL == db_home) db_home = "db";
	strncpy(db->db_home, db_home, sizeof(db->db_home));
	
	DB_ENV * env = NULL;
	int rc = db_env_create(&env, 0);
	assert(0 == rc);
	
	int env_flags = DB_CREATE | DB_INIT_MPOOL 
		| DB_INIT_LOG  // Initialize the logging subsystem. This subsystem should be used when recovery from application or system failure is necessary. 
		| DB_INIT_TXN  // Initialize the transaction subsystem. 
		| DB_INIT_LOCK // Initialize the locking subsystem.
		| DB_INIT_REP  // Initialize the replication subsystem. 
		| 0;
	
	rc = env->open(env, db_home, env_flags, 0664);
	assert(0 == rc);
	
	init_databases(db, env);
	return db;
}
void db_helpler_cleanup(db_helpler_t * db)
{
	close_databases(db);
	return;
}




typedef int (* sdb_associate_fn)(DB *, const DBT *, const DBT *, DBT *);
struct index_db_desc
{
	const char * sdb_name;
	sdb_associate_fn fn;
};

struct user_data
{
	char name[128];
	char email[128];
	char phone[32];
	// ...
};
struct user_model
{
	uuid_t uid;
	struct user_data user;
};



static int associate_user_name(DB * sdbp, const DBT * key, const DBT * value, DBT * skey)
{
	struct user_data * user = (void *)value->data;
	assert(user);
	memset(skey, 0, sizeof(*skey));
	skey->data = user->name;
	skey->size = strlen(user->name) + 1;
	return 0;
}
static int associate_user_email(DB * sdbp, const DBT * key, const DBT * value, DBT * skey)
{
	struct user_data * user = (void *)value->data;
	assert(user);
	memset(skey, 0, sizeof(*skey));
	skey->data = user->email;
	skey->size = strlen(user->email) + 1;
	return 0;
}
static int associate_user_phone(DB * sdbp, const DBT * key, const DBT * value, DBT * skey)
{
	struct user_data * user = (void *)value->data;
	assert(user);
	memset(skey, 0, sizeof(*skey));
	skey->data = user->phone;
	skey->size = strlen(user->phone) + 1;
	return 0;
}

static int init_databases(db_helpler_t * db, DB_ENV * env)
{
	// TODO:
	
	DB * dbp = NULL;
	DB * sdbp = NULL;
	int rc = 0;
	
	rc = db_create(&dbp, env, 0);
	if(rc) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, db_strerror(rc));
		exit(1);
	}
	
	const int mode = 0666;
	int db_flags = DB_AUTO_COMMIT | DB_CREATE;
	rc = dbp->open(dbp, NULL, "users.db", NULL, DB_BTREE, db_flags, mode);
	db_check_error(rc);
	db->users_db = dbp;
	
	
	static struct index_db_desc index_users_db[] = {
		[0] = { "user-names.sdb",  associate_user_name },
		[1] = { "user-emails.sdb", associate_user_email },
		[2] = { "user-phones.sdb", associate_user_phone },
		{ NULL, }
	};
	
	for(int i = 0; ; ++i) {
		struct index_db_desc * desc = &index_users_db[i];
		if(NULL == desc->sdb_name) break;
		sdbp = NULL;
		rc = db_create(&sdbp, env, 0); 
		db_check_error(rc);
		
		rc = sdbp->set_flags(sdbp, DB_DUPSORT);	// support duplicates for index db
		db_check_error(rc);
		
		rc = sdbp->open(sdbp, NULL, desc->sdb_name, NULL, DB_BTREE, db_flags, mode);
		db_check_error(rc);
		
		dbp->associate(dbp, NULL, sdbp, desc->fn, DB_CREATE);
		db_check_error(rc);
		
		db->users_sdbs[i] = sdbp;
	}
	
	return -1;
}
static void close_databases(db_helpler_t * db)
{
	// TODO
	
	
	return;
}
