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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "massbind.h"


void usage(const char *p)
{
	fprintf(stderr, "Usage: %s <add|del> <intf> <start ip6> <end ip6> <prefixlen>\n\n", p);
	exit(1);
}


int main(int argc, char **argv)
{

	printf("\nmassbind utility for the massbind lib\n\n"
	       "(C) 2019 Sebastian Krahmer -- https://github.com/stealth/massbind\n\n");

	if (argc != 6)
		usage(argv[0]);

	const char *cmd = argv[1], *dev = argv[2], *s_ip6 = argv[3], *e_ip6 = argv[4];
	unsigned char plen = (char)atoi(argv[5]);

	if (strcmp(cmd, "add") != 0 && strcmp(cmd, "del") != 0)
		usage(argv[0]);

	mb_ctx ctx;

	if (mb_init6(&ctx, dev, s_ip6, e_ip6, plen) < 0) {
		fprintf(stderr, "Error calling mb_init6.\n");
		exit(1);
	}

	int r = 0;
	struct in6_addr a;

	for (;;) {

		char ip_str[128] = {0};
		memset(&a, 0, sizeof(a));

		if (mb_next6_range(&ctx, &a) < 0)
			break;

		if (strcmp(cmd, "add") == 0)
			r = mb_add6(&ctx, &a);
		else
			r = mb_del6(&ctx, &a);


		inet_ntop(AF_INET6, &a, ip_str, sizeof(ip_str) - 1);
		printf("cmd=%s address=%s result=%d\n", cmd, ip_str, r);
	}

	mb_finish(&ctx);

	return 0;
}

