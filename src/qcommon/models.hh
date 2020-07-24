#pragma once

#include "qfiles.hh"
#include "generic_model.hh"
#include "qcommon.hh"

objModel_t * Model_LoadObj(char const * name);
std::shared_ptr<GenericModel const> Model_LoadObj2(char const * name);
