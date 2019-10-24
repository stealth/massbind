massbind
========


Massbind is a small convenience lib to handle large range of ip6 subnet
addresses (/64 or smaller) and add (or delete) ip6 addresses to a given
interface. The ip6 addresses may be picked in a range or randomly.

Linux only.


Rationale
---------

In certain situations is useful to bind to more than one ip address at
client or server side. In order to bind to the address, it first has to be added to
the interface. At IPv4 times, you were lucky if you had a subnet with
some dozen of unused IP's you could assign by hand. With IPv6 you usually
get a /64 subnet from your provider/hoster. You can't add addresses by hand
any longer.
Using *massbind*, you can pick a dedicated ip6 address for each connection
within your program and throw it away after usage.

Note that *massbind* will actually only add the address to the given interface,
you need to `bind()` to it by yourself program-wise in order to bind to the
correct (src:srcport, dst:dstport) tuple.


Alternatives
------------

You could use ip6 privacy extensions, but this is not fine grained enough to
pick one address per connection. You may also use `IP_TRANSPARENT` sockets
to bind to non-exsiting addresses and setup your netfilter rules accordingly.

Server-side you can also add an entire subnet to your interface and add
routes to the *local* routing table:

```
# ip -6 addr add 2019::/64 dev eth0
# ip -6 route add local 2019::/64 dev lo
# ip -6 route list table local
[...]
```

Add a route on your border router to reach `2019::/64` via a real ip6 address of
that machine, and bind the server to `::`.
This however makes the entire subnet range accessible from outside. With *massbind*
you can be more picky on the addresses you use and dispose at runtime from within
your scanning-engine or whatever use-case you have. You don't need all the
configuration steps from above when using *massbind*.


Example
-------

Example code is in `mass.c`, simply type `make`. It's straight forward to
init and add/del the addresses at hand.

The lib will not filter special addresses. So you may get `...:ffff` or similar
when walking a range. You either filter these by yourself or write your code
to handle errors during `mb_add6()` gracefully.

The lib is using the routing sockets to add/del addresses directly. You do not
need to install *libnl* or similar. The `rtnetlink` API should be stable enough
to work among various Linux kernel versions 3.x, 4.x and 5.x.


```
# ./mass add ens3 2019::1 2019::7350 64
[...]
```

to test adding all IPv6 addresses from range `2019::1` to `2019::7350` (inclusive)
to ens3.


Efficiency
----------

Mass adding 1000's of addresses is time consuming after a while w/o deleting unused
addresses again. The Linux kernel handles addresses in double linked lists, without
hash buckets:


```
 971 static void
 972 ipv6_link_dev_addr(struct inet6_dev *idev, struct inet6_ifaddr *ifp)
 973 {
 974         struct list_head *p;
 975         int ifp_scope = ipv6_addr_src_scope(&ifp->addr);
 976
 977         /*
 978          * Each device address list is sorted in order of scope -
 979          * global before linklocal.
 980          */
 981         list_for_each(p, &idev->addr_list) {
 982                 struct inet6_ifaddr *ifa
 983                         = list_entry(p, struct inet6_ifaddr, if_list);
 984                 if (ifp_scope >= ipv6_addr_src_scope(&ifa->addr))
 985                         break;
 986         }
 987
 988         list_add_tail_rcu(&ifp->if_list, p);
 989 }
```

Although the 5.x kernel has code to also maintain addresses per device within
hashed lists (additional to the linked list). Walking the linked list makes the
handling of addresses slow after a while. Additionally, the kernel checks flags
(that we could use to speed up adding) that are impossible to set:


```
2953         ifp = ipv6_add_addr(idev, cfg, true, extack);
2954         if (!IS_ERR(ifp)) {
2955                 if (!(cfg->ifa_flags & IFA_F_NOPREFIXROUTE)) {
2956                         addrconf_prefix_route(&ifp->addr, ifp->prefix_len,
2957                                               ifp->rt_priority, dev, expires,
2958                                               flags, GFP_KERNEL);
2959                 }
2960
```

Here we would like to set `IFA_F_NOPREFIXROUTE` to save a call to `addrconf_prefix_route()`
but `IFA_F_NOPREFIXROUTE` is defined as `0x200` while the `ifa_flags` can only hold 8 bits!


```
  8 struct ifaddrmsg {
  9         __u8            ifa_family;
 10         __u8            ifa_prefixlen;  /* The prefix length            */
 11         __u8            ifa_flags;      /* Flags                        */
 12         __u8            ifa_scope;      /* Address scope                */
 13         __u32           ifa_index;      /* Link index                   */
 14 };
```

So, there is no way for *massbind* to speed up certain things. Please outrage
at your local kernel dealer and ask them to fix things. Until then, take care
to also have calls to `mb_del6()` in your looping engine if you need >10k of
disposable addresses.


References
----------

[apnic blog](https://blog.apnic.net/2016/09/14/binding-ipv6-subnet/)


