#include "cg_local.hh"

static void CG_DrawNavMap() {
	refEntity_t test {};
	test.rotation = Q_random() * 360;
	test.radius = 32;
	test.shaderTime = cg.time / 1000.0f;
	test.reType = RT_SPRITE;
	test.customShader = trap->R_RegisterShader("textures/flares/flare_blue");
	VectorSet(test.origin, Q_random() * 100, Q_random() * 100, Q_random() * 100);
	trap->R_AddRefEntityToScene(&test);
}

void CG_AddDebugEntities() {
	
	if (cg_debug_navMap.integer)
		CG_DrawNavMap();
	
}
