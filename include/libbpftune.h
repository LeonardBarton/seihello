/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/* Copyright (c) 2023, Oracle and/or its affiliates. */

#ifndef __LIBBPFTUNE_H
#define __LIBBPFTUNE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <linux/types.h>

#include "bpftune.h"

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#define BPFTUNER_CGROUP_DIR		"/tmp/cgroupv2"
#define BPFTUNER_LIB_DIR		"/usr/lib64/bpftune/"
#define BPFTUNER_LIB_SUFFIX		"_tuner.so"

#define BPFTUNE_PROC_SYS		"/proc/sys/"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)			(sizeof(arr) / sizeof((arr)[0])) 
#endif

int bpftune_log_level(void);

void bpftune_log(int level, const char *fmt, ...);

void bpftune_log_stderr(void *ctx, int level, const char *fmt, va_list args);
void bpftune_log_syslog(void *ctx, int level, const char *fmt, va_list args);

void bpftune_set_log(int level,
		     void (*logfn)(void *ctx, int level, const char *fmt,
				   va_list args));
void bpftune_log_bpf_err(int err, const char *fmt);


int bpftune_cgroup_init(const char *cgroup_path);
const char *bpftune_cgroup_name(void);
int bpftune_cgroup_fd(void);
void bpftune_cgroup_fini(void);
int bpftuner_cgroup_attach(struct bpftuner *tuner, const char *prog_name,
			   enum bpf_attach_type attach_type);
void bpftuner_cgroup_detach(struct bpftuner *tuner, const char *prog_name,
			    enum bpf_attach_type attach_type);


struct bpftuner *bpftuner_init(const char *path);
int __bpftuner_bpf_load(struct bpftuner *tuner, const char **optionals);
int __bpftuner_bpf_attach(struct bpftuner *tuner);

int bpftuner_tunables_init(struct bpftuner *tuner, unsigned int num_descs,
			   struct bpftunable_desc *descs,
			   unsigned int num_scenarios,
			   struct bpftunable_scenario *scenarios);
struct bpftunable *bpftuner_tunable(struct bpftuner *tuner, unsigned int index);
unsigned int bpftuner_num_tunables(struct bpftuner *tuner);

static inline const char *bpftuner_tunable_name(struct bpftuner *tuner,
						unsigned int index)
{
	struct bpftunable *t = bpftuner_tunable(tuner, index);
	return t ? t->desc.name : NULL;
}

#define bpftuner_for_each_tunable(tuner, tunable)			     \
	for (unsigned int __itun = 0; (tunable = bpftuner_tunable(tuner, __itun)); __itun++)

int bpftuner_tunable_sysctl_write(struct bpftuner *tuner,
				  unsigned int tunable,
				  unsigned int scenario,
				  int netns_fd,
				  __u8 num_values, long *values,
				  const char *fmt, ...);

int bpftuner_tunable_update(struct bpftuner *tuner,
			    unsigned int tunable,
			    unsigned int scenario,
			    int netns_fd,
			    const char *fmt, ...);

struct bpftuner *bpftune_tuner(unsigned int index);
unsigned int bpftune_tuner_num(void);
#define bpftune_for_each_tuner(tuner)					     \
	for (unsigned int __it = 0; (tuner = bpftune_tuner(__it)) != NULL; __it++)

void bpftuner_fini(struct bpftuner *tuner, enum bpftune_state state);
void bpftuner_bpf_fini(struct bpftuner *tuner);
void bpftuner_tunables_fini(struct bpftuner *tuner);

/* need a macro in order to generate code for skeleton-specific struct */

