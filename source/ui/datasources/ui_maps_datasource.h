#ifndef __UI_MAPS_DATASOURCE_H__
#define __UI_MAPS_DATASOURCE_H__

#include <Rocket/Controls/DataSource.h>

namespace WSWUI
{
	/// Provides a list of available maps, with their full names, short
	/// names and pictures
	class MapsDataSource : public Rml::DataSource
	{
	public:
		MapsDataSource();

		virtual void GetRow (Rml::StringList &row, const Rml::String&, int row_index, const Rml::StringList& cols);
		virtual int GetNumRows (const Rml::String &table);

	private:
		typedef std::pair<std::string, std::string> MapInfo;
		typedef std::vector<MapInfo> MapList;

		MapList mapList;

		template<typename C>
		void getMapsList( C& maps_list );
	};
}

#endif // __UI_MAPS_DATASOURCE_H__
