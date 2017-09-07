//
// Created by yy on 17-9-6.
//

#include "ruby.h"
#include "ruby/io.h"
#include "ev.h"

struct RB_EV_IO_MONITOR {
    VALUE self;
    int interests;
    // int revents;
    struct ev_io ev_io;
};

#ifdef GetReadFile
# define FPTR_TO_FD(fptr) (fileno(GetReadFile(fptr)))
#else
# define FPTR_TO_FD(fptr) fptr->fd
#endif /* GetReadFile */

void ev_io_cb(struct ev_loop *loop, struct ev_io *io, int revents) {
  struct RB_EV_IO_MONITOR *monitor = (struct RB_EV_IO_MONITOR *) io->data;
  VALUE klass = monitor->self;
  VALUE io_cb = rb_ivar_get(klass, rb_intern("io_cb"));

  if (rb_class_of(io_cb) != rb_cProc)
    rb_raise(rb_eTypeError, "Expected Proc callback");

  rb_funcall(io_cb, rb_intern("call"), 0);
}

static void rb_ev_mark(struct RB_EV_IO_MONITOR *monitor) {
}

static void rb_ev_free(struct RB_EV_IO_MONITOR *monitor) {
  xfree(monitor);
}

static VALUE rb_ev_allocate(VALUE self) {
  struct RB_EV_IO_MONITOR *monitor = (struct RB_EV_IO_MONITOR *) xmalloc(sizeof(struct RB_EV_IO_MONITOR));
  return Data_Wrap_Struct(self, rb_ev_mark, rb_ev_free, monitor);
}

static VALUE rb_ev_io_cb(VALUE self) {
  return rb_ivar_get(self, rb_intern("io_cb"));
}

static VALUE rb_ev_set_io_cb(VALUE self, VALUE obj) {
  return rb_ivar_set(self, rb_intern("io_cb"), obj);
}

static VALUE rb_ev_io(VALUE self) {
  return rb_ivar_get(self, rb_intern("io"));
}

static VALUE rb_ev_initialize(VALUE self) {
  struct RB_EV_IO_MONITOR *monitor;
  Data_Get_Struct(self, struct RB_EV_IO_MONITOR, monitor);

  monitor->self = self;
  monitor->ev_io.data = (void *) monitor;

  return Qnil;
}

static VALUE rb_ev_io_register(VALUE self, VALUE io, VALUE interests, VALUE io_cb) {
  struct RB_EV_IO_MONITOR *monitor;
  Data_Get_Struct(self, struct RB_EV_IO_MONITOR, monitor);

  /**
   * VALUE io
   */
  rb_ivar_set(self, rb_intern("io"), io);
  rb_io_t *fptr;
  GetOpenFile(rb_convert_type(io, T_FILE, "IO", "to_io"), fptr); // IO.to_io
  rb_io_set_nonblock(fptr);
  int fd = FPTR_TO_FD(fptr);

  /**
   * VALUE interests
   */
  ID interests_id;
  interests_id = SYM2ID(interests);
  if (interests_id == rb_intern("r")) {
    monitor->interests = EV_READ;
  } else if (interests_id == rb_intern("w")) {
    monitor->interests = EV_WRITE;
  } else if (interests_id == rb_intern("rw")) {
    monitor->interests = EV_READ | EV_WRITE;
  } else {
    rb_raise(rb_eArgError, "invalid event type %s (must be :r, :w, or :rw)",
             RSTRING_PTR(rb_funcall(interests, rb_intern("inspect"), 0, 0)));
  }

  /**
   * VALUE io_cb
   */
  rb_ivar_set(self, rb_intern("io_cb"), io_cb);

  ev_io_init(&monitor->ev_io, ev_io_cb, fd, monitor->interests);

  return Qnil;
}

static VALUE rb_ev_io_start(VALUE self) {
  struct RB_EV_IO_MONITOR *monitor;
  Data_Get_Struct(self, struct RB_EV_IO_MONITOR, monitor);
  ev_io_start(EV_DEFAULT, &monitor->ev_io);

  return Qnil;
}

static VALUE rb_ev_io_stop(VALUE self) {
  struct RB_EV_IO_MONITOR *monitor;
  Data_Get_Struct(self, struct RB_EV_IO_MONITOR, monitor);
  ev_io_stop(EV_DEFAULT, &monitor->ev_io);

  return Qnil;
}

void Init_rbev(void) {
  VALUE rbev = rb_define_module("Rbev");

  static VALUE monitor = Qnil;
  monitor = rb_define_class_under(rbev, "Monitor", rb_cObject);
  rb_define_alloc_func(monitor, (rb_alloc_func_t) rb_ev_allocate);

  rb_define_method(monitor, "initialize", rb_ev_initialize, 0);
  rb_define_method(monitor, "io_cb", rb_ev_io_cb, 0);
  rb_define_method(monitor, "io_cb=", rb_ev_set_io_cb, 1);
  rb_define_method(monitor, "io", rb_ev_io, 0);
  rb_define_method(monitor, "io_register", rb_ev_io_register, 3);
  rb_define_method(monitor, "io_start", rb_ev_io_start, 0);
  rb_define_method(monitor, "io_stop", rb_ev_io_stop, 0);
}
