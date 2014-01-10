/*
 * SYSCALL_DEFINE0(getegid)
 */
#include "sanitise.h"

struct syscallentry syscall_getegid = {
	.name = "getegid",
	.num_args = 0,
	.rettype = RET_GID_T,
};
