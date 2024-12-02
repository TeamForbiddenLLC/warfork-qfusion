#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "datasources/ui_blockedplayers_datasource.h"
#include "kernel/ui_syscalls.h"
#include <cstdint>

#define TABLE_NAME "blockedplayers"

namespace WSWUI
{
    BlockedPlayersDataSource::BlockedPlayersDataSource():Rml::DataSource( "blockedplayers" )
	{

	}
	
	void BlockedPlayersDataSource::GetRow( Rml::StringList &row, const Rml::String&, int row_index, const Rml::StringList& cols )
	{
		for( Rml::StringList::const_iterator it = cols.begin(); it != cols.end(); ++it )
		{
			uint64_t steamid;

			char name[MAX_NAME_BYTES+1] = {0};
			size_t name_len = sizeof name;
			trap::GetBlocklistItem(row_index, &steamid, name, &name_len);

			if(*it == "name") row.push_back(name);
			else if(*it == "cleanname") {
				row.push_back(COM_RemoveColorTokens(name));
			}
			else if(*it == "steamid") row.push_back(va("%llu", steamid) );
			else if(*it == "index") row.push_back(va("%i", row_index));
			else row.push_back("");
		}
	}

	int BlockedPlayersDataSource::GetNumRows(const Rml::String&)
	{
		size_t i = 0;
		for (;;i++) {
  		if (!trap::GetBlocklistItem(i, NULL, NULL, NULL))
    		break;
		}
		return i;
	}
}

