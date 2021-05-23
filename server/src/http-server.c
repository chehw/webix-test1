/*
 * http-server.c
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

#include "app.h"
#include <libgen.h>
#include <libsoup/soup.h>
#include <jwt.h> // libjwt-dev_1.10.1

static void on_document_root(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data);
static void on_login(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data);
static void on_favicon(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data);
static void on_auth_token(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data);

/******************************************************
 * http server 
******************************************************/
http_server_t * http_server_init(http_server_t * http, void * user_data)
{
	app_context_t * app = user_data;
	assert(app);
	json_object * jconfig = app->jconfig;
	assert(jconfig);
	
	if(NULL == http) http = calloc(1, sizeof(*http));
	assert(http);
	http->user_data = app;
	
	char path_name[PATH_MAX] = "";
	ssize_t cb = readlink("/proc/self/exe", path_name, sizeof(path_name));
	assert(cb > 0);
	
	char * document_root = dirname(path_name);
	assert(document_root);
	strncpy(http->document_root, document_root, sizeof(http->document_root));
	
	unsigned int port = json_get_value(jconfig, int, port);
	if(port == 0 || port > 65535) port = DEFAULT_LISTEN_PORT;
	
	http->port = port;
	
	
	GError * gerr = NULL;
	gboolean ok = FALSE;
	SoupServerListenOptions flags = SOUP_SERVER_LISTEN_IPV4_ONLY;
	SoupServer * server = soup_server_new(SOUP_SERVER_SERVER_HEADER, "webapi-users", NULL);
	int use_ssl = json_get_value(jconfig, int, use_ssl);
	if(use_ssl) {
		const char * cert_file = json_get_value(jconfig, string, cert_file);
		const char * key_file = json_get_value(jconfig, string, key_file);
		
		assert(cert_file && key_file);
		ok = soup_server_set_ssl_cert_file(server, cert_file, key_file, &gerr);
		assert(ok && NULL == gerr);
		
		flags |= SOUP_SERVER_LISTEN_HTTPS;
	}
	
	soup_server_add_handler(server, "/", on_document_root, app, NULL);
	soup_server_add_handler(server, "/favicon.ico", on_favicon, app, NULL);
	soup_server_add_handler(server, "/login", on_login, app, NULL);
	soup_server_add_handler(server, "/auth", on_auth_token, app, NULL);
	
	ok = soup_server_listen_all(server, port, flags, &gerr);
	assert(ok && NULL == gerr);
	http->server = server;
	
	GSList * uris = soup_server_get_uris(server);
	assert(uris);
	for(GSList * uri = uris; uri; uri = uri->next) {
		if(uri->data) {
			fprintf(stderr, "Listening on: %s\n", soup_uri_to_string(uri->data, FALSE));
			soup_uri_free(uri->data);
			uri->data = NULL;
		}
	}
	g_slist_free(uris);
	
	return http;
}
void http_server_cleanup(http_server_t * http)
{
	return;
}

/******************************************************
 * http server message handlers
******************************************************/
static void on_document_root(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data)
{
	SoupMessageHeaders * req_headers = msg->request_headers;
	SoupMessageHeaders * resp_headers = msg->response_headers;
	assert(req_headers && resp_headers);
	
	const char * auth = soup_message_headers_get_one(req_headers, "Authorization");
	if(NULL == auth || strncasecmp(auth, "Bearer ", sizeof("Bearer")) != 0) {
		static const char * redirect_page = "/login";
		soup_message_headers_append(resp_headers, "Location", redirect_page);
		soup_message_set_status(msg, SOUP_STATUS_FOUND); // 302 found
		return;
	}
	
	// todo: verify token
	// ...
	
	SoupMessageBody * body = msg->response_body;
	soup_message_headers_set_content_type(resp_headers, "text/plain", NULL);
	
	char sz_resp[PATH_MAX] = "";
	ssize_t cb_resp = snprintf(sz_resp, sizeof(sz_resp), 
		"method: %s, \npath: %s, \nquery=%p\n"
		"Authorization: %s\n",
		msg->method, path, query,
		auth
	); 
	soup_message_body_append(body, SOUP_MEMORY_COPY, sz_resp, cb_resp);
	soup_message_set_status(msg, SOUP_STATUS_OK);
	return;
}
static void on_login(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data)
{
	// todo
	if(msg->method == SOUP_METHOD_GET) {
		// todo: show login page
		soup_message_set_status(msg, SOUP_STATUS_OK);
		return;
	}
	
	if(msg->method != SOUP_METHOD_POST) {
		soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
		return;
	}
	
	// TODO: 
	// 
	soup_message_set_status(msg, SOUP_STATUS_UNAUTHORIZED);
	return;
}

static void on_favicon(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
static void on_auth_token(SoupServer * server, SoupMessage * msg, const char * path, GHashTable * query, SoupClientContext * client, gpointer user_data)
{
	// TODO:
	// 
	if(msg->method == SOUP_METHOD_GET) {
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
		return;
	}
	
	if(msg->method != SOUP_METHOD_POST) {
		soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
		return;
	}
	
	soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
	return;
}
