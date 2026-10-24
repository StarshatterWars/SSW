/*  Project nGenEx
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    nGenEx.lib
	FILE:         DataLoader.cpp
	AUTHOR:       John DiCamillo

*/

#include "DataLoader.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Bitmap.h"
#include "Sound.h"
#include "Resource.h"
#include "Misc/Paths.h"
#include "LegacyBitmapLoader.h"
#include "Misc/App.h"
#include "Misc/PackageName.h"

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
    const FString Base=ANSI_TO_TCHAR(base_path.data());
    TArray<FString> Roots;
    if(FPaths::IsRelative(Base)) {Roots.Add(FPaths::ConvertRelativePathToFull(Base));Roots.Add(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("GameData")/Base));}
    else Roots.Add(Base);
    TSet<FString> Seen;TArray<FString> Names;
    for(FString Root:Roots)
    {
        FPaths::NormalizeDirectoryName(Root);TArray<FString> Found;
        if(recurse) IFileManager::Get().FindFilesRecursive(Found,*Root,ANSI_TO_TCHAR(filter),true,false,false);
        else {IFileManager::Get().FindFiles(Found,*(Root/ANSI_TO_TCHAR(filter)),true,false);for(auto& Name:Found)Name=Root/Name;}
        for(FString Name:Found) {FPaths::MakePathRelativeTo(Name,*(Root+TEXT("/")));const FString Key=Name.ToLower();if(!Seen.Contains(Key)){Seen.Add(Key);Names.Add(Name);}}
    }
    Names.Sort();for(const auto& Name:Names) list.append(new Text(TCHAR_TO_ANSI(*Name)));
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
    if(!enable_media || !name || !name[0])return 0;
    const FString Relative=FString(UTF8_TO_TCHAR(GetDataPath()))/UTF8_TO_TCHAR(name);
    TArray<FString> Paths;
    if(FPaths::IsRelative(Relative))Paths.Add(FPaths::ProjectContentDir()/TEXT("GameData")/Relative);
    Paths.Add(Relative);
    for(const FString& Path : Paths)
    {
        int32 W=0,H=0;TArray<FColor> Pixels;
        if(LegacyBitmapLoader::Load(Path,W,H,Pixels))
        {
            bitmap.SetCPUImage(W,H,Pixels.GetData(),type);
            bitmap.SetFilename(TCHAR_TO_UTF8(*Relative));return 1;
        }
    }
    bitmap.ClearImage();
    if(!optional)UE_LOG(LogTemp,Warning,TEXT("[DataLoader] Bitmap data missing or invalid: %s"),*Relative);
    return 0;
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadTexture(const char* name, Bitmap*& bitmap, int type, bool preload_cache, bool optional)
{
    bitmap=nullptr;if(!enable_media || !name || !name[0])return 0;
    const FString Key=FString(UTF8_TO_TCHAR(GetDataPath()))/UTF8_TO_TCHAR(name);
    bitmap=Bitmap::CheckCache(TCHAR_TO_UTF8(*Key));if(bitmap)return 1;
    Bitmap* Loaded=new Bitmap;
    if(!LoadGameBitmap(name,*Loaded,type,optional)){delete Loaded;return 0;}
    if(FApp::CanEverRender())Loaded->MakeTexture();
    Bitmap::AddToCache(Loaded);bitmap=Loaded;return 1;
}

// +--------------------------------------------------------------------+

int
DataLoader::LoadSound(const char* name, USound*& snd, DWORD flags, bool optional)
{
    snd=nullptr;
    if (!enable_media || !name || !name[0]) return 0;
    FString Input=UTF8_TO_TCHAR(name);
    Input.ReplaceInline(TEXT("\\"),TEXT("/"));
    TArray<FString> Candidates;
    if (Input.StartsWith(TEXT("/Game/")) || Input.StartsWith(TEXT("/Engine/")))
    {
        const FString Extension=FPaths::GetExtension(Input);
        if (Extension.Equals(TEXT("wav"),ESearchCase::IgnoreCase) || Extension.Equals(TEXT("ogg"),ESearchCase::IgnoreCase))
            Input=FPaths::ChangeExtension(Input,TEXT(""));
        Candidates.Add(Input.Contains(TEXT(".")) ? Input : Input+TEXT(".")+FPaths::GetCleanFilename(Input));
    }
    else
    {
        FString Relative=FString(UTF8_TO_TCHAR(GetDataPath()))/Input;
        Relative.ReplaceInline(TEXT("\\"),TEXT("/"));
        Relative=FPaths::ChangeExtension(Relative,TEXT(""));
        while(Relative.StartsWith(TEXT("/")))Relative.RightChopInline(1);
        for(const TCHAR* Root : {TEXT("/Game/Audio/"),TEXT("/Game/GameData/"),TEXT("/Game/")})
        {
            const FString Package=FString(Root)+Relative;
            Candidates.Add(Package+TEXT(".")+FPaths::GetCleanFilename(Package));
        }
    }
    for (const FString& Candidate : Candidates)
    {
        const FString Package=FPackageName::ObjectPathToPackageName(Candidate);
        if (!FPackageName::IsValidLongPackageName(Package) || !FPackageName::DoesPackageExist(Package)) continue;
        if (USoundBase* Asset=LoadObject<USoundBase>(nullptr,*Candidate,nullptr,LOAD_NoWarn|LOAD_Quiet))
        {
            snd=USound::CreateFromAsset(Asset,TCHAR_TO_UTF8(*Candidate),flags);
            return snd ? 1 : 0;
        }
    }
    if (!optional) UE_LOG(LogTemp,Warning,TEXT("[DataLoader] Sound asset missing: '%s' (path '%hs')"),*Input,GetDataPath());
    return 0;
}



