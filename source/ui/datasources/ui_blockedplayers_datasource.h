#pragma once
#include <Rocket/Controls/DataSource.h>

namespace WSWUI
{

class BlockedPlayersDataSource :
	public Rml::DataSource
{
public:
	BlockedPlayersDataSource( );

	virtual void GetRow( Rml::StringList& row, const Rml::String& table, int row_index, const Rml::StringList& columns );
	virtual int GetNumRows( const Rml::String& table );

};

}
