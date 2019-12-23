/*
 * This file is part of massbind.
 *
 * (C) 2019 by Sebastian Krahmer,
 *             sebastian [dot] krahmer [at] gmail [dot] com
 *
 * massbind is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * massbind is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with massbind.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include "massbind.h"


int mb_init6(mb_ctx *ctx, const char *dev, const char *start, const char *end, unsigned char prefix)
{
	if (!ctx || !dev || !start || !end)
		return -1;

	if (strlen(dev) >= sizeof(ctx->dev))
		return -1;

	memset(ctx, 0, sizeof(*ctx));

	// yolo
	strcpy(ctx->dev, dev);

	if (inet_pton(AF_INET6, start, &ctx->ip6_start) != 1)
		return -1;
	if (inet_pton(AF_INET6, end, &ctx->ip6_end) != 1)
		return -1;

	// Must at least be /64
	if (memcmp(&ctx->ip6_start, &ctx->ip6_end, 8) != 0)
		return -1;

	memcpy(&ctx->low64_start, (char *)&ctx->ip6_start + 8, 8);
	memcpy(&ctx->low64_end, (char *)&ctx->ip6_end + 8, 8);

	ctx->low64_start = mb_be64toh(ctx->low64_start);
	ctx->low64_end = mb_be64toh(ctx->low64_end);

	if (ctx->low64_start > ctx->low64_end)
		return -1;

	struct {
		struct nlmsghdr nh;
		struct ifinfomsg ifi;
		char attrbuf[256];
	} req, reply;

	struct rtattr *rta;

	if ((ctx->rfd = open("/dev/urandom", O_RDONLY)) < 0)
		return -1;

	if ((ctx->rt_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
		close(ctx->rfd);
		return -1;
	}

	memset(&req, 0, sizeof(req));

	req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nh.nlmsg_flags = NLM_F_REQUEST;
	req.nh.nlmsg_type = RTM_GETLINK;
	req.ifi.ifi_family = AF_UNSPEC;

	rta = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.nh.nlmsg_len));
	rta->rta_type = IFLA_IFNAME;
	rta->rta_len = RTA_LENGTH(sizeof(ctx->dev));
	memcpy(RTA_DATA(rta), ctx->dev, sizeof(ctx->dev));

	req.nh.nlmsg_len = NLMSG_ALIGN(req.nh.nlmsg_len) + RTA_LENGTH(sizeof(ctx->dev));

	if (send(ctx->rt_fd, &req, req.nh.nlmsg_len, 0) < 0) {
		close(ctx->rt_fd);
		close(ctx->rfd);
		return -1;
	}
	if (recv(ctx->rt_fd, &reply, sizeof(reply), 0) < sizeof(reply.nh)) {
		close(ctx->rt_fd);
		close(ctx->rfd);
		return -1;
	}

	if (reply.nh.nlmsg_type == NLMSG_ERROR) {
		close(ctx->rt_fd);
		close(ctx->rfd);
		return -1;
	}

	ctx->if_idx = reply.ifi.ifi_index;
	ctx->prefix = prefix;

	return 0;
}


void mb_finish(mb_ctx *ctx)
{
	if (!ctx)
		return;

	close(ctx->rt_fd);
	close(ctx->rfd);

	memset(ctx, 0, sizeof(*ctx));
}


int mb_next6_range(mb_ctx *ctx, struct in6_addr *ip6)
{
	if (!ctx || !ip6)
		return -1;

	if (ctx->low64_start + ctx->range_idx > ctx->low64_end)
		return -1;

	memcpy(ip6, &ctx->ip6_start, 8);
	uint64_t tmp = mb_htobe64(ctx->low64_start + ctx->range_idx);
	memcpy((char *)ip6 + 8, &tmp, sizeof(tmp));
	++ctx->range_idx;

	return 0;
}


int mb_next6_rand(mb_ctx *ctx, struct in6_addr *ip6)
{
	if (!ctx || !ip6)
		return -1;

	uint64_t rand_max = ctx->low64_end - ctx->low64_start + 1, r = 0;

	// urandom
	read(ctx->rfd, &r, sizeof(r));

	r %= rand_max;

	memcpy(ip6, &ctx->ip6_start, 8);
	r = mb_htobe64(ctx->low64_start + r);
	memcpy((char *)ip6 + 8, &r, sizeof(r));

	return 0;
}


static int mb_add_del6(mb_ctx *ctx, const struct in6_addr *ip6, int what)
{
	struct {
		struct nlmsghdr nh;
		struct ifaddrmsg ifa;
		char attrbuf[256];
	} req;

	struct {
		struct nlmsghdr nh;
		struct nlmsgerr err;
	} reply;

	struct rtattr *rta;

	memset(&req, 0, sizeof(req));

	req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.nh.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	req.nh.nlmsg_type = (what == MB_ADDR_ADD) ? RTM_NEWADDR : RTM_DELADDR;
	req.ifa.ifa_family = AF_INET6;
	req.ifa.ifa_flags = 0;	// replaced by IFA_FLAGS rta
	req.ifa.ifa_index = ctx->if_idx;
	req.ifa.ifa_prefixlen = ctx->prefix;

	rta = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.nh.nlmsg_len));
	rta->rta_type = IFA_ADDRESS;
	rta->rta_len = RTA_LENGTH(sizeof(struct in6_addr));
	memcpy(RTA_DATA(rta), ip6, sizeof(struct in6_addr));

	req.nh.nlmsg_len += RTA_LENGTH(sizeof(struct in6_addr));

	rta = (struct rtattr *)(((char *) rta) + RTA_LENGTH(sizeof(struct in6_addr)));
	rta->rta_type = IFA_FLAGS;
	rta->rta_len = RTA_LENGTH(sizeof(uint32_t));
	uint32_t flags = IFA_F_NODAD|IFA_F_NOPREFIXROUTE;
	memcpy(RTA_DATA(rta), &flags, sizeof(flags));

	req.nh.nlmsg_len += RTA_LENGTH(sizeof(uint32_t));

	if (send(ctx->rt_fd, &req, req.nh.nlmsg_len, 0) < 0)
		return -1;
	if (recv(ctx->rt_fd, &reply, sizeof(reply), 0) < sizeof(reply))
		return -1;

	// TODO: Maybe adding sequence field and match with reply?

	if (reply.nh.nlmsg_type == NLMSG_ERROR && reply.err.error != 0)
		return -1;

	return 0;
}


int mb_add6(mb_ctx *ctx, const struct in6_addr *ip6)
{
	return mb_add_del6(ctx, ip6, MB_ADDR_ADD);
}



int mb_del6(mb_ctx *ctx, const struct in6_addr *ip6)
{
	return mb_add_del6(ctx, ip6, MB_ADDR_DEL);
}

