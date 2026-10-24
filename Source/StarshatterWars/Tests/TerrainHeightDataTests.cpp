#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Bitmap.h"
#include "LegacyBitmapLoader.h"
#include "DataLoader.h"
#include "Terrain.h"
#include "TerrainPatch.h"
#include "TerrainLayer.h"
namespace
{
class FAuditHeightTerrain : public Terrain
{
public: FAuditHeightTerrain():Terrain(nullptr){} void SetWaterTexture(Bitmap* B){water_texture=B;}
};
class FAuditHeightPatch : public TerrainPatch
{
public:
    FAuditHeightPatch(Terrain* T,const Bitmap* B):TerrainPatch(T,B,Rect(0,0,16,16),FVector(0,0,0),FVector(16,2,16)){}
    float Sample(int32 Row,int32 Col)const{return heights[Row*17+Col];}
};
TArray<uint8> PaddedPCX()
{
    TArray<uint8> B;B.SetNumZeroed(128);B[0]=10;B[1]=5;B[2]=1;B[3]=8;
    B[8]=2;B[10]=1;B[65]=1;B[66]=4;
    B.Append({10,20,30,99,30,20,10,88});B.Add(12);
    for(int32 I=0;I<256;++I){B.Add(uint8(I));B.Add(0);B.Add(0);}
    return B;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainHeightDataTest,"Starshatter.Audit.F10.TerrainHeightData",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTerrainHeightDataTest::RunTest(const FString& Parameters)
{
    TArray<uint8> Bytes=PaddedPCX();int32 W=0,H=0;TArray<FColor> Pixels;
    TestTrue(TEXT("Indexed PCX with padded rows decodes"),LegacyBitmapLoader::DecodePCX(Bytes,W,H,Pixels));
    TestEqual(TEXT("PCX width excludes padding"),W,3);TestEqual(TEXT("PCX height"),H,2);
    if(Pixels.Num()==6)
    {
        TestEqual(TEXT("Palette red value preserved"),Pixels[2].R,uint8(30));
        TestEqual(TEXT("Second row skips previous padding byte"),Pixels[3].R,uint8(30));
    }
    auto Invalid=Bytes;Invalid[128]=0xc0;TestFalse(TEXT("Zero-length PCX run is rejected"),LegacyBitmapLoader::DecodePCX(Invalid,W,H,Pixels));
    Invalid=Bytes;Invalid.SetNum(130);TestFalse(TEXT("Truncated PCX fails without pixel output"),LegacyBitmapLoader::DecodePCX(Invalid,W,H,Pixels));
    TestTrue(TEXT("Failed decode leaves empty result"),W==0&&H==0&&Pixels.IsEmpty());
    DataLoader Loader;Loader.SetDataPath("Galaxy/Trocanther System/");Bitmap Imported;
    TestTrue(TEXT("Real authored PCX loads through DataLoader"),Loader.LoadGameBitmap("volante_map.pcx",Imported)!=0);
    TestTrue(TEXT("Real image retains CPU pixels without a renderer"),Imported.Width()>0&&Imported.Height()>0&&Imported.HiPixels()!=nullptr);
    Loader.SetDataPath("Galaxy/Moons/");Bitmap PNG;
    TestTrue(TEXT("Real PNG also decodes through Unreal image support"),Loader.LoadGameBitmap("ice_map.png",PNG)!=0);
    TestTrue(TEXT("PNG CPU pixels retained"),PNG.HiPixels()!=nullptr);

    TArray<FColor> Gradient;Gradient.SetNum(17*17);
    for(int32 Y=0;Y<17;++Y)for(int32 X=0;X<17;++X)Gradient[Y*17+X]=FColor(uint8(X+Y),0,0,255);
    Bitmap HeightMap;HeightMap.SetCPUImage(17,17,Gradient.GetData(),Bitmap::BMP_SOLID,false);
    Bitmap Copy;Copy.CopyBitmap(HeightMap);HeightMap.SetColor(0,0,FColor(99,0,0));
    TestEqual(TEXT("Bitmap copies retain independent CPU pixels"),Copy.GetColor(0,0).R,uint8(0));
    HeightMap.SetCPUImage(17,17,Gradient.GetData(),Bitmap::BMP_SOLID,false);
    FAuditHeightTerrain Terrain;Terrain.GetLayers().append(new TerrainLayer);
    FAuditHeightPatch Patch(&Terrain,&HeightMap);
    TestEqual(TEXT("Mirrored X height and mountain scale match legacy"),Patch.Sample(0,0),32.f);
    TestEqual(TEXT("Opposite horizontal edge samples low texel"),Patch.Sample(0,16),0.f);
    TestEqual(TEXT("Vertical image progression preserved"),Patch.Sample(16,0),64.f);
    TestEqual(TEXT("Patch min height restored"),Patch.MinHeight(),0.0);
    TestEqual(TEXT("Patch max height restored"),Patch.MaxHeight(),64.0);
    TestEqual(TEXT("Bilinear height retains legacy five-unit clearance"),Patch.Height(0,0),37.0);
    TestEqual(TEXT("Last grid corner interpolates without reading past array"),Patch.Height(16,16),37.0);
    TestEqual(TEXT("Out-of-range query is rejected"),Patch.Height(17,16),0.0);
    Bitmap Water;Terrain.SetWaterTexture(&Water);FAuditHeightPatch Wet(&Terrain,&HeightMap);
    TestEqual(TEXT("Water cutout preserves authored red less than two"),Wet.Sample(0,16),-5000.f);
    Copy.ClearImage();TestTrue(TEXT("Clear invalidates CPU pixels"),Copy.HiPixels()==nullptr && Copy.Width()==0);
    return true;
}
#endif
