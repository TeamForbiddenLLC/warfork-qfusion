#ifndef __UI_WORKSHOP_DATASOURCE_H__
#define __UI_WORKSHOP_DATASOURCE_H__

#include <Rocket/Controls/DataSource.h>
#include <string>
#include <vector>

struct steam_evt_pkt_s;

namespace WSWUI
{
	class WorkshopDataSource : public Rocket::Controls::DataSource
	{
	public:
		WorkshopDataSource();
		~WorkshopDataSource();

		// Triggers a Steam refresh; datasource updates reactively via events.
		void Refresh();

		void GetRow( Rocket::Core::StringList &row, const Rocket::Core::String &table, int row_index, const Rocket::Core::StringList &cols ) override;
		int GetNumRows( const Rocket::Core::String &table ) override;

	private:
		struct InstalledMod {
			std::string title;
			std::string name;
			std::string workshop_id;
			bool        is_local = false;
		};
		std::vector<InstalledMod> installedMods;

		void UpdateMods();

		static void OnWorkshopDetail( void *self, struct steam_evt_pkt_s *pkt );
		static void OnWorkshopItemInstalled( void *self, struct steam_evt_pkt_s *pkt );
		static void OnWorkshopItemUnsubscribed( void *self, struct steam_evt_pkt_s *pkt );
	};
}

#endif // __UI_WORKSHOP_DATASOURCE_H__
