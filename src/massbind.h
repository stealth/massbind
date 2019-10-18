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

#ifndef massbind_h
#define massbind_h

#include <stdint.h>
#include <arpa/inet.h>
#include <endian.h>
#include <byteswap.h>


#ifdef __cplusplus
extern "C" {
#endif


enum {

	MB_ADDR_RAND	= 10,
	MB_ADDR_RANGE	= 11,

	MB_ADDR_ADD	= 12,
	MB_ADDR_DEL	= 13

};


typedef struct {
	char dev[16];
	struct in6_addr ip6_start, ip6_end;
	uint64_t range_idx, low64_start, low64_end;
	int rt_fd, rfd, if_idx;
	unsigned char prefix;
} mb_ctx;


int mb_init6(mb_ctx *, const char *, const char *, const char *, unsigned char);

int mb_next6_range(mb_ctx *, struct in6_addr *);

int mb_next6_rand(mb_ctx *, struct in6_addr *);

int mb_add6(mb_ctx *, const struct in6_addr *);

int mb_del6(mb_ctx *, const struct in6_addr *);

void mb_finish(mb_ctx *);


# if __BYTE_ORDER == __LITTLE_ENDIAN
#define mb_be64toh(x) __bswap_64(x)
#define mb_htobe64(x) __bswap_64(x)
#else
#define mb_be64toh(x) (x)
#define mb_htobe64(x) (x)
#endif

#ifdef __cplusplus
}
#endif

#endif


