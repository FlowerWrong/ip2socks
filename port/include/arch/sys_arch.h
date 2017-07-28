/**
 * based on lwip-contrib
 */
#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#define SYS_MBOX_NULL NULL
#define SYS_SEM_NULL  NULL

typedef u32_t sys_prot_t;

struct sys_sem;
typedef struct sys_sem *sys_sem_t;
#define sys_sem_valid(sem)       (((sem) != NULL) && (*(sem) != NULL))
#define sys_sem_set_invalid(sem) do { if((sem) != NULL) { *(sem) = NULL; }}while(0)

struct sys_mutex;
typedef struct sys_mutex *sys_mutex_t;
#define sys_mutex_valid(mutex)       sys_sem_valid(mutex)
#define sys_mutex_set_invalid(mutex) sys_sem_set_invalid(mutex)

struct sys_mbox;
typedef struct sys_mbox *sys_mbox_t;
#define sys_mbox_valid(mbox)       sys_sem_valid(mbox)
#define sys_mbox_set_invalid(mbox) sys_sem_set_invalid(mbox)

struct sys_thread;
typedef struct sys_thread *sys_thread_t;

#endif /* LWIP_ARCH_SYS_ARCH_H */

