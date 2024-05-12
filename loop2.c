#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

uv_loop_t *loop;

void on_close(uv_handle_t *handle) {
    free(handle);
}


void on_write(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "Error al escribir en el socket del cliente: %s\n", uv_strerror(status));
    }
    // Liberar la memoria asignada a uv_write_t
    free(req->bufs);
    free(req);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    printf("sg: %ld\n", suggested_size);
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}


void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    printf("nread %ld\n", nread);
    if (nread > 0) {
        printf("Datos recibidos: \n%.*s\n", (int) nread, buf->base);

        // Respuesta al cliente
        const char *message = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 13\r\n"
                              "\r\n"
                              "Hola Cliente\n";
        printf("message len %ld\n", strlen(message));
        uv_buf_t res_buf = uv_buf_init((char *) message, strlen(message));
        uv_write_t *write_req = (uv_write_t *) malloc(sizeof(uv_write_t));
        uv_write(write_req, stream, &res_buf, 1, on_write);

    }
    else if (nread < 0) {
        printf("aqui 1\n");
        if (nread != UV_EOF)
            fprintf(stderr, "Error al leer datos: %s\n", uv_strerror(nread));
        uv_close((uv_handle_t*) stream, on_close);
    }

    if (buf->base){
        printf("liberando buf de request\n");
        free(buf->base);
    }
}

void on_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "Error en la conexión: %s\n", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        printf("Nueva conexión establecida.\n");
        uv_read_start((uv_stream_t*) client, alloc_buffer, on_read);
    }
    else {
        uv_close((uv_handle_t*) client, on_close);
    }
}


int main() {
    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 8079, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) &server, 128, on_connection);
    if (r != 0) {
        fprintf(stderr, "Error al escuchar: %s\n", uv_strerror(r));
        return 1;
    }

    printf("Servidor escuchando en el puerto 8079...\n");

    return uv_run(loop, UV_RUN_DEFAULT);
}
