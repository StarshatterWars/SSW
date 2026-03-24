/*  Project nGenEx
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         DataLoader.cpp
	AUTHOR:       John DiCamillo

*/

#include "DataLoader.h"
#include "Bitmap.h"
#include "Sound.h"
#include "Resource.h"

// Minimal Unreal includes required for FVector / FColor:
#include "Math/Vector.h"
#include "Math/Color.h"

// +------------------------------------------------------------------+

static DataLoader* def_loader = 0;
DataLoader* DataLoader::loader = 0;

// +--------------------------------------------------------------------+

DataLoader::DataLoader()
	: datapath(""), video(0), use_file_system(true), enable_media(true)
{
}

// +--------------------------------------------------------------------+

void
DataLoader::Initialize()
{
	def_loader = new DataLoader;
	loader = def_loader;
}

void
DataLoader::Close()
{
	Bitmap::ClearCache();

	delete def_loader;
	def_loader = 0;
	loader = 0;
}

void
DataLoader::Reset()
{
	Close();
}

// +--------------------------------------------------------------------+

void
DataLoader::UseFileSystem(bool use)
{
	use_file_system = use;
}

void
DataLoader::EnableMedia(bool enable)
{
	enable_media = enable;
}

// +--------------------------------------------------------------------+

void
DataLoader::SetDataPath(const char* path)
{
	if (path)
		datapath = path;
	else
		datapath = "";
}

// +--------------------------------------------------------------------+

bool
DataLoader::FindFile(const char* name)
{
	// assemble file name:
	char filename[1024];
	strcpy_s(filename, datapath);
	strcat_s(filename, name);

	// first check current directory:
	if (use_file_system) {
		FILE* f;
		::fopen_s(&f, filename, "rb");

		if (f) {
			::fclose(f);
			return true;
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

int
DataLoader::ListFiles(const char* filter, List<Text>& list, bool recurse)
{
	list.destroy();

	ListFileSystem(filter, list, datapath, recurse);
	return list.size();
}

// +--------------------------------------------------------------------+

void
DataLoader::ListFileSystem(const char* filter, List<Text>& list, Text base_path, bool recurse)
{
	
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadBuffer(const char* name, BYTE*& buf, bool null_terminate, bool optional)
{
	buf = 0;

	// assemble file name:
	char filename[1024];
	strcpy_s(filename, datapath);
	strcat_s(filename, name);

	if (use_file_system) {
		// first check current directory:
		FILE* f;
		::fopen_s(&f, filename, "rb");

		if (f) {
			::fseek(f, 0, SEEK_END);
			int len = ftell(f);
			::fseek(f, 0, SEEK_SET);

			if (null_terminate) {
				buf = new BYTE[len + 1];
				if (buf)
					buf[len] = 0;
			}

			else {
				buf = new BYTE[len];
			}

			if (buf)
				::fread(buf, len, 1, f);

			::fclose(f);

			return len;
		}
	}

	if (!optional)
		Print(TEXT("WARNING - DataLoader could not load buffer '%s'\n"), filename);
	return 0;
}

// +--------------------------------------------------------------------+

int
DataLoader::fread(void* buffer, size_t size, size_t count, BYTE*& stream)
{
	CopyMemory(buffer, stream, size * count);
	stream += size * count;

	return size * count;
}

// +--------------------------------------------------------------------+

void
DataLoader::ReleaseBuffer(BYTE*& buf)
{
	delete[] buf;
	buf = 0;
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadGameBitmap(const char* name, Bitmap& bitmap, int type, bool optional)
{
	return 0;
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadTexture(const char* name, Bitmap*& bitmap, int type, bool preload_cache, bool optional)
{
	return 0;
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadSound(const char* name, USound*& snd, DWORD flags, bool optional)
{
	return 0;
}



