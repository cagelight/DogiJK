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
	
}
