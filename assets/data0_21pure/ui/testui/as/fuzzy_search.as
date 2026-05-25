String fuzzySearch_savedQuery = "";

bool fuzzySearch_contains( const String &in text, const String &in query )
{
	if( query.empty() ) {
		return true;
	}
	if( text.empty() ) {
		return false;
	}
	String lowerText = text.tolower();
	String lowerQuery = query.tolower();
	uint pos = lowerText.locate( lowerQuery, 0 );
	return pos < lowerText.length();
}

bool fuzzySearch_match( const String &in text, const String &in query )
{
	if( query.empty() ) {
		return true;
	}
	if( text.empty() ) {
		return false;
	}
	array<String @> @terms = StringUtils::Split( query.tolower(), " " );
	String lowerText = text.tolower();
	for( uint i = 0; i < terms.length(); i++ ) {
		if( terms[i].empty() ) {
			continue;
		}
		uint pos = lowerText.locate( terms[i], 0 );
		if( pos >= lowerText.length() ) {
			return false;
		}
	}
	return true;
}

String fuzzySearch_buildText( DataSource @data, const String &in table, int rowIndex, array<String> @fields )
{
	String combined;
	for( uint i = 0; i < fields.length(); i++ ) {
		if( i > 0 ) {
			combined += " ";
		}
		combined += data.getField( table, rowIndex, fields[i] );
	}
	return combined;
}

void fuzzySearch_filterDatagrid( ElementDataGrid @grid, const String &in sourceName, const String &in table,
                                  const String &in query, array<String> @fields )
{
	if( @grid == null ) {
		return;
	}
	DataSource @data = getDataSource( sourceName );
	if( @data == null ) {
		return;
	}
	uint numRows = grid.getNumRows();
	for( uint i = 0; i < numRows; i++ ) {
		Element @row = grid.getRow( i );
		if( @row == null ) {
			continue;
		}
		String searchText = fuzzySearch_buildText( data, table, int( i ), fields );
		if( fuzzySearch_match( searchText, query ) ) {
			row.css( "display", "block" );
		} else {
			row.css( "display", "none" );
		}
	}
}

void fuzzySearch_saveQuery( const String &in query )
{
	fuzzySearch_savedQuery = query;
}

String fuzzySearch_loadQuery()
{
	return fuzzySearch_savedQuery;
}

void fuzzySearch_filterDivElementChildren( Element @container, const String &in query )
{
	if( @container == null ) {
		return;
	}
	uint numChildren = container.getNumChildren();
	for( uint i = 0; i < numChildren; i++ ) {
		Element @child = container.getChild( i );
		if( @child == null ) {
			continue;
		}
		String text = child.getInnerRML();
		if( fuzzySearch_match( text, query ) ) {
			child.css( "display", "block" );
		} else {
			child.css( "display", "none" );
		}
	}
}
