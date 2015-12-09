This is a running log of changes to libvmod-softpurge.

libvmod-softpurge 1.0.0+release (unreleased)
--------------------------------------------

This release for Varnish Cache 4.1 builds on previous work in the 4.0 branch.

The +release versioning means this code originates from the master
branch in git. The distinction is needed due to overlapping version
numbers in other branches.

In the future this branch will follow the latest Varnish Cache version.

There were multiple internal non-tagged releases in the time period between 0.2
and 1.0.0+release. These should not be relied on.


libvmod-softpurge 0.2 (2014-09-15)
----------------------------------

First tagged release.

Soft purges is cache invalidation in Varnish that reduces TTL but keeps the
grace value of a resource. This makes it possible to serve stale content to
users if the backend is unavailable and fresh content can not be fetched.
