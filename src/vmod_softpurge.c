#include <stdlib.h>


#include "vrt.h"
#include "vrt_obj.h"
#include "vcl.h"
#include "cache/cache.h"
#include "hash/hash_slinger.h"

#include "vcc_if.h"

void
vmod_softpurge(const struct vrt_ctx *ctx)
{

	struct objcore *oc, **ocp;
	struct objhead *oh;
        unsigned spc, ospc, nobj, n;
	double now;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);

	if (!ctx->req) {
		VSL(SLT_Debug, 0, "softpurge() is only valid in"
		    "vcl_hit and vcl_miss.");
		return;
	}

	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);
	if (ctx->method != VCL_MET_HIT &&  ctx->method != VCL_MET_MISS) {
		VSL(SLT_Debug, 0, "softpurge() is only valid in"
		    " vcl_hit and vcl_miss.");
		return;
	}

	oh = ctx->req->objcore->objhead;
        CHECK_OBJ_NOTNULL(oh, OBJHEAD_MAGIC);
        ospc = WS_Reserve(ctx->ws, 0);
        assert(ospc >= sizeof *ocp);

	spc = ospc;
	nobj = 0;
	ocp = (void*)ctx->ws->f;
	Lck_Lock(&oh->mtx);
	assert(oh->refcnt > 0);
	now = ctx->req->t_prev;
	VTAILQ_FOREACH(oc, &oh->objcs, list) {
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		assert(oc->objhead == oh);
		if (oc->flags & OC_F_BUSY) {
			continue;
		}
		if (oc->exp_flags & OC_EF_DYING)
			continue;
		if (spc < sizeof *ocp) {
			break;
		}
		oc->refcnt++;
		spc -= sizeof *ocp;
		ocp[nobj++] = oc;
	}
	Lck_Unlock(&oh->mtx);

	for (n = 0; n < nobj; n++) {
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		VSL(SLT_Debug, 0, "XX: object BEFORE.  ttl ends in %.3f,"
		    " grace ends in %.3f for object %i",
		    (EXP_Ttl(ctx->req, &oc->exp) - now),
		    (EXP_Ttl(ctx->req, &oc->exp) + oc->exp.grace - now), n);
		EXP_Rearm(oc, now, 0, oc->exp.grace, oc->exp.keep);

		VSL(SLT_Debug, 0, "XX: object updated. ttl ends in %.3f,"
		    " grace ends in %.3f for object %i",
		    (EXP_Ttl(ctx->req, &oc->exp) - now),
		    (EXP_Ttl(ctx->req, &oc->exp) + (double)oc->exp.grace - now), n);
		(void)HSH_DerefObjCore(ctx->req->wrk, &oc);
	}
	WS_Release(ctx->ws, 0);


}
