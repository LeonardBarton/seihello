/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
/* Copyright (c) 2023, Oracle and/or its affiliates. */

#include <bpftune/bpftune.bpf.h>
#include "route_table_tuner.h"

struct dst_net {
	struct net *net;
	int entries;
};

BPF_MAP_DEF(dst_net_map, BPF_MAP_TYPE_HASH, __u64, struct dst_net, 65536);

SEC("kprobe/fib6_run_gc")
int BPF_KPROBE(bpftune_fib6_run_gc_entry, unsigned long expires,
					  struct net *net, bool force)
{
	struct dst_net dst_net = {};

	save_entry_data(dst_net_map, dst_net, net, net);
	return 0;
}

/* catch dst alloc failures and increase route table max size */
SEC("kretprobe/fib6_run_gc")
int BPF_KRETPROBE(bpftune_fib6_run_gc)
{
	struct dst_net *dst_net;
	struct net *net;
	int max_size;

	get_entry_struct(dst_net_map, dst_net);
	if (!dst_net || !dst_net->net)
		return 0;

	net = dst_net->net;

	max_size = BPF_CORE_READ(net, ipv6.sysctl.ip6_rt_max_size);
	__bpf_printk("gc: net: 0x%lx entries: %d max_size %d\n",
		     net, dst_net->entries, max_size);
	if (NEARLY_FULL(dst_net->entries, max_size)) {
		struct bpftune_event event = {};
		long old[3] = {};
		long new[3] = {};

		event.tuner_id = tuner_id;
		event.scenario_id = ROUTE_TABLE_FULL;

		old[0] = max_size;
		new[0] = BPFTUNE_GROW_BY_DELTA(max_size);
		if (send_net_sysctl_event(net, ROUTE_TABLE_FULL,
					  ROUTE_TABLE_IPV6_MAX_SIZE,
					  old, new, &event) < 0)
			return 0;
	}
	return 0;
}

/* count dst entries */
BPF_FENTRY(fib6_age, struct fib6_info *rt, void *arg)
{
	struct dst_net *dst_net;

	get_entry_struct(dst_net_map, dst_net);
	if (dst_net)
		dst_net->entries++;

	return 0;
}
