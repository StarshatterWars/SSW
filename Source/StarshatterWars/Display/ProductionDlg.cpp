#include "ProductionDlg.h"
#include "BlueprintsDlg.h"
#include "ShipSystemIcon.h"
#include "ReprocessingAI.h"

#include "GameStructs_products.h"
#include "MissionUIStyle.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
using FFacilityPtr = TSharedPtr<FSSWProductionFacility>;

struct FRecipeLabels
{
    FText Minerals;
    FText Product;
    FText ProcessingTime;
    FName Image;
};

const FSSWProductionStock* GetLiveStock(const FSSWProductionFacility& Facility)
{
    Sim* Simulation = Sim::GetSim();
    return Simulation ? Simulation->GetProductionStock(Facility) : nullptr;
}

FText GetProductionStatus(const FSSWProductionStock* Stock)
{
    if (!Stock) return INVTEXT("NO LIVE DATA");
    if (Stock->RemainingSeconds > 0) return INVTEXT("PRODUCING");
    if (Stock->GetTonnes(FName(TEXT("Finished Goods Storage"))) > 0) return INVTEXT("OUTPUT READY");
    return INVTEXT("AWAITING ORE");
}

class SProductionRow : public SMultiColumnTableRow<FFacilityPtr>
{
public:
    SLATE_BEGIN_ARGS(SProductionRow) {}
        SLATE_ARGUMENT(FFacilityPtr, Facility)
        SLATE_ARGUMENT(FText, Minerals)
        SLATE_ARGUMENT(FText, Product)
        SLATE_ARGUMENT(FText, ProcessingTime)
        SLATE_ARGUMENT(FName, Image)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    {
        Facility = Args._Facility;
        Minerals = Args._Minerals;
        Product = Args._Product;
        ProcessingTime = Args._ProcessingTime;
        Image = Args._Image;
        SMultiColumnTableRow<FFacilityPtr>::Construct(FSuperRowType::FArguments().Style(&MissionUIStyle::FlightTableRowStyle()), Owner);
    }

    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        if (Column == "Facility")
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4,2)[SNew(SShipSystemIcon).Icon(SShipSystemIcon::ForCatalogImage(Facility->Image))]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(FText::FromString(Facility->Facility).ToUpper()).OverflowPolicy(ETextOverflowPolicy::Ellipsis)];
        if (Column == "Product")
            return SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(4,2)[SNew(SShipSystemIcon).Icon(SShipSystemIcon::ForCatalogImage(Image))]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Product.ToUpper()).OverflowPolicy(ETextOverflowPolicy::Ellipsis)];
        if (Column == "Status")
            return SNew(STextBlock).Margin(FMargin(5, 4)).Font(MissionUIStyle::GetRowFont(12))
                .Text_Lambda([Site = Facility]() { return GetProductionStatus(GetLiveStock(*Site)); });

        FText Label;
        if (Column == "Facility") Label = FText::FromString(Facility->Facility);
        else if (Column == "System") Label = FText::FromString(Facility->System);
        else if (Column == "Planet") Label = FText::FromString(Facility->Planet);
        else if (Column == "Region") Label = FText::FromString(Facility->Region);
        else if (Column == "Empire") Label = FText::FromString(Facility->Empire);
        else if (Column == "Type") Label = Facility->bSurface ? INVTEXT("SURFACE") : INVTEXT("ORBITAL");
        else if (Column == "Minerals") Label = Minerals;
        else if (Column == "Product") Label = Product;
        else if (Column == "ProcessingTime") Label = ProcessingTime;
        else if (Column == "Capacity")
            Label = FText::Format(INVTEXT("{0} t"), FText::AsNumber(Facility->CapacityTonnes));
        return SNew(STextBlock).Margin(FMargin(5, 4)).Font(MissionUIStyle::GetRowFont(12))
            .Text(Label.ToUpper()).ToolTipText(Label).OverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }

