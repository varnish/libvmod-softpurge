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
	double now;

	now = TIM_real();
	// we don't have the VCL_MET_HIT/MISS values here, and hardcoding
	// them isn't the best of ideas. If we have an obj we're in hit. Use
	// sp->cur_method at some later stage.
	if (sp->obj != NULL) {
    	CHECK_OBJ_NOTNULL(sp->obj, OBJECT_MAGIC);
		CHECK_OBJ_NOTNULL(sp->obj->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(sp->obj->objcore->objhead, OBJHEAD_MAGIC);
		oh = sp->obj->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_hit");
	} else if (sp->objcore != NULL) {
		CHECK_OBJ_NOTNULL(sp->objcore, OBJCORE_MAGIC);
		CHECK_OBJ_NOTNULL(sp->objcore->objhead, OBJHEAD_MAGIC);
		oh = sp->objcore->objhead;
		VSL(SLT_Debug, 0, "Softpurge running inside vcl_miss");
	} else {
		VSL(SLT_Debug, 0, "softpurge() is only valid in vcl_hit and vcl_miss.");
		return;
	}

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
		oc = ocp[n];
		CHECK_OBJ_NOTNULL(oc, OBJCORE_MAGIC);
		o = oc_getobj(sp->wrk, oc);
		VSL(SLT_Debug, 0, "foo %i", n);
		if (o == NULL) {
			continue;
		}
		CHECK_OBJ_NOTNULL(o, OBJECT_MAGIC);

		/*
		   object really expires at o->exp.entered + o->exp.ttl, or at
		   an earlier point for this request if overridden in TTL.
		*/

		VSL(SLT_Debug, 0, "XX: object BEFORE.  ttl ends in %.3f, grace ends in %.3f for object %i",
				(EXP_Ttl(sp, o) - now),
				(EXP_Grace(sp, o) - now), n);

		// Update the object's TTL so that it expires right now.
		if (o->exp.ttl > (now - o->exp.entered))
			o->exp.ttl = now - o->exp.entered;

		VSL(SLT_Debug, 0, "XX: object updated. ttl ends in %.3f, grace ends in %.3f for object %i",
				(EXP_Ttl(sp, o) - now),
				(EXP_Grace(sp, o) - now), n);

		// Reshuffle the LRU tree since timers has changed.
		EXP_Rearm(o);

		// decrease refcounter and clean up if the object has been removed.
		(void)HSH_Deref(sp->wrk, NULL, &o);


	}
	WS_Release(sp->wrk->ws, 0);
}
