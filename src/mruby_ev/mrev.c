//
// Created by king Yang on 2017/9/2.
//

#include <mruby.h>
#include <stdio.h>
#include <assert.h>
#include "ev.h"

static mrb_value
mrb_hello_world(mrb_state *mrb, mrb_value self)
{
  puts("A C Extension hello world");
  return self;
}

/* libev callback fired whenever a monitor gets an event */
// void mrb_callback(ev_loop *ev_loop, struct ev_io *io, int revents)
// {
//     struct NIO_Monitor *monitor_data = (struct NIO_Monitor *)io->data;
//     struct NIO_Selector *selector = monitor_data->selector;
//     VALUE monitor = monitor_data->self;
//
//     assert(selector != 0);
//     selector->ready_count++;
//     monitor_data->revents = revents;
//
//     if(rb_block_given_p()) {
//         rb_yield(monitor);
//     } else {
//         assert(selector->ready_array != Qnil);
//         rb_ary_push(selector->ready_array, monitor);
//     }
// }

void
mrb_mrev_gem_init(mrb_state* mrb) {
  struct RClass *class_cextension = mrb_define_module(mrb, "Mrev");
  mrb_define_class_method(mrb, class_cextension, "hello_world", mrb_hello_world, MRB_ARGS_NONE());
}

void
mrb_mrev_gem_final(mrb_state* mrb) {
  /* finalizer */
}
