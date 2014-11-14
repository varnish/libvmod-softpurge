#include <stdlib.h>

#include "vrt.h"
#include "cache/cache.h"
#include "hash/hash_slinger.h"
#include "vcc_if.h"

void
vmod_softpurge(const struct vrt_ctx *ctx)
{
	struct objcore *oc, **ocp;
	struct objhead *oh;
	unsigned spc, nobj, n, ttl;
	struct object *o;
	unsigned u;
	double now;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (!ctx->req) {
		VSL(SLT_Debug, 0, "softpurge() is only valid in vcl_hit and vcl_miss.");
		return;
	}

	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);
	if (ctx->req->obj != NULL) {
		CHECK_OBJ_NOTNULL(ctx->req->obj, OBJECT_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->obj->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->obj->objcore->objhead, OBJHEAD_MAGIC);
		oh = ctx->req->obj->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_hit");
	} else if (ctx->req->objcore != NULL) {
		CHECK_OBJ_NOTNULL(ctx->req->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->objcore->objhead, OBJHEAD_MAGIC);
		oh = ctx->req->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_miss");
	} else {
		VSL(SLT_Debug, 0, "softpurge() is only valid in vcl_hit and vcl_miss.");
		return;
	}

	now = ctx->req->t_prev;
	CHECK_OBJ_NOTNULL(oh, OBJHEAD_MAGIC);

	spc = WS_Reserve(ctx->ws, 0);
	ocp = (void*)ctx->ws->f;

	Lck_Lock(&oh->mtx);
	assert(oh->refcnt > 0);
	nobj = 0;
	VTAILQ_FOREACH(oc, &oh->objcs, list) {
	    	CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		assert(oc->objhead == oh);
		if (oc->flags & OC_F_BUSY) {
			continue;
		}

		(void)oc_getobj(&ctx->req->wrk->stats, oc);

		xxxassert(spc >= sizeof *ocp);
		oc->refcnt++;
		spc -= sizeof *ocp;
		ocp[nobj++] = oc;
	}
	Lck_Unlock(&oh->mtx);

	for (n = 0; n < nobj; n++) {
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		o = oc_getobj(&ctx->req->wrk->stats, oc);
		if (o == NULL) {
			continue;
		}
		CHECK_OBJ_NOTNULL(o, OBJECT_MAGIC);

		/*
		   object really expires at o->exp.t_origin + o->exp.ttl, or at
		   an earlier point for this request if overridden in TTL.
		*/

		VSL(SLT_Debug, 0, "XX: object BEFORE.  ttl ends in %.3f, grace ends in %.3f for object %i",
		    (EXP_Ttl(ctx->req, o) - now),
		    (EXP_Ttl(ctx->req, o) + o->exp.grace - now), n);

		// Update the object's TTL so that it expires right now.
		if (o->exp.ttl > (now - o->exp.t_origin))
			EXP_Rearm(o, now, now - o->exp.t_origin, o->exp.grace, o->exp.keep);

		VSL(SLT_Debug, 0, "XX: object updated. ttl ends in %.3f, grace ends in %.3f for object %i",
		    (EXP_Ttl(ctx->req, o) - now),
		    (EXP_Ttl(ctx->req, o) + o->exp.grace - now), n);

		// decrease refcounter and clean up if the object has been removed.
		(void)HSH_DerefObj(&ctx->req->wrk->stats, &o);
	}
	WS_Release(ctx->ws, 0);
}
