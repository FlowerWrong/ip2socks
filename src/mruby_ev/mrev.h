//
// Created by king Yang on 2017/9/2.
//

#ifndef DUKTAPE_MREV_H
#define DUKTAPE_MREV_H

#ifdef __cplusplus
extern "C" {
#endif

void mrb_mrev_gem_init(mrb_state* mrb);

void mrb_mrev_gem_final(mrb_state* mrb);

#ifdef __cplusplus
}
#endif

#endif //DUKTAPE_MREV_H
