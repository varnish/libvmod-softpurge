#include <stdlib.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"
#include "bin/varnishd/hash_slinger.h"
#include "vcc_if.h"


int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return (0);
}

void
vmod_softpurge(struct sess *sp)
{
	struct objcore *oc, **ocp;
	struct objhead *oh;

	unsigned spc, nobj, n, ttl;
	struct object *o;
	unsigned u;

	// we don't have the VCL_MET_HIT/MISS values here, and hardcoding
	// them isn't the best of ideas. If we have an obj we'er in hit. sp->cur_method later on.
	// if (sp->cur_method == VCL_MET_HIT) {
	if (sp->obj != NULL) {
    	CHECK_OBJ_NOTNULL(sp->obj, OBJECT_MAGIC);
		CHECK_OBJ_NOTNULL(sp->obj->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(sp->obj->objcore->objhead, OBJHEAD_MAGIC);
		oh = sp->obj->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_hit");
        // } else if (sp->cur_method == VCL_MET_MISS) {
	} else if (sp->objcore != NULL) {
		CHECK_OBJ_NOTNULL(sp->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(sp->objcore->objhead, OBJHEAD_MAGIC);
		oh = sp->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_miss");
	} else {
		VSL(SLT_Debug, 0, "softpurge() only valid in vcl_hit and vcl_miss");
		return;
	}

	VSL(SLT_Debug, 0, "found oh");
	CHECK_OBJ_NOTNULL(oh, OBJHEAD_MAGIC);

	spc = WS_Reserve(sp->wrk->ws, 0);
	ocp = (void*)sp->wrk->ws->f;

	Lck_Lock(&oh->mtx);
	assert(oh->refcnt > 0);
	nobj = 0;
	VTAILQ_FOREACH(oc, &oh->objcs, list) {
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		assert(oc->objhead == oh);
		if (oc->flags & OC_F_BUSY) {
			/*
                         * We cannot purge busy objects here, because their
                         * owners have special rights to them, and may nuke
                         * them without concern for the refcount, which by
                         * definition always must be one, so they don't check.
                         */
			continue;
		}

		(void)oc_getobj(sp->wrk, oc); /* XXX: still needed ? */

		xxxassert(spc >= sizeof *ocp);
		oc->refcnt++;
		spc -= sizeof *ocp;
		ocp[nobj++] = oc;
	}
	Lck_Unlock(&oh->mtx);

	for (n = 0; n < nobj; n++) {
		VSL(SLT_Debug, 0, "considering object %i", n);
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		o = oc_getobj(sp->wrk, oc);
		VSL(SLT_Debug, 0, "foo %i", n);
		if (o == NULL) {
			VSL(SLT_Debug, 0, "object %i refered to by objectcore is null", n);
			continue;
		}
		CHECK_OBJ_NOTNULL(o, OBJECT_MAGIC);

		o->exp.ttl = -1.;

		// reshuffle the lru tree since timers has changed.
		EXP_Rearm(o);

		VSL(SLT_Debug, 0, "object updated. ttl is %.3f, grace is %.3f for object %i", o->exp.ttl, o->exp.grace, n);
	}
	WS_Release(sp->wrk->ws, 0);
}
