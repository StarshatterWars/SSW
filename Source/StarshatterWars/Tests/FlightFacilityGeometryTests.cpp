#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "ShipVisualCatalog.h"
#include "NativeShipGeometry.h"
#include "ShipActor.h"
#include "Camera.h"
#include "GameStructs.h"
#include "FlightFacilityCatalog.h"
#include "JsonObjectConverter.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
namespace
{
TArray<TSharedPtr<FJsonValue>> BayVector(const FVector& V)
{return {MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)};}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlightFacilityGeometryTest,"Starshatter.Facilities.NativeBayGeometryInventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlightFacilityGeometryTest::RunTest(const FString&)
{
    FString Json;TArray<TSharedPtr<FJsonValue>> Designs;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Designs))return false;
    auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);if(!TestNotNull(TEXT("Temporary geometry world"),World))return false;
    FScopedSSWTestWorldContext Context(World);ON_SCOPE_EXIT{World->DestroyWorld(false);};
    TArray<TSharedPtr<FJsonValue>> Report;
    for(const auto& V:Designs)
    {
        const auto O=V->AsObject();const FString Kind=O->GetStringField(TEXT("ShipClass"));
        if(!Kind.Equals(TEXT("Carrier"),ESearchCase::IgnoreCase) && !Kind.Equals(TEXT("Station"),ESearchCase::IgnoreCase) && !Kind.Equals(TEXT("Starbase"),ESearchCase::IgnoreCase))continue;
        const FString Name=O->GetStringField(TEXT("Row"));UClass* Class=ShipVisualCatalog::Resolve(Name,O->GetStringField(TEXT("Model")));
        if(!TestNotNull(*FString::Printf(TEXT("Facility Blueprint: %s"),*Name),Class))continue;
        auto* Actor=World->SpawnActorDeferred<AShipActor>(Class,FTransform::Identity,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!TestNotNull(TEXT("Facility preview actor"),Actor))continue;
        Actor->bAutoRebuildGeneratedComponents=false;Actor->FinishSpawning(FTransform::Identity);
        if(Name==TEXT("Asteroid Base") || Name==TEXT("Mining Station") || Name==TEXT("Processing Facility") || Name==TEXT("Shipyard"))Actor->SetActorScale3D(FVector(O->GetNumberField(TEXT("Scale"))));
        if(Name==TEXT("Asteroid Base"))
        {
            TArray<TPair<UStaticMeshComponent*,FTransform>> Assembly;
            for(UStaticMeshComponent* Part:TInlineComponentArray<UStaticMeshComponent*>(Actor))
                if(Part!=Actor->HullMesh && Part->GetStaticMesh())Assembly.Emplace(Part,Part->GetComponentTransform());
            Actor->OnConstruction(Actor->GetActorTransform());
            for(const auto& Part:Assembly)TestTrue(TEXT("Repeated shell alignment preserves doors/interior/contents"),Part.Key->GetComponentTransform().Equals(Part.Value,.001));
        }
        auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("design"),Name);Entry->SetStringField(TEXT("empire"),O->GetStringField(TEXT("ShipEmpire")));
        Entry->SetStringField(TEXT("actorClass"),Class->GetPathName());TArray<TSharedPtr<FJsonValue>> Parts;FBox Combined(ForceInit);
        for(UStaticMeshComponent* Part:TInlineComponentArray<UStaticMeshComponent*>(Actor))if(const UStaticMesh* Mesh=Part->GetStaticMesh())
        {
            const auto Box=Mesh->GetBounds().GetBox().TransformBy(Part->GetComponentTransform());Combined+=Box;
            auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("component"),Part->GetName());P->SetStringField(TEXT("mesh"),Mesh->GetPathName());P->SetStringField(TEXT("parent"),Part->GetAttachParent()?Part->GetAttachParent()->GetName():TEXT("none"));P->SetStringField(TEXT("relativeRotation"),Part->GetRelativeRotation().ToString());P->SetArrayField(TEXT("ueMin"),BayVector(Box.Min));P->SetArrayField(TEXT("ueMax"),BayVector(Box.Max));Parts.Add(MakeShared<FJsonValueObject>(P));
        }
        Entry->SetArrayField(TEXT("parts"),Parts);Entry->SetBoolField(TEXT("hasPrimaryHull"),Actor->HullMesh && Actor->HullMesh->GetStaticMesh());
        if(Combined.IsValid){Entry->SetArrayField(TEXT("legacyMin"),BayVector(FVector(Combined.Min.Y,Combined.Min.Z,Combined.Min.X)));Entry->SetArrayField(TEXT("legacyMax"),BayVector(FVector(Combined.Max.Y,Combined.Max.Z,Combined.Max.X)));}
        if(FParse::Param(FCommandLine::Get(),TEXT("SSWBayMeshExport")) && Actor->HullMesh && Actor->HullMesh->GetStaticMesh())
        {
            const auto* Data=Actor->HullMesh->GetStaticMesh()->GetRenderData();
            if(Data && Data->LODResources.Num())
            {
                const auto& LOD=Data->LODResources[0];TArray<TSharedPtr<FJsonValue>> Vertices,Indices;
                for(uint32 I=0;I<LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();++I)
                {
                    const FVector Position=Actor->HullMesh->GetComponentTransform().TransformPosition(FVector(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(I)));
                    Vertices.Add(MakeShared<FJsonValueArray>(BayVector(FVector(Position.Y,Position.Z,Position.X))));
                }
                for(int32 I=0;I<LOD.IndexBuffer.GetNumIndices();++I)Indices.Add(MakeShared<FJsonValueNumber>(LOD.IndexBuffer.GetIndex(I)));
                auto Mesh=MakeShared<FJsonObject>();Mesh->SetArrayField(TEXT("vertices"),Vertices);Mesh->SetArrayField(TEXT("indices"),Indices);FString MeshText;FJsonSerializer::Serialize(Mesh,TJsonWriterFactory<>::Create(&MeshText));
                FFileHelper::SaveStringToFile(MeshText,*(FPaths::ProjectSavedDir()/TEXT("Analysis/BayMesh-")+Name+TEXT(".json")));
            }
        }
        Camera Pose;FNativeShipGeometry Geometry;TArray<TSharedPtr<FJsonValue>> Decks;int32 Index=0;
        for(const auto& D:O->GetArrayField(TEXT("FlightDeck")))
        {
            FShipFlightDeck Resolved;if(!FJsonObjectConverter::JsonObjectToUStruct(D->AsObject().ToSharedRef(),&Resolved))return false;
            if(!FlightFacilityCatalog::ResolveDeck(FName(*Name),Index,Resolved)){++Index;continue;}
            auto Record=MakeShared<FJsonObject>();Record->SetNumberField(TEXT("index"),Index++);Record->SetStringField(TEXT("name"),Resolved.Name);
            const bool Recovery=Resolved.bRecoveryDeck;double Az=Resolved.AzimuthRadians;
            Camera Rail;Rail.Yaw(Az);FVector Start,Direction;double Length=0;
            if(Recovery){Start=Resolved.StartPoint;const FVector End=Resolved.EndPoint;Direction=(End-Start).GetSafeNormal();Length=FVector::Distance(Start,End);}
            else if(!Resolved.Slots.IsEmpty()){Start=Resolved.Slots[0].Location;Direction=Rail.vpn();Length=FMath::Max(500.0,FVector::Distance(Start,Resolved.EndPoint));}
            FVector Hit;Weapon* HitWeapon=nullptr;const int32 Result=Geometry.Ray(*Actor,Pose,Start,Direction,FMath::Max(0.0,Length-.1),Hit,HitWeapon);
            TestEqual(*FString::Printf(TEXT("Native bay centerline clear: %s/%d"),*Name,Index-1),Result,0);
            if(!Recovery)for(int32 SlotIndex=1;SlotIndex<Resolved.Slots.Num();++SlotIndex)
            {
                const FVector SlotStart=Resolved.Slots[SlotIndex].Location;FVector SlotHit;Weapon* SlotWeapon=nullptr;
                const double SlotLength=FMath::Max(500.0,FVector::Distance(SlotStart,Resolved.EndPoint));
                TestEqual(*FString::Printf(TEXT("Native launch slot clear: %s/%d/%d"),*Name,Index-1,SlotIndex),Geometry.Ray(*Actor,Pose,SlotStart,Direction,SlotLength-.1,SlotHit,SlotWeapon),0);
            }
            Record->SetNumberField(TEXT("centerRayResult"),Result);Record->SetArrayField(TEXT("start"),BayVector(Start));Record->SetArrayField(TEXT("direction"),BayVector(Direction));Record->SetNumberField(TEXT("length"),Length);
            if(Result>0){Record->SetArrayField(TEXT("hit"),BayVector(Hit));Record->SetNumberField(TEXT("hitDistance"),FVector::Distance(Start,Hit));}
            if(Result>0)
            {
                TArray<TSharedPtr<FJsonValue>> Candidates;
                for(const double Turn:{0.0,double(PI)/2,-double(PI)/2,double(PI)})
                {
                    Camera CandidateRail;CandidateRail.Yaw(Az+Turn);const FVector Forward=CandidateRail.vpn(),Right=CandidateRail.vrt();
                    for(const double Side:{0.,-.5,.5,-1.,1.,-2.,2.})for(const double Height:{0.,.25,-.25,.5,-.5,1.,-1.})
                    {
                        const FVector Offset=Right*(Side*Resolved.Size)+FVector(0,Height*Resolved.Size,0);
                        const FVector End=Recovery?Resolved.EndPoint+Offset:Start+Offset;
                        const FVector From=Recovery?End-Forward*Length:End;
                        FVector CandidateHit;Weapon* CandidateWeapon=nullptr;
                        if(Geometry.Ray(*Actor,Pose,From,Forward,Length,CandidateHit,CandidateWeapon)!=0)continue;
                        auto Candidate=MakeShared<FJsonObject>();Candidate->SetNumberField(TEXT("turnDegrees"),FMath::RadiansToDegrees(Turn));Candidate->SetNumberField(TEXT("sideSizes"),Side);Candidate->SetNumberField(TEXT("heightSizes"),Height);Candidate->SetArrayField(TEXT("point"),BayVector(End));Candidates.Add(MakeShared<FJsonValueObject>(Candidate));
                    }
                }
                Record->SetArrayField(TEXT("diagnosticClearCandidates"),Candidates);
            }
            Decks.Add(MakeShared<FJsonValueObject>(Record));
        }
        Entry->SetArrayField(TEXT("decks"),Decks);Report.Add(MakeShared<FJsonValueObject>(Entry));Actor->Destroy();
    }
    auto Root=MakeShared<FJsonObject>();Root->SetStringField(TEXT("scope"),TEXT("Read-only spawned Blueprint/primary cooked hull centerline inventory. Center rays are diagnostic, not full craft-clearance or a flown launch/recovery certification. Compound hulls/doors may require additional component testing."));Root->SetArrayField(TEXT("facilities"),Report);FString Out;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));
    TestEqual(TEXT("Every authored carrier/base/station inventoried"),Report.Num(),23);
    TestTrue(TEXT("Geometry evidence written"),FFileHelper::SaveStringToFile(Out,*(FPaths::ProjectSavedDir()/TEXT("Analysis/FlightFacilityGeometry.json"))));return true;
}
#endif
