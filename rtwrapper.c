#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <signal.h>
#include <libnetlink.h>

#include <dlfcn.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <linux/in_route.h>
#include <linux/sockios.h>

#include "ll_map.h"


static int (*real_ioctl)(int, unsigned long, void*);

static struct rtnl_handle rth = { .fd = -1 };

void init() __attribute__((constructor));

void dispose() __attribute__((destructor));

__u32 sockaddr2rta(struct sockaddr * sa) {
	return ((struct sockaddr_in *) sa)->sin_addr.s_addr;
}

__u8 sockaddr2len(struct sockaddr * sa) {
    struct sockaddr_in * sa_in = (struct sockaddr_in *) sa;
    __u32 addr = sa_in->sin_addr.s_addr;
    __u32 i = 1;
    __u8 c = 0;
    int maxlen = (sizeof(addr) * 8);
    while ((addr & i) && c < maxlen) {
        c ++;
        i <<= 1;
    }
    return c;
}

int route_modify(int cmd, int flags, struct rtentry *rte) {

    struct {
        struct nlmsghdr     n;
        struct rtmsg        r;
        char            buf[1024];
    } req;
    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | flags;
    req.n.nlmsg_type = cmd;
    req.r.rtm_family = rte->rt_dst.sa_family;
    req.r.rtm_table = 128;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_scope = RT_SCOPE_UNIVERSE;
    req.r.rtm_type = RTN_UNICAST;
	req.r.rtm_dst_len = sockaddr2len(&(rte->rt_genmask));

    int dev = ll_name_to_index(rte->rt_dev);

	addattr32(&req.n, sizeof(req), RTA_DST, sockaddr2rta(&(rte->rt_dst)));
    addattr32(&req.n, sizeof(req), RTA_GATEWAY, sockaddr2rta(&(rte->rt_gateway)));
    addattr32(&req.n, sizeof(req), RTA_PRIORITY, rte->rt_metric);

    if (dev != 0)
        addattr32(&req.n, sizeof(req), RTA_OIF, dev);

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL) < 0)
		puts("Error talking to netlink");

    return 0;
}

int ioctl(int fd, unsigned long request, void *arg) {
	switch (request) {
	case SIOCADDRT:
		return route_modify(RTM_NEWROUTE, NLM_F_CREATE|NLM_F_EXCL, arg);
	case SIOCDELRT:
		return route_modify(RTM_DELROUTE, 0, arg);
	default:
		return real_ioctl(fd, request, arg);
	}
}

void init() {

	// get original ioctl function ptr
	real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (real_ioctl == NULL) {
        puts("failed");
		exit(3);
    }

	// open netlink connection
	if (rtnl_open(&rth, 0) < 0 ) {
		puts("Unable to connect to netlink socket");
		exit(1);
	}

}

void dispose() {

	// TODO flush 128 routing table before close

	rtnl_close(&rth);
}

