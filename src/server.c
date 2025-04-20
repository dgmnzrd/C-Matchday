// src/server.c

#include "mongoose.h"
#include "auth.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>

#define WEB_ROOT "public"   // <— cambia a "public"

static void on_http_event(struct mg_connection *c, int ev, void *ev_data) {
  if (ev != MG_EV_HTTP_MSG) return;
  struct mg_http_message *hm = (struct mg_http_message *) ev_data;
  struct mg_http_serve_opts opts = { .root_dir = WEB_ROOT };
  char path[512];

  // POST /login
  if (hm->method.len==4 && strncmp(hm->method.buf,"POST",4)==0 &&
      hm->uri.len   ==6 && strncmp(hm->uri.buf,"/login",6)==0) {
    cJSON *js = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    const cJSON *email = cJSON_GetObjectItem(js,"email");
    const cJSON *pw    = cJSON_GetObjectItem(js,"password");
    char *token = NULL;
    if (cJSON_IsString(email) && cJSON_IsString(pw) &&
        firebase_login(email->valuestring,pw->valuestring,&token)) {
      mg_http_reply(c,200,"Content-Type:application/json\r\n",
                    "{\"idToken\":\"%s\"}",token);
      free(token);
    } else {
      mg_http_reply(c,401,"Content-Type:text/plain\r\n","Login failed");
    }
    cJSON_Delete(js);
    return;
  }

  // POST /register
  if (hm->method.len==4 && strncmp(hm->method.buf,"POST",4)==0 &&
      hm->uri.len   ==9 && strncmp(hm->uri.buf,"/register",9)==0) {
    cJSON *js = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
    const cJSON *email = cJSON_GetObjectItem(js,"email");
    const cJSON *pw    = cJSON_GetObjectItem(js,"password");
    char *token = NULL;
    if (cJSON_IsString(email) && cJSON_IsString(pw) &&
        firebase_register(email->valuestring,pw->valuestring,&token)) {
      mg_http_reply(c,200,"Content-Type:application/json\r\n",
                    "{\"idToken\":\"%s\"}",token);
      free(token);
    } else {
      mg_http_reply(c,400,"Content-Type:text/plain\r\n","Registration failed");
    }
    cJSON_Delete(js);
    return;
  }

  // GET "/" -> index.html
  if (hm->method.len==3 && strncmp(hm->method.buf,"GET",3)==0 &&
      hm->uri.len==1 && hm->uri.buf[0]=='/') {
    mg_http_serve_file(c,hm, WEB_ROOT "/index.html", &opts);
    return;
  }
  // GET "/login" -> login.html
  if (hm->method.len==3 && strncmp(hm->method.buf,"GET",3)==0 &&
      hm->uri.len==6 && strncmp(hm->uri.buf,"/login",6)==0) {
    mg_http_serve_file(c,hm, WEB_ROOT "/login.html", &opts);
    return;
  }
  // GET "/register" -> register.html
  if (hm->method.len==3 && strncmp(hm->method.buf,"GET",3)==0 &&
      hm->uri.len==9 && strncmp(hm->uri.buf,"/register",9)==0) {
    mg_http_serve_file(c,hm, WEB_ROOT "/register.html", &opts);
    return;
  }

  // 4) El resto de estáticos: /css/*, /js/*, imágenes, etc.
  mg_http_serve_dir(c,hm,&opts);
}

void initialize_server(const char *host, int port) {
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);

  char addr[64];
  snprintf(addr,sizeof(addr),"http://%s:%d",host,port);
  if (!mg_http_listen(&mgr,addr,on_http_event,NULL)) {
    fprintf(stderr,"ERROR: no pude escuchar en %s\n",addr);
    return;
  }

  printf("Servidor en %s, sirviendo %s/\n",addr,WEB_ROOT);
  for (;;) mg_mgr_poll(&mgr,1000);
  mg_mgr_free(&mgr);
}