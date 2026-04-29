#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "datasources/ui_workshop_datasource.h"
#include "../../steamshim/src/mod_steam.h"
#include "../../steamshim/src/steamshim_types.h"

#include <sstream>

#define WORKSHOP_SOURCE   "workshop"
#define TABLE_INSTALLED   "installed"
#define COL_TITLE         "title"
#define COL_WORKSHOP_ID   "workshop_id"
#define COL_DISPLAY_NAME  "display_name"
#define COL_IS_LOCAL      "is_local"
#define COL_IS_INSTALLED  "is_installed"
#define COL_TAGS          "tags"
#define COL_PREVIEW_URL   "preview_url"
#define COL_VOTES_UP      "votes_up"
#define COL_VOTES_DOWN    "votes_down"
#define COL_SCORE         "score"
#define COL_VISIBILITY    "visibility"
#define COL_TIME_UPDATED  "time_updated"

namespace WSWUI
{

WorkshopDataSource::WorkshopDataSource() :
	Rocket::Controls::DataSource( WORKSHOP_SOURCE )
{
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_DETAIL, this, OnWorkshopDetail );
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_ITEM_INSTALLED, this, OnWorkshopItemInstalled );
	STEAMSHIM_subscribeEvent( EVT_WORKSHOP_ITEM_UNSUBSCRIBED, this, OnWorkshopItemUnsubscribed );
	UpdateMods();
}

WorkshopDataSource::~WorkshopDataSource()
{
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_DETAIL, OnWorkshopDetail );
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_ITEM_INSTALLED, OnWorkshopItemInstalled );
	STEAMSHIM_unsubscribeEvent( EVT_WORKSHOP_ITEM_UNSUBSCRIBED, OnWorkshopItemUnsubscribed );
}

void WorkshopDataSource::Refresh()
{
	Steam_RefreshWorkshopMods();
}

void WorkshopDataSource::UpdateMods()
{
	installedMods.clear();

	if( !STEAMSHIM_active() ) {
		NotifyRowChange( TABLE_INSTALLED );
		return;
	}

	const struct steam_workshop_mod_s *mods = Steam_GetWorkshopMods();
	size_t count = Steam_GetWorkshopModCount();

	for( size_t i = 0; i < count; i++ ) {
		// skip placeholder entries with no identity at all
		if( mods[i].workshop_id == 0 && !mods[i].path && !mods[i].local_path ) {
			continue;
		}

		InstalledMod mod;
		mod.title       = mods[i].title       ? mods[i].title       : ( mods[i].name ? mods[i].name : "" );
		mod.name        = mods[i].name         ? mods[i].name        : "";
		mod.tags        = mods[i].tags         ? mods[i].tags        : "";
		mod.preview_url = mods[i].preview_url  ? mods[i].preview_url : "";
		mod.is_local    = mods[i].is_local;
		mod.is_installed = mods[i].path != nullptr;
		mod.votes_up    = mods[i].votes_up;
		mod.votes_down  = mods[i].votes_down;
		mod.score       = mods[i].score;
		mod.visibility  = mods[i].visibility;
		mod.time_updated = mods[i].time_updated;

		std::ostringstream oss;
		oss << mods[i].workshop_id;
		mod.workshop_id = oss.str();

		installedMods.push_back( mod );
	}

	NotifyRowChange( TABLE_INSTALLED );
}

void WorkshopDataSource::GetRow( Rocket::Core::StringList &row, const Rocket::Core::String &table, int row_index, const Rocket::Core::StringList &cols )
{
	if( table != TABLE_INSTALLED )
		return;
	if( row_index < 0 || (size_t)row_index >= installedMods.size() )
		return;

	const InstalledMod &mod = installedMods[row_index];
	for( size_t i = 0; i < cols.size(); i++ ) {
		if( cols[i] == COL_TITLE )
			row.push_back( mod.title.c_str() );
		else if( cols[i] == COL_WORKSHOP_ID )
			row.push_back( mod.workshop_id.c_str() );
		else if( cols[i] == COL_DISPLAY_NAME )
			row.push_back( mod.is_local && !mod.name.empty() ? mod.name.c_str() : mod.title.c_str() );
		else if( cols[i] == COL_IS_LOCAL )
			row.push_back( mod.is_local ? "1" : "0" );
		else if( cols[i] == COL_IS_INSTALLED )
			row.push_back( mod.is_installed ? "1" : "0" );
		else if( cols[i] == COL_TAGS )
			row.push_back( mod.tags.c_str() );
		else if( cols[i] == COL_PREVIEW_URL )
			row.push_back( mod.preview_url.c_str() );
		else if( cols[i] == COL_VOTES_UP ) {
			std::ostringstream oss; oss << mod.votes_up;
			row.push_back( oss.str().c_str() );
		} else if( cols[i] == COL_VOTES_DOWN ) {
			std::ostringstream oss; oss << mod.votes_down;
			row.push_back( oss.str().c_str() );
		} else if( cols[i] == COL_SCORE ) {
			std::ostringstream oss; oss << mod.score;
			row.push_back( oss.str().c_str() );
		} else if( cols[i] == COL_VISIBILITY ) {
			std::ostringstream oss; oss << mod.visibility;
			row.push_back( oss.str().c_str() );
		} else if( cols[i] == COL_TIME_UPDATED ) {
			std::ostringstream oss; oss << mod.time_updated;
			row.push_back( oss.str().c_str() );
		} else
			row.push_back( "" );
	}
}

int WorkshopDataSource::GetNumRows( const Rocket::Core::String &table )
{
	if( table == TABLE_INSTALLED )
		return (int)installedMods.size();
	return 0;
}

void WorkshopDataSource::OnWorkshopDetail( void *self, struct steam_evt_pkt_s *pkt )
{
	static_cast<WorkshopDataSource *>( self )->UpdateMods();
}

void WorkshopDataSource::OnWorkshopItemInstalled( void *self, struct steam_evt_pkt_s *pkt )
{
	static_cast<WorkshopDataSource *>( self )->UpdateMods();
}

void WorkshopDataSource::OnWorkshopItemUnsubscribed( void *self, struct steam_evt_pkt_s *pkt )
{
	static_cast<WorkshopDataSource *>( self )->UpdateMods();
}

}
