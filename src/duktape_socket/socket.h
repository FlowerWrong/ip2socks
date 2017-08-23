//
// Created by king Yang on 2017/8/22.
//

#ifndef DUKTAPE_SOCKET_H
#define DUKTAPE_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

static void set_nonblocking(duk_context *ctx, int fd);

static void set_reuseaddr(duk_context *ctx, int fd);

#if defined(__APPLE__)

static void set_nosigpipe(duk_context *ctx, int fd);

#endif

static int socket_create_server_socket(duk_context *ctx);

static int socket_close(duk_context *ctx);

static int socket_accept(duk_context *ctx);

static int socket_connect(duk_context *ctx);

static int socket_read(duk_context *ctx);

static int socket_write(duk_context *ctx);

void socket_register(duk_context *ctx);

#ifdef __cplusplus
}
#endif

#endif //DUKTAPE_SOCKET_H
