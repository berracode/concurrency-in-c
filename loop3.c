#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

uv_loop_t *loop;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_response(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
     if (nread < 0) {
         if (nread != UV_EOF)
             fprintf(stderr, "Read error %s\n", uv_err_name(nread));
         uv_close((uv_handle_t*) client, NULL);
         free(buf->base);
         free(client);
         return;
     }
 
     char *data = (char*) malloc(sizeof(char) * (nread+1));
     data[nread] = '\0';
     strncpy(data, buf->base, nread);
 
     fprintf(stderr, "%s", data);
     free(data);
     free(buf->base);
 }

void on_write(uv_write_t* wreq, int status) {
  if (status) {
    fprintf(stderr, "uv_write error: %s\n", uv_err_name(status));
    free(wreq);
    return;
  }

  uv_read_start((uv_stream_t*) wreq->handle, alloc_buffer, on_response);
  free(wreq);
}

void on_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
        free(req);
        return;
    }

    char *for_send = "{.... text-message ....}";

    uv_buf_t bfr = uv_buf_init(for_send, strlen(for_send));

    uv_write_t *info_req = (uv_write_t*) malloc(sizeof(uv_write_t));
    uv_write(info_req, (uv_stream_t*) req->handle, &bfr, 1, on_write);
}

int main() {
  loop = uv_default_loop();
  const char *serv_ip = "0.0.0.0";
  const short serv_port = 8079;

  uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, socket); uv_tcp_keepalive(socket, 1, 60);
  uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));

  struct sockaddr_in dest;
  uv_ip4_addr(serv_ip, serv_port, &dest);
  uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);

  return uv_run(loop, UV_RUN_DEFAULT);
}