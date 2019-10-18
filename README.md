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

And add a route on your border router to reach `2019::/64` via a real ip6 address of
that machine.
This however makes the entire subnet range accessible from outside. With *massbind*
you can be more picky on the addresses you use and dispose at runtime from within
your scanning-engine or whatever use-case you have.


Example
-------

Example code is in `mass.c`, simply type `make`. It's straight forward to
init and add/del the addresses at hand.

The lib will not filter special addresses. So you may get `...:ffff` or similar
when walking a range. You either filter these by yourself or write your code
to handle errors during `mb_add6()` gracefully.

The lib is using the routing sockets to add/del addresses directly. You do not
need to install *libnl* or similar. The `rtnetlink` API should be stable enough
to work among various Linux kernel versions 3.x, 4.x.


References
----------

[apnic blog](https://blog.apnic.net/2016/09/14/binding-ipv6-subnet/)


