#include "vrt.h"
#include "vcl.h"
#include "cache/cache.h"
#include "hash/hash_slinger.h"

#include "vcc_if.h"

void
vmod_softpurge(const struct vrt_ctx *ctx)
{
	struct objcore *oc, **ocp;
	struct objhead *oh;
	unsigned spc, nobj, n;
	struct object *o;
	double now;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	if (ctx->method != VCL_MET_HIT && ctx->method != VCL_MET_MISS) {
		VSLb(ctx->vsl, SLT_VCL_Error, "softpurge() is only "
		    "available in vcl_hit{} and vcl_miss{}");
		return;
	}

	CHECK_OBJ_NOTNULL(ctx->req, REQ_MAGIC);
	if (ctx->req->obj != NULL) {
		CHECK_OBJ_NOTNULL(ctx->req->obj, OBJECT_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->obj->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->obj->objcore->objhead, OBJHEAD_MAGIC);
		oh = ctx->req->obj->objcore->objhead;
	} else if (ctx->req->objcore != NULL) {
		CHECK_OBJ_NOTNULL(ctx->req->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(ctx->req->objcore->objhead, OBJHEAD_MAGIC);
		oh = ctx->req->objcore->objhead;
	}

	CHECK_OBJ_NOTNULL(oh, OBJHEAD_MAGIC);
	spc = WS_Reserve(ctx->ws, 0);
	assert(spc >= sizeof *ocp);

	nobj = 0;
	ocp = (void*)ctx->ws->f;
	Lck_Lock(&oh->mtx);
	assert(oh->refcnt > 0);
	now = ctx->req->t_prev;
	VTAILQ_FOREACH(oc, &oh->objcs, list) {
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		assert(oc->objhead == oh);
		if (oc->flags & OC_F_BUSY)
			continue;
		(void)oc_getobj(&ctx->req->wrk->stats, oc);
		if (spc < sizeof *ocp)
			break;
		oc->refcnt++;
		spc -= sizeof *ocp;
		ocp[nobj++] = oc;
	}
	Lck_Unlock(&oh->mtx);

	for (n = 0; n < nobj; n++) {
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		o = oc_getobj(&ctx->req->wrk->stats, oc);
		if (o == NULL)
			continue;
		CHECK_OBJ_NOTNULL(o, OBJECT_MAGIC);
		if (o->exp.ttl > (now - o->exp.t_origin))
			EXP_Rearm(o, now, 0, o->exp.grace, o->exp.keep);
		(void)HSH_DerefObj(&ctx->req->wrk->stats, &o);
	}
	WS_Release(ctx->ws, 0);
}
