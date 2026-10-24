#include "LegacyBitmapLoader.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
bool LegacyBitmapLoader::DecodePCX(TConstArrayView<uint8> B,int32& W,int32& H,TArray<FColor>& Pixels)
{
    W=H=0;Pixels.Reset();
    if(B.Num()<128 || B[0]!=10 || B[2]!=1 || B[3]!=8)return false;
    auto U16=[B](int32 I){return int32(B[I])|(int32(B[I+1])<<8);};
    const int32 Width=U16(8)-U16(4)+1,Height=U16(10)-U16(6)+1;
    const int32 Planes=B[65],Stride=U16(66);
    if(Width<1 || Height<1 || Stride<Width || (Planes!=1 && Planes!=3) || int64(Width)*Height>16*1024*1024)return false;
    const int32 PaletteStart=B.Num()-768,DataEnd=Planes==1 ? PaletteStart-1 : B.Num();
    if(Planes==1 && (DataEnd<128 || B[DataEnd]!=12))return false;
    const int32 RowBytes=Stride*Planes;
    TArray<uint8> Row;Row.SetNumUninitialized(RowBytes);
    TArray<FColor> Decoded;Decoded.SetNumUninitialized(Width*Height);
    int32 Pos=128;
    for(int32 Y=0;Y<Height;++Y)
    {
        int32 Out=0;
        while(Out<RowBytes)
        {
            if(Pos>=DataEnd)return false;
            uint8 Value=B[Pos++];int32 Count=1;
            if((Value&0xc0)==0xc0)
            {
                Count=Value&0x3f;
                if(Count==0 || Pos>=DataEnd)return false;
                Value=B[Pos++];
            }
            if(Count>RowBytes-Out)return false;
            FMemory::Memset(Row.GetData()+Out,Value,Count);Out+=Count;
        }
        for(int32 X=0;X<Width;++X)
            Decoded[Y*Width+X]=Planes==1 ? FColor(B[PaletteStart+3*Row[X]],B[PaletteStart+3*Row[X]+1],B[PaletteStart+3*Row[X]+2],255)
                : FColor(Row[X],Row[Stride+X],Row[2*Stride+X],255);
    }
    W=Width;H=Height;Pixels=MoveTemp(Decoded);return true;
}
bool LegacyBitmapLoader::Load(const FString& Path,int32& W,int32& H,TArray<FColor>& Pixels)
{
    W=H=0;Pixels.Reset();TArray<uint8> Bytes;
    if(!FFileHelper::LoadFileToArray(Bytes,*Path))return false;
    if(FPaths::GetExtension(Path).Equals(TEXT("pcx"),ESearchCase::IgnoreCase))return DecodePCX(Bytes,W,H,Pixels);
    FImage Image;
    if(!FImageUtils::DecompressImage(Bytes.GetData(),Bytes.Num(),Image) || Image.SizeX<1 || Image.SizeY<1 || int64(Image.SizeX)*Image.SizeY>16*1024*1024)return false;
    FImage BGRA;Image.CopyTo(BGRA,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
    W=BGRA.SizeX;H=BGRA.SizeY;
    Pixels.Append(reinterpret_cast<const FColor*>(BGRA.RawData.GetData()),W*H);return true;
}
