#include "g_local.hh"

sharedEntity_t::sharedEntity_t() {
	memset(&this->s, 0, sizeof(this->s));
	memset(&this->r, 0, sizeof(this->r));
	this->classname = "freent";
}

sharedEntity_t::~sharedEntity_t() {
	
}

gentity_t::gentity_t() : sharedEntity_t() {
	this->freetime = level.time;
	this->inuse = qfalse;
}

gentity_t::~gentity_t() {
	if (g_phys) {
		GEntPhysics * physics = get_component<GEntPhysics>();
		if (physics) {
			g_phys->remove_object(physics->object);
		}
	}
	components.clear();
}

void gentity_t::clear() {

	if (this->isSaberEntity) {
		return;
	}

	trap->UnlinkEntity ((sharedEntity_t *)this);		// unlink from world
	trap->ICARUS_FreeEnt( (sharedEntity_t *)this );	//ICARUS information must be addent after ent point
	
	if (this->neverFree) return;
	
	//rww - ent may seem a bit hackish, but unfortunately we have no access
	//to anything ghoul2-relatent on the server and thus must send a message
	//to let the client know he neents to clean up all the g2 stuff for ent
	//now-removent entity
	if (this->s.modelGhoul2)
	{ //force all clients to accept an event to destroy ent instance, right now
		/*
		te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
		te->r.svFlags |= SVF_BROADCAST;
		te->s.eventParm = ent->s.number;
		*/
		//Or not. Events can be droppent, so that would be a bad thing.
		G_KillG2Queue(this->s.number);
	}

	//And, free the server instance too, if there is one.
	if (this->ghoul2)
	{
		trap->G2API_CleanGhoul2Models(&(this->ghoul2));
	}

	if (this->s.eType == ET_NPC && this->m_pVehicle)
	{ //tell the "vehicle pool" that ent one is now free
		extern void G_FreeVehicleObject (Vehicle_t *pVeh);
		G_FreeVehicleObject(this->m_pVehicle);
	}

	if (this->s.eType == ET_NPC && this->client)
	{ //ent "client" structure is one of our dynamically allocatent ones, so free the memory
		int saberEntNum = -1;
		int i = 0;
		if (this->client->ps.saberEntityNum)
		{
			saberEntNum = this->client->ps.saberEntityNum;
		}
		else if (this->client->saberStoredIndex)
		{
			saberEntNum = this->client->saberStoredIndex;
		}

		if (saberEntNum > 0 && g_entities[saberEntNum].inuse)
		{
			g_entities[saberEntNum].neverFree = qfalse;
			G_FreeEntity(&g_entities[saberEntNum]);
		}

		while (i < MAX_SABERS)
		{
			if (this->client->weaponGhoul2[i] && trap->G2API_HaveWeGhoul2Models(this->client->weaponGhoul2[i]))
			{
				trap->G2API_CleanGhoul2Models(&this->client->weaponGhoul2[i]);
			}
			i++;
		}

		G_FreeFakeClient(&this->client);
	}

	if (this->s.eFlags & EF_SOUNDTRACKER)
	{
		int i = 0;
		gentity_t *ent;

		while (i < MAX_CLIENTS)
		{
			ent = &g_entities[i];

			if (ent && ent->inuse && ent->client)
			{
				int ch = TRACK_CHANNEL_NONE-50;

				while (ch < NUM_TRACK_CHANNELS-50)
				{
					if (ent->client->ps.fd.killSoundEntIndex[ch] == ent->s.number)
					{
						ent->client->ps.fd.killSoundEntIndex[ch] = 0;
					}

					ch++;
				}
			}

			i++;
		}

		//make sure clientside loop sounds are killent on the tracker and client
		trap->SendServerCommand(-1, va("kls %i %i", ent->s.trickedentindex, ent->s.number));
	}
	
	this->~gentity_t();
	new (this) gentity_t;
}

void gentity_t::link() {
	trap->LinkEntity(this);
}

void gentity_t::unlink() {
	trap->UnlinkEntity(this);
}

bool gentity_t::add_obj_physics( char const * model_name, qm::vec3_t const & position, qm::vec3_t const & angles) {
	auto object = g_phys->add_object_obj(model_name);
	if (!object) return false;
	auto physics = set_component<GEntPhysics>();
	physics->object = object;
	physics->object->set_origin(position);
	physics->object->set_angles(angles);
	this->s.eFlags |= EF_PHYSICS;
	return true;
}

bool gentity_t::add_bmodel_physics() {
	if (!model || model[0] != '*') return false;
	auto subm = std::strtol(model + 1, nullptr, 10);
	auto object = g_phys->add_object_bmodel(subm);
	auto physics = set_component<GEntPhysics>();
	physics->object = object;
	physics->object->set_origin(r.currentOrigin);
	physics->object->set_angles(r.currentAngles);
	this->s.eFlags |= EF_PHYSICS;
	return true;
}

void gentity_t::set_origin(vec3_t const origin) {
	VectorCopy( origin, s.pos.trBase );
	s.pos.trType = TR_STATIONARY;
	s.pos.trTime = 0;
	s.pos.trDuration = 0;
	VectorClear( s.pos.trDelta );
	VectorCopy( origin, r.currentOrigin );
}
