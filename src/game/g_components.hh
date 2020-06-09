#include "g_local.hh"

#include <unordered_set>

struct GEntComponentMultiActivator : public GEntityComponent {
	std::unordered_set<gentity_t *> activators;
};
