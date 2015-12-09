This is a running log of changes to libvmod-softpurge.

libvmod-softpurge 1.0.0+4.0 (2015-12-09)
----------------------------------------

* Fix issue with correctly setting TTL to zero on Varnish 4.0.

* Versioning scheme changed to new standard.

* Packaging files has been retired.


Issues fixed:

*  4.0 docs are for 3.0 [Issue #4]

There were multiple internal non-tagged releases in the time period between 0.2
and 1.0.0+4.0. These should not be relied on.


libvmod-softpurge 0.2 (2014-09-15)
----------------------------------

First tagged release.

Soft purges is cache invalidation in Varnish that reduces TTL but keeps the
grace value of a resource. This makes it possible to serve stale content to
users if the backend is unavailable and fresh content can not be fetched.
