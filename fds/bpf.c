/* bpf FDs */

#include "config.h"
#ifdef USE_BPF

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/bpf.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <linux/unistd.h>
#include <linux/perf_event.h>

#include "fd.h"
#include "log.h"
#include "objects.h"
#include "random.h"
#include "sanitise.h"
#include "shm.h"
#include "compat.h"
#include "trinity.h"

static int bpf(int cmd, union bpf_attr *attr, unsigned int size)
{
	return syscall(__NR_bpf, cmd, attr, size);
}

static int bpf_create_map(enum bpf_map_type map_type, unsigned int key_size,
			unsigned int value_size, unsigned int max_entries, int map_flags)
{
	union bpf_attr attr = {
		.map_type    = map_type,
		.key_size    = key_size,
		.value_size  = value_size,
		.max_entries = max_entries,
		.map_flags   = map_flags,
	};

	return bpf(BPF_MAP_CREATE, &attr, sizeof(attr));
}


static void bpf_destructor(struct object *obj)
{
	close(obj->bpf_map_fd);
}

static int open_bpf_fds(void)
{
	struct objhead *head;
	int fd, key;
	long long value = 0;
	struct object *obj;
	struct rlimit r = {1 << 20, 1 << 20};

	setrlimit(RLIMIT_MEMLOCK, &r);

	head = get_objhead(OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	head->destroy = &bpf_destructor;

	fd = bpf_create_map(BPF_MAP_TYPE_HASH, sizeof(long long), sizeof(long long), 1024, 0);
	if (fd < 0)
		goto fail_hash;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf hash\n", fd);
fail_hash:

	fd = bpf_create_map(BPF_MAP_TYPE_ARRAY, sizeof(key), sizeof(value), 256, 0);
	if (fd < 0)
		goto fail_array;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf array\n", fd);
fail_array:

	fd = bpf_create_map(BPF_MAP_TYPE_PROG_ARRAY, sizeof(int), sizeof(int), 4, 0);
	if (fd < 0)
		goto fail_progarray;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf progarray\n", fd);
fail_progarray:

	fd = bpf_create_map(BPF_MAP_TYPE_PERF_EVENT_ARRAY, sizeof(int), sizeof(u32), 32, 0);
	if (fd < 0)
		goto fail_perf_event_array;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf perf event array\n", fd);
fail_perf_event_array:

	fd = bpf_create_map(BPF_MAP_TYPE_PERCPU_HASH, sizeof(u32), sizeof(u64) * PERF_MAX_STACK_DEPTH, 10000, 0);
	if (fd < 0)
		goto fail_percpu_hash;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf percpu hash\n", fd);
fail_percpu_hash:

	fd = bpf_create_map(BPF_MAP_TYPE_PERCPU_ARRAY, sizeof(u32), sizeof(u64), 100, 0);
	if (fd < 0)
		goto fail_percpu_array;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf percpu array\n", fd);
fail_percpu_array:

	fd = bpf_create_map(BPF_MAP_TYPE_STACK_TRACE, sizeof(u32), sizeof(u64), 100, 0);
	if (fd < 0)
		goto fail_stacktrace;
	obj = alloc_object();
	obj->bpf_map_fd = fd;
	add_object(obj, OBJ_GLOBAL, OBJ_FD_BPF_MAP);
	output(2, "fd[%d] = bpf stack trace\n", fd);
fail_stacktrace:

	//FIXME: right now, returning FALSE means "abort everything", not
	// "skip this provider", so on -ENOSYS, we have to still register.

	return TRUE;
}

static int get_rand_bpf_fd(void)
{
	struct object *obj;

	/* check if bpf unavailable/disabled. */
	if (objects_empty(OBJ_FD_BPF_MAP) == TRUE)
		return -1;

	obj = get_random_object(OBJ_FD_BPF_MAP, OBJ_GLOBAL);
	return obj->bpf_map_fd;
}

static const struct fd_provider bpf_fd_provider = {
	.name = "bpf",
	.enabled = TRUE,
	.open = &open_bpf_fds,
	.get = &get_rand_bpf_fd,
};

REG_FD_PROV(bpf_fd_provider);
#endif
