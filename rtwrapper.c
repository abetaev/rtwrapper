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


struct rtnl_handle rth = { .fd = 0 };

static int (*real_ioctl)(int, unsigned long, void*);

void init() __attribute__((constructor));

void dispose() __attribute__((destructor));

struct rta_addr {
	__u32 addr[8];
	int len;
};

void sockaddr2rta(struct rta_addr * rta, struct sockaddr * sa) {
	struct sockaddr_in * sa_in = (struct sockaddr_in *) sa;
	rta->addr[0] = sa_in->sin_addr.s_addr;
	rta->len = 4;
	for (int i = 0; i < rta->len; i ++) {
		printf("%i ", ((char*) (&rta->addr)) + i);
		puts("");
	}
}

int route_modify(int cmd, int flags, struct rtentry *rte) {
	printf("hello");


	struct rtnl_handle rth;

    struct {
        struct nlmsghdr     n;
        struct rtmsg        r;
        char            buf[1024];
    } req;
    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | flags;
    req.n.nlmsg_type = cmd;
    //req.r.rtm_family = AF_INET;
    req.r.rtm_family = rte->rt_dst.sa_family;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_scope = RT_SCOPE_UNIVERSE;
    req.r.rtm_type = RTN_UNICAST;
	req.r.rtm_table = RT_TABLE_MAIN;
	req.r.rtm_dst_len = 32;

	struct rta_addr rta_dst_addr;
	struct rta_addr rta_gw_addr;

	sockaddr2rta(&rta_dst_addr, &(rte->rt_dst));
	sockaddr2rta(&rta_gw_addr, &(rte->rt_gateway));

	addattr_l(&req.n, sizeof(req), RTA_DST, rta_dst_addr.addr, rta_dst_addr.len);
    addattr_l(&req.n, sizeof(req), RTA_GATEWAY, rta_gw_addr.addr, rta_gw_addr.len);

    printf("family: %i\n", req.r.rtm_family);
    printf("scope: %i\n", req.r.rtm_scope);
    printf("table: %i\n", req.r.rtm_table);
    printf("protocol: %i\n", req.r.rtm_protocol);
    printf("type: %i\n", req.r.rtm_type);
    printf("dlen: %i\n", req.r.rtm_dst_len);
    printf("slen: %i\n", req.r.rtm_src_len);
    printf("tos: %i\n", req.r.rtm_tos);
    printf("flags: %i\n", req.r.rtm_flags);
	puts("---");
    printf("len: %i\n", req.n.nlmsg_len);
    printf("type: %i\n", req.n.nlmsg_type);
    printf("flags: %i\n", req.n.nlmsg_flags);
    printf("pid: %i\n", req.n.nlmsg_pid);
    printf("seq: %i\n", req.n.nlmsg_seq);

    if (rtnl_talk(&rth, &req.n, 0, 0, NULL) < 0)
        exit(2);


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
	// open connection to netlink socket
	if (rtnl_open(&rth, 0) < 0 ) {
		puts("Unable to connect to netlink socket");
		exit(1);
	}

	// get original ioctl function ptr
	real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (real_ioctl == NULL) {
        puts("failed");
		exit(3);
    }

}

void dispose() {
	rtnl_close(&rth);
}

int main(int argc, char * argv[]) {
	init();
	//route_modify(RTM_NEWROUTE, NLM_F_CREATE|NLM_F_EXCL, );
	dispose();

	return 0;
}
