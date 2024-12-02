/*
 * UI_FileInterface.h
 *
 *  Created on: 25.6.2011
 *      Author: hc
 */

#ifndef UI_FILEINTERFACE_H_
#define UI_FILEINTERFACE_H_

#include <RmlUi/Core/FileInterface.h>

namespace WSWUI
{

class UI_FileInterface : public Rml::FileInterface
{
public:
	UI_FileInterface();
	virtual ~UI_FileInterface();

	//// Implement FileInterface

	/// Opens a file.
	virtual Rml::FileHandle Open(const Rml::String& path);

	/// Closes a previously opened file.
	virtual void Close(Rml::FileHandle file);

	/// Reads data from a previously opened file.
	virtual size_t Read(void* buffer, size_t size, Rml::FileHandle file);

	/// Seeks to a point in a previously opened file.
	virtual bool Seek(Rml::FileHandle file, long offset, int origin);

	/// Returns the current position of the file pointer.
	virtual size_t Tell(Rml::FileHandle file);

	/// Returns the length of the file.
	virtual size_t Length(Rml::FileHandle file);

private:
	typedef std::map<int, size_t> fileSizeMap_t;
	fileSizeMap_t fileSizeMap;
};

}

#endif /* UI_FILEINTERFACE_H_ */
