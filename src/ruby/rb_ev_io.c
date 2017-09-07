//
// Created by yy on 17-9-6.
//

#include "ruby.h"
#include "ruby/io.h"
#include "ev.h"

struct RB_EV_IO {
    VALUE self;
    int interests;
    struct ev_timer timer;
    struct ev_io ev_io;
};

#ifdef GetReadFile
# define FPTR_TO_FD(fptr) (fileno(GetReadFile(fptr)))
#else
# define FPTR_TO_FD(fptr) fptr->fd
#endif /* GetReadFile */

void ev_io_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct RB_EV_IO *rb_ev_io = (struct RB_EV_IO *) watcher->data;
  VALUE klass = rb_ev_io->self;
  VALUE io_cb = rb_ivar_get(klass, rb_intern("io_cb"));

  if (rb_class_of(io_cb) != rb_cProc)
    rb_raise(rb_eTypeError, "Expected Proc callback");

  rb_funcall(io_cb, rb_intern("call"), 0);
}

void ev_timer_cb(struct ev_loop *loop, ev_timer *watcher, int revents) {
  struct RB_EV_IO *rb_ev_io = (struct RB_EV_IO *) watcher->data;
  VALUE klass = rb_ev_io->self;
  VALUE timer_cb = rb_ivar_get(klass, rb_intern("timer_cb"));

  if (rb_class_of(timer_cb) != rb_cProc)
    rb_raise(rb_eTypeError, "Expected Proc callback");

  rb_funcall(timer_cb, rb_intern("call"), 0);
}

/**
 * alloc and dealloc
 */
static void rb_ev_mark(struct RB_EV_IO *rb_ev_io) {
}

static void rb_ev_free(struct RB_EV_IO *rb_ev_io) {
  xfree(rb_ev_io);
}

static VALUE rb_ev_allocate(VALUE self) {
  struct RB_EV_IO *rb_ev_io = (struct RB_EV_IO *) xmalloc(sizeof(struct RB_EV_IO));
  return Data_Wrap_Struct(self, rb_ev_mark, rb_ev_free, rb_ev_io);
}

static VALUE rb_ev_io_cb(VALUE self) {
  return rb_ivar_get(self, rb_intern("io_cb"));
}

static VALUE rb_ev_set_io_cb(VALUE self, VALUE obj) {
  return rb_ivar_set(self, rb_intern("io_cb"), obj);
}

static VALUE rb_ev_io_fd(VALUE self) {
  return rb_ivar_get(self, rb_intern("io_fd"));
}

/**
 * initialize
 */
static VALUE rb_ev_initialize(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  rb_ev_io->self = self;
  rb_ev_io->ev_io.data = (void *) rb_ev_io;
  rb_ev_io->timer.data = (void *) rb_ev_io;

  return Qnil;
}


/**
 * ev io
 */
static VALUE rb_ev_io_register(VALUE self, VALUE io_fd, VALUE interests, VALUE io_cb) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  /**
   * VALUE io
   */
  rb_ivar_set(self, rb_intern("io_fd"), io_fd);
  rb_io_t *fptr;
  GetOpenFile(rb_convert_type(io_fd, T_FILE, "IO", "to_io"), fptr); // IO.to_io
  rb_io_set_nonblock(fptr);
  int fd = FPTR_TO_FD(fptr);

  /**
   * VALUE interests
   */
  ID interests_id;
  interests_id = SYM2ID(interests);
  if (interests_id == rb_intern("r")) {
    rb_ev_io->interests = EV_READ;
  } else if (interests_id == rb_intern("w")) {
    rb_ev_io->interests = EV_WRITE;
  } else if (interests_id == rb_intern("rw")) {
    rb_ev_io->interests = EV_READ | EV_WRITE;
  } else {
    rb_raise(rb_eArgError, "invalid event type %s (must be :r, :w, or :rw)",
             RSTRING_PTR(rb_funcall(interests, rb_intern("inspect"), 0, 0)));
  }

  /**
   * VALUE io_cb
   */
  rb_ivar_set(self, rb_intern("io_cb"), io_cb);

  ev_io_init(&rb_ev_io->ev_io, ev_io_cb, fd, rb_ev_io->interests);

  return Qnil;
}

static VALUE rb_ev_io_start(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  if (!ev_is_active(&rb_ev_io->ev_io)) {
    ev_io_start(EV_DEFAULT, &rb_ev_io->ev_io);
  }

  return Qnil;
}

static VALUE rb_ev_io_stop(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  // first stop ev timer
  if (ev_is_active(&rb_ev_io->timer)) {
    ev_timer_stop(EV_DEFAULT, &rb_ev_io->timer);
  }

  if (ev_is_active(&rb_ev_io->ev_io)) {
    ev_io_stop(EV_DEFAULT, &rb_ev_io->ev_io);
  }

  return Qnil;
}


/**
 * ev timer
 */
static VALUE rb_ev_timer_register(VALUE self, VALUE timeout, VALUE repeat, VALUE timer_cb) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  /**
   * VALUE timeout double
   */
  rb_ivar_set(self, rb_intern("timeout"), timeout);

  /**
   * VALUE repeat double
   */
  rb_ivar_set(self, rb_intern("repeat"), repeat);

  /**
   * VALUE timer_cb
   */
  rb_ivar_set(self, rb_intern("timer_cb"), timer_cb);

  ev_timer_init(&rb_ev_io->timer, ev_timer_cb, NUM2DBL(timeout), NUM2DBL(repeat));

  return Qnil;
}

static VALUE rb_ev_timer_start(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  if (!ev_is_active(&rb_ev_io->timer)) {
    ev_timer_start(EV_DEFAULT, &rb_ev_io->timer);
  }

  return Qnil;
}

static VALUE rb_ev_timer_again(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  ev_timer_again(EV_DEFAULT, &rb_ev_io->timer);

  return Qnil;
}

static VALUE rb_ev_timer_stop(VALUE self) {
  struct RB_EV_IO *rb_ev_io;
  Data_Get_Struct(self, struct RB_EV_IO, rb_ev_io);

  if (ev_is_active(&rb_ev_io->timer)) {
    ev_timer_stop(EV_DEFAULT, &rb_ev_io->timer);
  }

  return Qnil;
}

void Init_rb_ev_io(void) {
  VALUE rbev = rb_define_module("Rbev");

  static VALUE rb_ev_io = Qnil;
  rb_ev_io = rb_define_class_under(rbev, "IO", rb_cObject);
  rb_define_alloc_func(rb_ev_io, (rb_alloc_func_t) rb_ev_allocate);

  rb_define_method(rb_ev_io, "initialize", rb_ev_initialize, 0);

  rb_define_method(rb_ev_io, "io_cb", rb_ev_io_cb, 0);
  rb_define_method(rb_ev_io, "io_cb=", rb_ev_set_io_cb, 1);
  rb_define_method(rb_ev_io, "io_fd", rb_ev_io_fd, 0);

  rb_define_method(rb_ev_io, "io_register", rb_ev_io_register, 3);
  rb_define_method(rb_ev_io, "io_start", rb_ev_io_start, 0);
  rb_define_method(rb_ev_io, "io_stop", rb_ev_io_stop, 0);

  rb_define_method(rb_ev_io, "timer_register", rb_ev_timer_register, 3);
  rb_define_method(rb_ev_io, "timer_start", rb_ev_timer_start, 0);
  rb_define_method(rb_ev_io, "timer_again", rb_ev_timer_again, 0);
  rb_define_method(rb_ev_io, "timer_stop", rb_ev_timer_stop, 0);
}
