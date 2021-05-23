/*
 * webapi-users.c
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

#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <json-c/json.h>
#include "app.h"

/**********************************************
 * app_context member functions
**********************************************/
static int app_init(app_context_t * app)
{
	http_server_t *http = http_server_init(app->http, app);
	assert(http);
	
	db_helpler_t * db = db_helpler_init(app->db, app);
	assert(db);
	return 0;
}

static int app_run(app_context_t * app)
{
	GMainLoop * loop = g_main_loop_new(NULL, FALSE);
	app->loop = loop;
	app->is_running = 1;
	g_main_loop_run(loop);
	app->is_running = 0;
	return 0;
}

static int app_stop(app_context_t * app)
{
	if(app->is_running) {
		app->is_running = 0;
		GMainLoop * loop = app->loop;
		app->loop = NULL;
		
		if(loop) g_main_loop_quit(loop);
	}
	return 0;
}

static json_object * generate_default_config(void)
{
	json_object * jconfig = json_object_new_object();
	assert(jconfig);
	json_object_object_add(jconfig, "port", json_object_new_int(DEFAULT_LISTEN_PORT));
	json_object_object_add(jconfig, "db_home", json_object_new_string("db"));
	return jconfig;
}

static app_context_t g_app[1];
app_context_t * app_context_init(app_context_t * app, int argc, char ** argv, void * user_data)
{
	if(NULL ==  app) app = g_app;
	const char * conf_file = "conf/config.json";
	json_object * jconfig = json_object_from_file(conf_file);
	if(NULL == jconfig) {
		jconfig = generate_default_config();
		assert(jconfig);
	}
	
	app->user_data = user_data;
	app->jconfig = jconfig;
	
	return app;
 
}
void app_context_cleanup(app_context_t * app)
{
	app_stop(app);
	http_server_cleanup(app->http);
	db_helpler_cleanup(app->db);
	
	json_object * jconfig = app->jconfig;
	app->jconfig = NULL;
	if(jconfig) json_object_put(jconfig);
	return;
}


/**********************************************
 * Main
**********************************************/
int main(int argc, char **argv)
{
	app_context_t * app = app_context_init(NULL, argc, argv, NULL);
	assert(app);
	
	app_init(app);
	app_run(app);
	app_context_cleanup(app);
	return 0;
}

