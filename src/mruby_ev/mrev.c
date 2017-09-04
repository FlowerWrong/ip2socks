//
// Created by king Yang on 2017/9/2.
//

#include <mruby.h>
#include <stdio.h>
#include <assert.h>
#include "ev.h"


struct NIO_Selector {
    struct ev_loop *ev_loop;
    struct ev_timer timer; /* for timeouts */
    struct ev_io wakeup;

    int ready_count;
    int closed, selecting;
    int wakeup_reader, wakeup_writer;
    volatile int wakeup_fired;

    mrb_value ready_array;
};

struct NIO_callback_data {
    mrb_value *monitor;
    struct NIO_Selector *selector;
};

struct NIO_Monitor {
    mrb_value self;
    int interests, revents;
    struct ev_io ev_io;
    struct NIO_Selector *selector;
};

struct NIO_ByteBuffer {
    char *buffer;
    int position, limit, capacity, mark;
};


static mrb_value
mrb_hello_world(mrb_state *mrb, mrb_value self) {
  puts("A C Extension hello world");
  return self;
}

mrb_value
mrb_block(mrb_state *mrb, mrb_value self)
{
  mrb_value blk;

  mrb_get_args(mrb, "&", &blk);

  printf("start mrb_block\n");

  mrb_yield_argv(mrb, blk, 0, NULL);

  printf("end mrb_block\n");

  return mrb_nil_value();
}

/* libev callback fired whenever a monitor gets an event */
//void mrb_callback(struct ev_loop *ev_loop, struct ev_io *io, int revents) {
//  struct NIO_Monitor *monitor_data = (struct NIO_Monitor *) io->data;
//  struct NIO_Selector *selector = monitor_data->selector;
//  mrb_value monitor = monitor_data->self;
//
//  assert(selector != 0);
//  selector->ready_count++;
//  monitor_data->revents = revents;
//
//  if (rb_block_given_p()) {
//    mrb_yield(monitor);
//  } else {
//    assert(selector->ready_array != Qnil);
//    rb_ary_push(selector->ready_array, monitor);
//  }
//}

void
mrb_mrev_gem_init(mrb_state *mrb) {
  struct RClass *class_cextension = mrb_define_module(mrb, "Mrev");
  mrb_define_class_method(mrb, class_cextension, "hello_world", mrb_hello_world, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_cextension, "blk", mrb_block, MRB_ARGS_NONE());
}

void
mrb_mrev_gem_final(mrb_state *mrb) {
  /* finalizer */
}
