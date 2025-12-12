#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "DungeonFullTestActor.h"

// 카테고리 필터 타입
enum class EDungeonCategoryFilter : uint8
{
	All,        // 전체
	Core,       // Dungeon Core
	Grid,       // Dungeon Metrics + Offsets
	Generation, // Dungeon Generation
	LOD,        // Dungeon LOD
	Assets,     // Dungeon Assets + Materials
	Chunking,   // Dungeon Chunking
	Streaming   // Dungeon Streaming (Phase 4)
};

class FDungeonFullTestActorDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Callback for generating the dungeon */
	FReply OnGenerateClicked();

	/** Callback for clearing the dungeon */
	FReply OnClearClicked();

	/** 카테고리 필터 버튼 클릭 콜백 */
	FReply OnCategoryFilterClicked(EDungeonCategoryFilter Filter);

	/** 현재 카테고리 필터에 따른 버튼 스타일 반환 */
	FSlateColor GetFilterButtonColor(EDungeonCategoryFilter Filter) const;

	/** 카테고리 필터 버튼 생성 */
	TSharedRef<class SWidget> CreateFilterButton(const FText& Label, EDungeonCategoryFilter Filter);

	/** The actor we are customizing */
	TWeakObjectPtr<ADungeonFullTestActor> SelectedActor;

	/** 현재 선택된 카테고리 필터 (static으로 인스턴스 간 유지) */
	static EDungeonCategoryFilter CurrentFilter;

	/** DetailBuilder 참조 (필터 변경 시 새로고침용) */
	IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};
