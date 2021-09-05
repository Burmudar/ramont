#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <uv.h>

#define SOCKET_PATH "uv.socket"

uv_loop_t *loop;

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

long current_millis() {
    struct timeval time;
    gettimeofday(&time, NULL);

    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

char* time_now() {
    char* buff = malloc(sizeof(char) * 80);
    char b[26];
    time_t t;
    time(&t);
    strftime(b, 26, "%Y-%m-%d %H:%M:%S", localtime(&t));
    sprintf(buff, "%s.%0.3ld", b, current_millis());
    return buff;
}

void process_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        char * now = time_now();
        fprintf(stderr, "time: %s buff: %s\n", now, buf->base);
        free(now);
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        }
        uv_close((uv_handle_t*) client, NULL);
    }

    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status == -1) {
        return;
    }

    uv_pipe_t *client = (uv_pipe_t *) malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, client, 0);

    if (uv_accept(server, (uv_stream_t *) client) == 0) {
        uv_read_start((uv_stream_t *)client, alloc_buffer, process_data);
    } else {
        uv_close((uv_handle_t *)client, NULL);
    }
}

void remove_sock(int sig) {
    uv_fs_t req;

    uv_fs_unlink(loop, &req, SOCKET_PATH, NULL);
    exit(0);
}

int main() {

    loop = uv_default_loop();

    uv_pipe_t server;
    uv_pipe_init(loop, &server ,0);

    signal(SIGINT, remove_sock);

    int r;
    if ((r = uv_pipe_bind(&server, SOCKET_PATH))) {
        fprintf(stderr, "Bind error: %s\n", uv_err_name(r));
    }

    if ((r = uv_listen((uv_stream_t *)&server, 128, on_new_connection))) {
        fprintf(stderr, "Listen error: %s\n", uv_err_name(r));
        return 2;
    }

    return uv_run(loop, UV_RUN_DEFAULT);
}
