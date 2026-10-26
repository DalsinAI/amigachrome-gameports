/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_SESSION_DATA_H
#define ALPHA_LOCAL_SESSION_DATA_H
#include "local_production_session.h"
#include <stdlib.h>
namespace alpha {
struct LocalAccountingTables;
struct LocalProductionSession::Data {
 OriginalWorld world;
 Rules technology;
 FacilityDefinitions facilities;
 UnitDefinitions* units=nullptr;
 LocalAccountingTables* accounting=nullptr;
 ~Data(){free(units);free(accounting);}
 BasicRules rules;
 WorldRules world_rules;
 Variant profile;
 bool expansion_enabled;
 int32_t local_human;
};
}
#endif
