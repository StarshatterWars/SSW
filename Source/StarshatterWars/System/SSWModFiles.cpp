#include "SSWModFiles.h"
#include "SSWMissionMods.h"

#include "HAL/FileManager.h"
#include "Misc/Compression.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    constexpr int32 HeaderSize = 20;
    constexpr int32 RecordSize = 76;
    constexpr int32 MaxEntrySize = 64 * 1024 * 1024;

    uint32 ReadWord(const uint8* Bytes)
    {
        return uint32(Bytes[0]) | uint32(Bytes[1]) << 8 | uint32(Bytes[2]) << 16 | uint32(Bytes[3]) << 24;
    }

    FString RelativeKey(FString Path)
    {
        FPaths::NormalizeFilename(Path);
        if (!FPaths::IsRelative(Path))
        {
            FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("GameData"));
            FPaths::NormalizeDirectoryName(Root);
            Root += TEXT("/");
            if (!Path.StartsWith(Root)) return {};
            Path.RightChopInline(Root.Len());
        }
        TArray<FString> Parts;
        Path.ParseIntoArray(Parts, TEXT("/"));
        if (Parts.Contains(TEXT("..")) || Path.Contains(TEXT(":"))) return {};
        Parts.Remove(TEXT("."));
        return FString::Join(Parts, TEXT("/"));
    }

    bool ReadCompressed(FArchive& File, int64 Offset, uint32 PackedSize, uint32 Size, TArray<uint8>& Out)
    {
        if (!PackedSize || PackedSize > MaxEntrySize || Size > MaxEntrySize || Offset + PackedSize > File.TotalSize()) return false;
        TArray<uint8> Packed;
        Packed.SetNumUninitialized(PackedSize);
        File.Seek(Offset);
        File.Serialize(Packed.GetData(), Packed.Num());
        Out.SetNumUninitialized(FMath::Max(1u, Size));
        if (File.IsError() || !FCompression::UncompressMemory(NAME_Zlib, Out.GetData(), Size, Packed.GetData(), Packed.Num()))
        {
            Out.Reset();
            return false;
        }
        Out.SetNum(Size);
        return true;
    }

    bool ReadArchive(const FString& Filename, const FString& Key, TArray<uint8>& Out)
    {
        TUniquePtr<FArchive> File(IFileManager::Get().CreateFileReader(*Filename));
        if (!File || File->TotalSize() < HeaderSize) return false;
        uint8 Header[HeaderSize];
        File->Serialize(Header, HeaderSize);
        const uint32 Count = ReadWord(Header + 4);
        if (ReadWord(Header) != 0x10 || !Count || Count > MaxEntrySize / RecordSize) return false;
        TArray<uint8> Directory;
        if (!ReadCompressed(*File, HeaderSize + int64(ReadWord(Header + 16)), ReadWord(Header + 12), Count * RecordSize, Directory)) return false;
        for (uint32 Index = 0; Index < Count; ++Index)
        {
            const uint8* Record = Directory.GetData() + Index * RecordSize;
            ANSICHAR Name[65] = {};
            FMemory::Memcpy(Name, Record, 64);
            if (RelativeKey(ANSI_TO_TCHAR(Name)).Equals(Key, ESearchCase::IgnoreCase))
                return ReadCompressed(*File, HeaderSize + int64(ReadWord(Record + 72)), ReadWord(Record + 68), ReadWord(Record + 64), Out);
        }
        return false;
    }
}

namespace
{
bool ReadAlternatives(const TArray<FString>& Paths, TArray<uint8>& Bytes, int32& Selected)
{
    if (!GConfig) return false;
    TArray<FString> Keys;
    for (const FString& Path:Paths)
    {
        const FString Key=RelativeKey(FPaths::ConvertRelativePathToFull(Path));
        Keys.Add(Key.IsEmpty() && FPaths::IsRelative(Path) ? RelativeKey(Path) : Key);
    }
    const FString Root=SSWMissionMods::Directory();
    TArray<FString> Archives,Directories;
    IFileManager::Get().FindFiles(Archives,*(Root/TEXT("*.dat")),true,false);
    IFileManager::Get().FindFiles(Directories,*(Root/TEXT("*")),false,true);
    Directories.Append(Archives); Directories.Sort();
    for (int32 PackIndex=Directories.Num()-1;PackIndex>=0;--PackIndex)
    {
        const FString& Pack=Directories[PackIndex]; bool bEnabled=false;
        if (!GConfig->GetBool(TEXT("Starshatter.MissionMods"),*Pack,bEnabled,GGameUserSettingsIni) || !bEnabled) continue;
        const FString Filename=Root/Pack;
        for (int32 Index=0;Index<Keys.Num();++Index)
        {
            if (Keys[Index].IsEmpty()) continue;
            if (IFileManager::Get().DirectoryExists(*Filename))
            {
                const FString Loose=Filename/Keys[Index]; const int64 Size=IFileManager::Get().FileSize(*Loose);
                if (Size>=0 && Size<=MaxEntrySize && FFileHelper::LoadFileToArray(Bytes,*Loose)) { Selected=Index; return true; }
            }
            else if (ReadArchive(Filename,Keys[Index],Bytes)) { Selected=Index; return true; }
        }
    }
    return false;
}
}
bool SSWModFiles::Read(const FString& Path,TArray<uint8>& Bytes)
{
    int32 Selected=INDEX_NONE; return ReadAlternatives({Path},Bytes,Selected);
}
bool SSWModFiles::ReadMission(const FString& Path,TArray<uint8>& Bytes,bool& bJson)
{
    int32 Selected=INDEX_NONE;
    const bool Found=ReadAlternatives({FPaths::ChangeExtension(Path,TEXT("json")),FPaths::ChangeExtension(Path,TEXT("def"))},Bytes,Selected);
    bJson=Found && Selected==0; return Found;
}
