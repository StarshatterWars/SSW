#include "ProductionDlg.h"

#include "GameStructs_products.h"
#include "MissionUIStyle.h"
#include "Sim.h"
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
};

class SProductionRow : public SMultiColumnTableRow<FFacilityPtr>
{
public:
    SLATE_BEGIN_ARGS(SProductionRow) {}
        SLATE_ARGUMENT(FFacilityPtr, Facility)
        SLATE_ARGUMENT(FText, Minerals)
        SLATE_ARGUMENT(FText, Product)
        SLATE_ARGUMENT(FText, ProcessingTime)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args, const TSharedRef<STableViewBase>& Owner)
    {
        Facility = Args._Facility;
        Minerals = Args._Minerals;
        Product = Args._Product;
        ProcessingTime = Args._ProcessingTime;
        SMultiColumnTableRow<FFacilityPtr>::Construct(FSuperRowType::FArguments(), Owner);
    }

    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column) override
    {
        if (Column == "Status")
            return SNew(STextBlock).Margin(FMargin(5, 4)).Font(MissionUIStyle::GetRowFont(13))
                .Text_Lambda([Site = Facility]()
                {
                    Sim* Simulation = Sim::GetSim();
                    const auto* Stock = Simulation ? Simulation->GetProductionStock(*Site) : nullptr;
                    if (!Stock) return INVTEXT("NO LIVE DATA");
                    if (Stock->RemainingSeconds > 0) return INVTEXT("PRODUCING");
                    if (Stock->GetTonnes(FName(TEXT("Finished Goods Storage"))) > 0) return INVTEXT("OUTPUT READY");
                    return INVTEXT("AWAITING ORE");
                });

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
        return SNew(STextBlock).Margin(FMargin(5, 4)).Font(MissionUIStyle::GetRowFont(13))
            .Text(Label.ToUpper()).ToolTipText(Label).OverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }

private:
    FFacilityPtr Facility;
    FText Minerals;
    FText Product;
    FText ProcessingTime;
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
                RecipeLabels->Add(Name, FRecipeLabels{
                    FText::Format(INVTEXT("{0} t: {1}"), FText::AsNumber(Recipe.InputTonnes),
                        FText::Join(INVTEXT(", "), Minerals)),
                    Recipe.CargoItem.IsEmpty()
                        ? FText::Format(INVTEXT("{0} t: {1}"), FText::AsNumber(Recipe.OutputTonnes),
                            StaticEnum<ESSWFinishedItem>()->GetDisplayNameTextByValue(int64(Recipe.Product)))
                        : FText::Format(INVTEXT("{0} {1}: {2}"), FText::AsNumber(Recipe.OutputQuantity),
                            FText::FromString(Recipe.OutputUnit), FText::FromString(Recipe.OutputDisplayName)),
                    FText::Format(INVTEXT("{0} s"), FText::AsNumber(Recipe.SecondsPerBatch))
                });
            });

    auto Header = SNew(SHeaderRow);
    const auto Column = [&Header](FName Id, FText Label, float Weight)
    {
        Header->AddColumn(SHeaderRow::Column(Id).DefaultLabel(Label).FillWidth(Weight)
            [SNew(STextBlock).Font(MissionUIStyle::GetRowFont(13)).Text(Label)]);
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

    return SNew(SListView<FFacilityPtr>).ListItemsSource(&*Rows).HeaderRow(Header)
        .SelectionMode(ESelectionMode::None)
        .OnGenerateRow_Lambda([Rows, RecipeLabels](FFacilityPtr Site, const TSharedRef<STableViewBase>& Owner)
            -> TSharedRef<ITableRow>
        {
            const FRecipeLabels* Recipe = RecipeLabels->Find(Site->Recipe);
            return SNew(SProductionRow, Owner).Facility(Site)
                .Minerals(Recipe ? Recipe->Minerals : INVTEXT("UNASSIGNED"))
                .Product(Recipe ? Recipe->Product : INVTEXT("UNASSIGNED"))
                .ProcessingTime(Recipe ? Recipe->ProcessingTime : INVTEXT("—"));
        });
}
