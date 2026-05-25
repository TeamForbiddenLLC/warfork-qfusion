#pragma once
#ifndef __UI_FILERELOADER_H__
#define __UI_FILERELOADER_H__

#include <vector>
#include <string>
#include <ctime>

namespace WSWUI {

	class UI_FileReloader
	{
	public:
		UI_FileReloader();
		~UI_FileReloader();

		void checkForChanges( unsigned int currentTime );
		void clearTrackedFiles();

	private:
		static const unsigned int CHECK_INTERVAL_MS = 500;
		static const unsigned int COOLDOWN_MS = 1000;

		bool needsScan;
		unsigned int lastCheckTime;
		unsigned int lastReloadTime;

		struct FileEntry {
			std::string path;
			time_t lastMTime;
		};
		std::vector<FileEntry> trackedFiles;

		void scanUIDirectory();
		void pollFiles();
	};

}

#endif
