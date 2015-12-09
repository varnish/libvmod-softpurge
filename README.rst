==============
vmod_softpurge
==============

.. image:: https://travis-ci.org/varnish/libvmod-softpurge.png
   :alt: Travis CI badge
   :target: https://travis-ci.org/varnish/libvmod-softpurge

--------------------
Softpurge in Varnish
--------------------

:Author: Lasse Karstensen
:Date: 2015-12-02
:Version: 1.0.0+release
:Manual section: 3

SYNOPSIS
========

import softpurge;

DESCRIPTION
===========

``Softpurge`` is cache invalidation in Varnish that reduces TTL but
keeps the grace value of a resource.

This makes it possible to serve stale content to users if the backend
is unavailable and fresh content can not be fetched.

FUNCTIONS
=========

softpurge
---------

Prototype
	softpurge()
Return value
	NONE
Description
	Performs a soft purge. Valid in vcl_hit and vcl_miss.
Example
::
	sub vcl_hit {
	    if (req.method == "PURGE") {
	        softpurge.softpurge();
	    }
	}

INSTALLATION
============

The source tree is based on autotools to configure the building, and
does also have the necessary bits in place to do functional unit tests
using the varnishtest tool.

Usage::

./configure

Make targets:

* make - builds the vmod
* make install - installs the vmod
* make check - runs the unit tests in ``src/tests/*.vtc``

In your VCL you could then use this vmod along the following lines::

    import softpurge;

    sub vcl_recv {
        if (req.method == "PURGE") {
            return (hash);
        }
    }

    sub vcl_backend_response {
        set beresp.grace = 10m;
    }

    sub vcl_hit {
        if (req.method == "PURGE") {
            softpurge.softpurge();
            return (synth(200, "Successful softpurge"));
        }
    }

    sub vcl_miss {
        if (req.method == "PURGE") {
            softpurge.softpurge();
            return (synth(200, "Successful softpurge"));
        }
    }

COPYRIGHT
=========

* Copyright (c) 2014-2015 Varnish Software Group