private:
    FFacilityPtr Facility;
    FText Minerals;
    FText Product;
    FText ProcessingTime;
    FName Image;
};
}

TSharedRef<SWidget> UProductionDlg::RebuildWidget()
{
    const auto Rows = MakeShared<TArray<FFacilityPtr>>();
    const auto RecipeLabels = MakeShared<TMap<FName, FRecipeLabels>>();
    const auto* Table = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Game/DT_ProductionFacilities.DT_ProductionFacilities"));
    if (Table && Table->GetRowStruct() == FSSWProductionFacility::StaticStruct())
        Table->ForeachRow<FSSWProductionFacility>(TEXT("Production facilities"),
            [&Rows](const FName&, const FSSWProductionFacility& Row)
            {
                Rows->Add(MakeShared<FSSWProductionFacility>(Row));
            });
    if (Rows->IsEmpty())
        return SNew(STextBlock).Margin(FMargin(12)).Text(INVTEXT("NO PRODUCTION FACILITIES AVAILABLE"));

    Rows->Sort([](const FFacilityPtr& A, const FFacilityPtr& B)
    {
        const int32 SystemOrder = A->System.Compare(B->System);
        return SystemOrder == 0 ? A->Facility < B->Facility : SystemOrder < 0;
    });
    const auto* Recipes = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/Game/DT_ProductionRecipes.DT_ProductionRecipes"));
    if (Recipes && Recipes->GetRowStruct() == FSSWProductionRecipe::StaticStruct())
        Recipes->ForeachRow<FSSWProductionRecipe>(TEXT("Production outputs"),
            [&RecipeLabels](const FName& Name, const FSSWProductionRecipe& Recipe)
            {
                TArray<FText> Minerals;
                for (const ESSWMineral Mineral : Recipe.Minerals)
                {
                    FSSWAsteroidResource Resource;
                    Resource.Mineral = Mineral;
                    Minerals.Add(Resource.GetMineralName());
                }
                if (!Recipe.Inputs.IsEmpty())
                {
                    Minerals.Reset();
                    for (const auto& Input : Recipe.Inputs)
                        Minerals.Add(FText::Format(INVTEXT("{0} t {1}"), FText::AsNumber(Input.Quantity * Input.UnitTonnes), FText::FromName(Input.Item)));
                }
                RecipeLabels->Add(Name, FRecipeLabels{
                    FText::Format(INVTEXT("{0} t: {1}"), FText::AsNumber(Recipe.InputTonnes),
                        FText::Join(INVTEXT(", "), Minerals)),
                    Recipe.CargoItem.IsEmpty()
                        ? FText::Format(INVTEXT("{0} t: {1}"), FText::AsNumber(Recipe.OutputTonnes),
                            StaticEnum<ESSWFinishedItem>()->GetDisplayNameTextByValue(int64(Recipe.Product)))
                        : FText::Format(INVTEXT("{0} {1}: {2}"), FText::AsNumber(Recipe.OutputQuantity),
                            FText::FromString(Recipe.OutputUnit), FText::FromString(Recipe.OutputDisplayName)),
                    FText::Format(INVTEXT("{0} s | {1} t residue"), FText::AsNumber(Recipe.SecondsPerBatch),
                        FText::AsNumber(ReprocessingAI::Calculate(Recipe).WasteTonnes)), Recipe.Image
                });
            });

    auto Header = SNew(SHeaderRow);
    const auto Column = [&Header](FName Id, FText Label, float Weight)
    {
        Header->AddColumn(SHeaderRow::Column(Id).DefaultLabel(Label).FillWidth(Weight)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(12)).Text(Label.ToUpper())]);
    };
    Column("Facility", INVTEXT("FACILITY"), 2.5f);
    Column("System", INVTEXT("SYSTEM"), 1.f);
    Column("Planet", INVTEXT("PLANET"), 1.f);
    Column("Region", INVTEXT("REGION"), 1.f);
    Column("Empire", INVTEXT("EMPIRE"), 1.6f);
    Column("Type", INVTEXT("TYPE"), 0.8f);
    Column("Minerals", INVTEXT("INPUT / BATCH"), 2.1f);
    Column("Product", INVTEXT("OUTPUT / BATCH"), 2.1f);
    Column("ProcessingTime", INVTEXT("BATCH TIME"), 1.1f);
    Column("Capacity", INVTEXT("MAX STORAGE"), 1.3f);
    Column("Status", INVTEXT("STATUS"), 1.2f);

    const auto Selected = MakeShared<FFacilityPtr>();
    auto Screens = SNew(SWidgetSwitcher);
    const auto OpenFactory = [Selected, WeakScreens = TWeakPtr<SWidgetSwitcher>(Screens)](FFacilityPtr Site)
    {
        if (Site)
        {
            *Selected = MoveTemp(Site);
            if (const auto View = WeakScreens.Pin()) View->SetActiveWidgetIndex(1);
        }
    };
    const auto VisibleRows = MakeShared<TArray<FFacilityPtr>>(*Rows);
    auto List = SNew(SListView<FFacilityPtr>).ListItemsSource(&*VisibleRows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::Single)
        .OnMouseButtonDoubleClick_Lambda(OpenFactory)
        .OnSelectionChanged_Lambda([Selected](FFacilityPtr Site, ESelectInfo::Type)
        {
            *Selected = MoveTemp(Site);
        })
        .OnGenerateRow_Lambda([VisibleRows, RecipeLabels](FFacilityPtr Site, const TSharedRef<STableViewBase>& Owner)
            -> TSharedRef<ITableRow>
        {
            const FRecipeLabels* Recipe = RecipeLabels->Find(Site->Recipe);
            return SNew(SProductionRow, Owner).Facility(Site)
                .Image(Recipe ? Recipe->Image : FName(TEXT("Production")))
                .Minerals(Recipe ? Recipe->Minerals : INVTEXT("UNASSIGNED"))
                .Product(Recipe ? Recipe->Product : INVTEXT("UNASSIGNED"))
                .ProcessingTime(Recipe ? Recipe->ProcessingTime : INVTEXT("—"));
        });

    const auto ApplyRange = [Rows, VisibleRows, Selected, WeakList = TWeakPtr<SListView<FFacilityPtr>>(List)](int32 Range)
    {
        const auto View = WeakList.Pin();
        if (!View) return FReply::Handled();
        View->ClearSelection();
        Selected->Reset();

        Sim* Simulation = Sim::GetSim();
        SimRegion* Region = Simulation ? Simulation->GetActiveRegion() : nullptr;
        StarSystem* LocalSystem = Region ? Region->GetSystem() : nullptr;
        TSet<FName> Nearby;
        if (LocalSystem)
        {
            Nearby.Add(FName(ANSI_TO_TCHAR(LocalSystem->GetName())));
            if (const auto* Environment = UStarshatterEnvironmentSubsystem::Get())
                for (StarSystem* System : Environment->GetRuntimeStarSystems())
                    if (System && System != LocalSystem && LocalSystem->HasLinkTo(System))
                        Nearby.Add(FName(ANSI_TO_TCHAR(System->GetName())));
        }

        VisibleRows->Reset();
        for (const auto& Site : *Rows)
        {
            const bool bCurrentRegion = Region && LocalSystem &&
                Site->System.Equals(ANSI_TO_TCHAR(LocalSystem->GetName()), ESearchCase::IgnoreCase) &&
                Site->Region.Equals(ANSI_TO_TCHAR(Region->GetName()), ESearchCase::IgnoreCase);
            if (Range == 0 || (Range == 1 && bCurrentRegion) ||
                (Range == 2 && Nearby.Contains(FName(*Site->System))))
                VisibleRows->Add(Site);
        }
        View->RequestListRefresh();
        return FReply::Handled();
    };
    auto Filters = SNew(SHorizontalBox);
    const FText FilterLabels[] = {INVTEXT("SHOW ALL"), INVTEXT("CURRENT REGION"), INVTEXT("WITHIN 1 JUMP")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(FilterLabels); ++Index)
        Filters->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
        [
            SNew(SButton).Text(FilterLabels[Index])
            .OnClicked_Lambda([ApplyRange, Index]() { return ApplyRange(Index); })
        ];

    auto Browser = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(12, 8)
        [Filters]
        + SVerticalBox::Slot().FillHeight(0.65f).Padding(12, 0)
        [MissionUIStyle::MakeDataPane(List, FMargin(0))]
        + SVerticalBox::Slot().FillHeight(0.35f).Padding(12, 8, 12, 10)
        [
            MissionUIStyle::MakeDataPane(
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(STextBlock)
                .Font(MissionUIStyle::GetRowFont(16))
                .Text_Lambda([Selected]()
                {
                    return *Selected ? FText::FromString((*Selected)->Facility).ToUpper()
                        : INVTEXT("SELECT A PRODUCTION FACILITY");
                })
            ]
            + SScrollBox::Slot().Padding(0, 6)
            [
                SNew(SButton).Text(INVTEXT("OPEN FACTORY"))
                .Visibility_Lambda([Selected]() { return *Selected ? EVisibility::Visible : EVisibility::Collapsed; })
                .OnClicked_Lambda([OpenFactory, Selected]() { OpenFactory(*Selected); return FReply::Handled(); })
            ]
            + SScrollBox::Slot().Padding(0, 6)
            [
                SNew(STextBlock)
                .Font(MissionUIStyle::GetRowFont(13))
                .AutoWrapText(true)
                .Text_Lambda([Selected, RecipeLabels]()
                {
                    if (!*Selected) return FText::GetEmpty();

                    const auto& Site = **Selected;
                    const auto* Recipe = RecipeLabels->Find(Site.Recipe);
                    const auto* Stock = GetLiveStock(Site);
                    TArray<FText> Lines;
                    Lines.Add(FText::Format(INVTEXT("{0}  |  {1}  |  {2}  |  {3}  |  {4}"),
                        FText::FromString(Site.System), FText::FromString(Site.Planet),
                        FText::FromString(Site.Region), FText::FromString(Site.Empire),
                        Site.bSurface ? INVTEXT("SURFACE") : INVTEXT("ORBITAL")));
                    Lines.Add(FText::Format(INVTEXT("MAX STORAGE  {0} T  |  {1}"),
                        FText::AsNumber(Site.CapacityTonnes), GetProductionStatus(Stock)));
                    if (Recipe)
                    {
                        Lines.Add(FText::Format(INVTEXT("INPUT PER BATCH  {0}"), Recipe->Minerals));
                        Lines.Add(FText::Format(INVTEXT("OUTPUT PER BATCH  {0}"), Recipe->Product));
                        Lines.Add(FText::Format(INVTEXT("BATCH TIME  {0}"), Recipe->ProcessingTime));
                    }
                    else
                    {
                        Lines.Add(INVTEXT("NO RECIPE ASSIGNED"));
                    }
                    return FText::Join(INVTEXT("\n"), Lines).ToUpper();
                })
            ]
            )
        ];
    const auto FactoryColumn = [](const FText& Header, TAttribute<FText> Body)
    {
        return SNew(SBorder).Padding(14).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(MissionUIStyle::PanelBG.CopyWithNewOpacity(1.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
                [SNew(STextBlock).Text(Header).Font(MissionUIStyle::GetRowFont(16))]
                + SVerticalBox::Slot().FillHeight(1)
                [SNew(SScrollBox) + SScrollBox::Slot()
                    [SNew(STextBlock).Text(Body).AutoWrapText(true).Font(MissionUIStyle::GetRowFont(14))]]
            ];
    };
    auto Factory = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(12, 10)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
            [
                SNew(SButton).Text(INVTEXT("FACILITIES"))
                .OnClicked_Lambda([WeakScreens = TWeakPtr<SWidgetSwitcher>(Screens)]()
                {
                    if (const auto View = WeakScreens.Pin()) View->SetActiveWidgetIndex(0);
                    return FReply::Handled();
                })
            ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [
                SNew(STextBlock).Font(MissionUIStyle::GetRowFont(18))
                .Text_Lambda([Selected]() { return *Selected ? FText::FromString((*Selected)->Facility).ToUpper() : FText::GetEmpty(); })
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1).Padding(12, 0, 12, 12)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(2).Padding(0, 0, 8, 0)
            [
                FactoryColumn(INVTEXT("BLUEPRINT / MATERIALS"), TAttribute<FText>::CreateLambda([Selected, RecipeLabels]()
                {
                    if (!*Selected) return FText::GetEmpty();
                    const auto* Recipe = RecipeLabels->Find((*Selected)->Recipe);
                    if (!Recipe) return INVTEXT("NO BLUEPRINT ASSIGNED");
                    const auto* Stock = GetLiveStock(**Selected);
                    return FText::Format(INVTEXT("{0}\n\nINPUT PER BATCH\n{1}\n\nAVAILABLE INPUT\n{2}"),
                        FText::FromName((*Selected)->Recipe), Recipe->Minerals,
                        Stock ? FText::Format(INVTEXT("{0} t"), FText::AsNumber(Stock->GetTonnes(TEXT("Ore Storage"))))
                              : INVTEXT("NO LIVE INVENTORY"));
                }))
            ]
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [
                FactoryColumn(INVTEXT("OUTPUT"), TAttribute<FText>::CreateLambda([Selected, RecipeLabels]()
                {
                    if (!*Selected) return FText::GetEmpty();
                    const auto* Recipe = RecipeLabels->Find((*Selected)->Recipe);
                    if (!Recipe) return INVTEXT("NO PRODUCT ASSIGNED");
                    return FText::Format(INVTEXT("{0}\n\nBATCH TIME\n{1}"), Recipe->Product, Recipe->ProcessingTime);
                }))
            ]
            + SHorizontalBox::Slot().FillWidth(1)
            [
                FactoryColumn(INVTEXT("ACTIVE JOBS"), TAttribute<FText>::CreateLambda([Selected]()
                {
                    if (!*Selected) return FText::GetEmpty();
                    const auto* Stock = GetLiveStock(**Selected);
                    if (!Stock || Stock->RemainingSeconds <= 0) return GetProductionStatus(Stock);
                    return FText::Format(INVTEXT("PRODUCING\n\n{0} s remaining"),
                        FText::AsNumber(FMath::CeilToInt(Stock->RemainingSeconds)));
                }))
            ]
        ];
    Screens->AddSlot()[Browser];
    Screens->AddSlot()[Factory];
    BlueprintBrowser = CreateWidget<UBlueprintsDlg>(GetOwningPlayer());
    if (BlueprintBrowser) Screens->AddSlot()[BlueprintBrowser->TakeWidget()];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(8)
        [SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
            [SNew(SButton).ToolTipText(INVTEXT("Production facilities"))
                .OnClicked_Lambda([Weak=TWeakPtr<SWidgetSwitcher>(Screens)]() { if (auto View=Weak.Pin()) View->SetActiveWidgetIndex(0); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Production)]]
            + SHorizontalBox::Slot().AutoWidth()
            [SNew(SButton).ToolTipText(INVTEXT("NPC blueprint catalogue and your blueprint library"))
                .OnClicked_Lambda([Weak=TWeakPtr<SWidgetSwitcher>(Screens)]() { if (auto View=Weak.Pin(); View && View->GetNumWidgets()>2) View->SetActiveWidgetIndex(2); return FReply::Handled(); })
                [SNew(SShipSystemIcon).Icon(EShipSystemIcon::Reference)]]]
        + SVerticalBox::Slot().FillHeight(1)[Screens];
}
