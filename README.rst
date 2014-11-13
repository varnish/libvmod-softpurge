==============
vmod_softpurge
==============

----------------------
Softpurge in Varnish
----------------------

:Author: Lasse Karstensen
:Date: 2012-08-21
:Version: 1.0
:Manual section: 1

SYNOPSIS
========

import softpurge;

DESCRIPTION
===========

``Softpurge`` is cache invalidation in Varnish that reduces TTL but keeps the grace
value of a resource.

This makes it possible to serve purged content to users if
a backend is unavailable and fresh content can not be fetched.


FUNCTIONS
=========

softpurge
---------

Prototype
        ::

                softpurge()
Return value
	NULL

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

 ./configure VARNISHSRC=DIR [VMODDIR=DIR]

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``


In your VCL you could then use this vmod along the following lines::

        import softpurge;

        sub vcl_recv {
            	if (req.method == "PURGE") { return(hash); }
	}
        sub vcl_fetch { set beresp.grace = 10m; }

        sub vcl_hit {
                if (req.method == "PURGE) {
			softpurge.softpurge();
			return (synth(200, "Successful softpurge"));
		}
        }
        sub vcl_miss {
                if (req.method == "PURGE) {
			softpurge.softpurge();
			return (synth(200, "Successful softpurge"));
		}
        }

COPYRIGHT
=========

* Copyright (c) 2014 Varnish Software
