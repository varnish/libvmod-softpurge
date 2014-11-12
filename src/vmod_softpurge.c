#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"
#include "hash/hash_slinger.h"
#include "vtim.h"
#include "vcc_if.h"

void
vmod_softpurge(const struct vrt_ctx *vrt)
{
	struct objcore *oc, **ocp;
	struct objhead *oh;

	unsigned spc, nobj, n, ttl;
	struct object *o;
	unsigned u;
	double now;

	now = VTIM_real();

	if (vrt->req->obj != NULL) {
	    	CHECK_OBJ_NOTNULL(vrt->req->obj, OBJECT_MAGIC);
	    	CHECK_OBJ_NOTNULL(vrt->req->obj->objcore, OBJCORE_MAGIC);
	    	CHECK_OBJ_NOTNULL(vrt->req->obj->objcore->objhead, OBJHEAD_MAGIC);
		oh = vrt->req->obj->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_hit");
	} else if (vrt->req->objcore != NULL) {
	    	CHECK_OBJ_NOTNULL(vrt->req->objcore, OBJCORE_MAGIC);
	    	CHECK_OBJ_NOTNULL(vrt->req->objcore->objhead, OBJHEAD_MAGIC);
		oh = vrt->req->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_miss");
	} else {
		VSL(SLT_Debug, 0, "softpurge() is only valid in vcl_hit and vcl_miss.");
		return;
	}

		CHECK_OBJ_NOTNULL(oh, OBJHEAD_MAGIC);

	spc = WS_Reserve(vrt->ws, 0);
	ocp = (void*)vrt->ws->f;

	Lck_Lock(&oh->mtx);
		assert(oh->refcnt > 0);
	nobj = 0;
	VTAILQ_FOREACH(oc, &oh->objcs, list) {
	    	CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		assert(oc->objhead == oh);
		if (oc->flags & OC_F_BUSY) {
			continue;
		}

		//(void)oc_getobj(&vrt->req->wrk->stats, oc); /* XXX: still needed ? I think it's not needed anymore*/

		xxxassert(spc >= sizeof *ocp);
		oc->refcnt++;
		spc -= sizeof *ocp;
		ocp[nobj++] = oc;
	}
	Lck_Unlock(&oh->mtx);

	for (n = 0; n < nobj; n++) {
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		o = oc_getobj(&vrt->req->wrk->stats, oc);
		if (o == NULL) {
			continue;
		}
			CHECK_OBJ_NOTNULL(o, OBJECT_MAGIC);

		/*
		   object really expires at o->exp.t_origin + o->exp.ttl, or at
		   an earlier point for this request if overridden in TTL.
		*/

		VSL(SLT_Debug, 0, "XX: object BEFORE.  ttl ends in %.3f, grace ends in %.3f for object %i",
				(EXP_Ttl(vrt->req, o) - now),
				(EXP_Ttl(vrt->req, o) + o->exp.grace - now), n);

		// Update the object's TTL so that it expires right now.
		if (o->exp.ttl > (now - o->exp.t_origin))
		    EXP_Rearm(o, now, now - o->exp.t_origin, o->exp.grace, o->exp.keep);

		VSL(SLT_Debug, 0, "XX: object updated. ttl ends in %.3f, grace ends in %.3f for object %i",
				(EXP_Ttl(vrt->req, o) - now),
                                (EXP_Ttl(vrt->req, o) + o->exp.grace - now), n);

		// decrease refcounter and clean up if the object has been removed.
		(void)HSH_DerefObj(&vrt->req->wrk->stats, &o);
	}
	WS_Release(vrt->ws, 0);
}