#define bpftuner_bpf_open(tuner_name, tuner)				     \
	do {								     \
		struct tuner_name##_tuner_bpf *__skel;                       \
		struct tuner_name##_tuner_bpf_legacy *__lskel;		     \
                int __err;                                                   \
                                                                             \
                tuner->name = #tuner_name;                                   \
		tuner->bpf_legacy = bpftuner_bpf_legacy();		     \
                if (!tuner->bpf_legacy) {				     \
			tuner->skel = __skel = tuner_name##_tuner_bpf__open();\
			tuner->skeleton = __skel->skeleton;		     \
			__skel->bss->debug = bpftune_log_level() >= LOG_DEBUG;\
			__skel->bss->bpftune_pid = getpid();		     \
			tuner->obj = __skel->obj;			     \
			tuner->ring_buffer_map = __skel->maps.ring_buffer_map;\
			tuner->corr_map = __skel->maps.corr_map;	     \
		} else {						     \
			tuner->skel = __lskel = tuner_name##_tuner_bpf_legacy__open();\
			tuner->skeleton = __lskel->skeleton;		     \
			__lskel->bss->debug = bpftune_log_level() >= LOG_DEBUG;\
			__lskel->bss->bpftune_pid = getpid();		     \
			tuner->obj = __lskel->obj;			     \
			tuner->ring_buffer_map = __lskel->maps.ring_buffer_map;\
			tuner->corr_map = __lskel->maps.corr_map;	     \
		}							     \
                __err = libbpf_get_error(tuner->skel);                       \
                if (__err) {                                                 \
                        bpftune_log_bpf_err(__err,                           \
                                            #tuner_name " open bpf: %s\n");  \
                        return __err;                                        \
                }                                                            \
	} while (0)

#define bpftuner_bpf_destroy(tuner_name, tuner)				     \
	do {								     \
		if (!tuner->bpf_legacy)					     \
			tuner_name##_tuner_bpf__destroy(tuner->skel);	     \
		else							     \
			tuner_name##_tuner_bpf_legacy__destroy(tuner->skel); \
	} while (0)

#define _bpftuner_bpf_load(tuner_name, tuner, optionals)		     \
	do {								     \
		struct tuner_name##_tuner_bpf *__skel = tuner->skel;	     \
		struct tuner_name##_tuner_bpf_legacy *__lskel = tuner->skel; \
		int __err;						     \
									     \
		__err = __bpftuner_bpf_load(tuner, optionals);		     \
		if (__err) {						     \
			bpftuner_bpf_destroy(tuner_name, tuner);	     \
			return __err;					     \
		}							     \
		if (!tuner->bpf_legacy)					     \
			__skel->bss->tuner_id = bpftune_tuner_num();	     \
		else							     \
			__lskel->bss->tuner_id = bpftune_tuner_num();	     \
	} while (0)

#define bpftuner_bpf_load(tuner_name, tuner)				     \
	_bpftuner_bpf_load(tuner_name, tuner, NULL)

#define bpftuner_bpf_attach(tuner_name, tuner, optionals)		     \
	do {								     \
                int __err;                                                   \
                                                                             \
                __err = __bpftuner_bpf_attach(tuner);			     \
		if (__err && optionals != NULL) {			     \
			bpftuner_bpf_fini(tuner);			     \
			bpftuner_bpf_open(tuner_name, tuner);		     \
			_bpftuner_bpf_load(tuner_name, tuner, optionals);    \
			__err = __bpftuner_bpf_attach(tuner);		     \
		}							     \
                if (__err) {                                                 \
			bpftuner_bpf_destroy(tuner_name, tuner);	     \
                        return __err;                                        \
		}							     \
	} while (0)

#define bpftuner_bpf_init(tuner_name, tuner, optionals)			     \
	do {								     \
		bpftuner_bpf_open(tuner_name, tuner);			     \
		bpftuner_bpf_load(tuner_name, tuner);			     \
		bpftuner_bpf_attach(tuner_name, tuner, optionals);	     \
	} while (0)


enum bpftune_support_level {
	BPFTUNE_NONE = -1,
	BPFTUNE_LEGACY,
	BPFTUNE_NORMAL
};

enum bpftune_entity {
	BPFTUNE_PROG,
	BPFTUNE_MAP,
	BPFTUNE_NETNS,
};

struct bpftune_support {
	enum bpftune_entity entity;
	const char *name;
	int id;
	bool required;
	bool legacy_required;
};

enum bpftune_support_level bpftune_bpf_support(void);
void bpftuner_force_bpf_legacy(void);
bool bpftuner_bpf_legacy(void);
int bpftuner_ring_buffer_map_fd(struct bpftuner *tuner);
void *bpftune_ring_buffer_init(int ringbuf_map_fd, void *ctx);
int bpftune_ring_buffer_poll(void *ring_buffer, int interval);
void bpftune_ring_buffer_fini(void *ring_buffer);

void bpftune_sysctl_name_to_path(const char *name, char *path, size_t path_sz);
int bpftune_sysctl_read(int netns_fd, const char *name, long *values);
int bpftune_sysctl_write(int netns_fd, const char *name, __u8 num_values, long *values);

bool bpftune_netns_cookie_supported(void);
int bpftune_netns_set(int fd, int *orig_fd);
int bpftune_netns_info(int pid, int *fd, unsigned long *cookie);
int bpftune_netns_init_all(void);
void bpftuner_netns_init(struct bpftuner *tuner, unsigned long cookie);
void bpftuner_netns_fini(struct bpftuner *tuner, unsigned long cookie);
struct bpftuner_netns *bpftuner_netns_from_cookie(unsigned long tuner_id, unsigned long cookie);
int bpftune_netns_fd_from_cookie(unsigned long cookie);

#define bpftuner_for_each_netns(tuner, netns)				\
	for (netns = &tuner->netns; netns != NULL; netns = netns->next)

int bpftune_module_load(const char *name);
int bpftune_module_delete(const char *name);

#endif /* __LIBBPFTUNE_H */
