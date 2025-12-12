#include "DungeonFullTestActorDetails.h"
#include "DungeonFullTestActor.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateColor.h"

// static 멤버 변수 정의
EDungeonCategoryFilter FDungeonFullTestActorDetails::CurrentFilter = EDungeonCategoryFilter::All;

#define LOCTEXT_NAMESPACE "DungeonFullTestActorDetails"

TSharedRef<IDetailCustomization> FDungeonFullTestActorDetails::MakeInstance()
{
	return MakeShareable(new FDungeonFullTestActorDetails);
}

void FDungeonFullTestActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedDetailBuilder = &DetailBuilder;

	// Get the selected actor
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	if (SelectedObjects.Num() == 1)
	{
		SelectedActor = Cast<ADungeonFullTestActor>(SelectedObjects[0].Get());
	}

	// --- 카테고리 필터 버튼 섹션 ---
	IDetailCategoryBuilder& FilterCategory = DetailBuilder.EditCategory("Dungeon Filter", 
		LOCTEXT("DungeonFilter", "Dungeon Filter"), ECategoryPriority::Important);

	FilterCategory.AddCustomRow(LOCTEXT("CategoryFilterRow", "Category Filter"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterAll", "전체"), EDungeonCategoryFilter::All)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterCore", "Core"), EDungeonCategoryFilter::Core)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterGrid", "Grid"), EDungeonCategoryFilter::Grid)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterGeneration", "생성"), EDungeonCategoryFilter::Generation)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterLOD", "LOD"), EDungeonCategoryFilter::LOD)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterAssets", "에셋"), EDungeonCategoryFilter::Assets)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterChunking", "청킹"), EDungeonCategoryFilter::Chunking)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				CreateFilterButton(LOCTEXT("FilterStreaming", "스트리밍"), EDungeonCategoryFilter::Streaming)
			]
		];

	// --- Action Buttons ---
	IDetailCategoryBuilder& ActionCategory = DetailBuilder.EditCategory("Dungeon Action",
		LOCTEXT("DungeonAction", "Dungeon Action"), ECategoryPriority::Important);

	if (SelectedActor.IsValid())
	{
		ActionCategory.AddCustomRow(LOCTEXT("GenerateButton", "Generate"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GenerateApprove", "Controls"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			.MinDesiredWidth(200)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(2.0f)
				[
					SNew(SButton)
					.OnClicked(this, &FDungeonFullTestActorDetails::OnGenerateClicked)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Generate", "Generate"))
						.Justification(ETextJustify::Center)
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(2.0f)
				[
					SNew(SButton)
					.OnClicked(this, &FDungeonFullTestActorDetails::OnClearClicked)
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Clear", "Clear"))
						.Justification(ETextJustify::Center)
					]
				]
			];
	}

	// --- 카테고리 가시성 설정 ---
	// 필터에 따라 카테고리 표시/숨김

	// 모든 던전 관련 카테고리
	TArray<FName> CoreCategories = { TEXT("Dungeon Core") };
	TArray<FName> GridCategories = { TEXT("Dungeon Metrics"), TEXT("Dungeon Offsets") };
	TArray<FName> GenerationCategories = { TEXT("Dungeon Generation") };
	TArray<FName> LODCategories = { TEXT("Dungeon LOD") };
	TArray<FName> AssetCategories = { TEXT("Dungeon Assets"), TEXT("Dungeon Materials") };
	TArray<FName> ChunkingCategories = { TEXT("Dungeon Chunking"), TEXT("Dungeon Mesh Merging") };
	TArray<FName> StreamingCategories = { TEXT("Dungeon Streaming") };

	// 카테고리 숨김 헬퍼 - HideCategory만 호출하여 성능 최적화
	auto HideCategories = [&DetailBuilder](const TArray<FName>& Categories)
	{
		for (const FName& CategoryName : Categories)
		{
			DetailBuilder.HideCategory(CategoryName);
		}
	};

	// 필터에 따라 카테고리 숨기기
	switch (CurrentFilter)
	{
	case EDungeonCategoryFilter::All:
		// 모두 표시 (기본값이므로 아무것도 숨기지 않음)
		break;
	case EDungeonCategoryFilter::Core:
		HideCategories(GridCategories);
		HideCategories(GenerationCategories);
		HideCategories(LODCategories);
		HideCategories(AssetCategories);
		HideCategories(ChunkingCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::Grid:
		HideCategories(CoreCategories);
		HideCategories(GenerationCategories);
		HideCategories(LODCategories);
		HideCategories(AssetCategories);
		HideCategories(ChunkingCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::Generation:
		HideCategories(CoreCategories);
		HideCategories(GridCategories);
		HideCategories(LODCategories);
		HideCategories(AssetCategories);
		HideCategories(ChunkingCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::LOD:
		HideCategories(CoreCategories);
		HideCategories(GridCategories);
		HideCategories(GenerationCategories);
		HideCategories(AssetCategories);
		HideCategories(ChunkingCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::Assets:
		HideCategories(CoreCategories);
		HideCategories(GridCategories);
		HideCategories(GenerationCategories);
		HideCategories(LODCategories);
		HideCategories(ChunkingCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::Chunking:
		HideCategories(CoreCategories);
		HideCategories(GridCategories);
		HideCategories(GenerationCategories);
		HideCategories(LODCategories);
		HideCategories(AssetCategories);
		HideCategories(StreamingCategories);
		break;
	case EDungeonCategoryFilter::Streaming:
		HideCategories(CoreCategories);
		HideCategories(GridCategories);
		HideCategories(GenerationCategories);
		HideCategories(LODCategories);
		HideCategories(AssetCategories);
		HideCategories(ChunkingCategories);
		break;
	}
}

TSharedRef<SWidget> FDungeonFullTestActorDetails::CreateFilterButton(const FText& Label, EDungeonCategoryFilter Filter)
{
	// 현재 필터인지 확인하여 스타일 변경
	const bool bIsSelected = (CurrentFilter == Filter);

	return SNew(SButton)
		.OnClicked(this, &FDungeonFullTestActorDetails::OnCategoryFilterClicked, Filter)
		.ButtonColorAndOpacity(this, &FDungeonFullTestActorDetails::GetFilterButtonColor, Filter)
		[
			SNew(STextBlock)
			.Text(Label)
			.Justification(ETextJustify::Center)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
}

FSlateColor FDungeonFullTestActorDetails::GetFilterButtonColor(EDungeonCategoryFilter Filter) const
{
	if (CurrentFilter == Filter)
	{
		// 선택된 버튼 - 파란색 강조
		return FSlateColor(FLinearColor(0.2f, 0.5f, 1.0f, 1.0f));
	}
	// 기본 버튼 색상
	return FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f));
}

FReply FDungeonFullTestActorDetails::OnCategoryFilterClicked(EDungeonCategoryFilter Filter)
{
	if (CurrentFilter != Filter)
	{
		CurrentFilter = Filter;
		
		// 디테일 패널 새로고침
		if (CachedDetailBuilder)
		{
			CachedDetailBuilder->ForceRefreshDetails();
		}
	}
	return FReply::Handled();
}

FReply FDungeonFullTestActorDetails::OnGenerateClicked()
{
	if (SelectedActor.IsValid())
	{
		SelectedActor->Generate();
	}
	return FReply::Handled();
}

FReply FDungeonFullTestActorDetails::OnClearClicked()
{
	if (SelectedActor.IsValid())
	{
		SelectedActor->Clear();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
