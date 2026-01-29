#include "Utils/DungeonAssetUtils.h"
#include "Algo/Reverse.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"
#include "Misc/Base64.h"
#include "ObjectTools.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UObject/SavePackage.h"

// PCG Includes
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"
#include "PCGEdge.h"
#include "PCGSettings.h"

TArray<uint8> UDungeonAssetUtils::CaptureThumbnail(FString AssetPath)
{
	// Resolve Asset
	UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Asset)
	{
		return TArray<uint8>();
	}

	// 1. Get Thumbnail Tools
	FThumbnailRenderingInfo* RenderInfo = UThumbnailManager::Get().GetRenderingInfo(Asset);
	if (!RenderInfo || !RenderInfo->Renderer)
	{
		return TArray<uint8>();
	}

	// 2. Generate Thumbnail via Editor API
	// Note: Direct capturing to TArray is tricky without private APIs. 
	// We often resort to ObjectTools or similar helpers.
	// For simplicity in this snippets, we try to use the cached thumbnail first.
	
	// Force generate
	ThumbnailTools::RenderThumbnail(Asset, 256, 256, ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, NULL);
	
	// This part is complex because RenderThumbnail renders to a texture in memory (slate).
	// A more robust way for "File" extraction:
	// Use FObjectThumbnail if stored in package? 
	
	// Let's rely on standard ThumbnailMap check
	FObjectThumbnail* Thumb = ThumbnailTools::GenerateThumbnailForObjectToSaveToDisk(Asset);
	if (Thumb && !Thumb->GetUncompressedImageData().IsEmpty())
	{
		return Thumb->GetUncompressedImageData();
	}

	return TArray<uint8>();
}

FString UDungeonAssetUtils::AnalyzePCGTopology(FString GraphPath)
{
	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph)
	{
		return TEXT("{\"Error\": \"Graph not found\"}");
	}

	FString Json = TEXT("{\"Nodes\": [");
    bool bFirstNode = true;

	for (const UPCGNode* Node : Graph->GetNodes())
	{
        if (!Node) continue;
        
        if (!bFirstNode) Json += TEXT(",");
        bFirstNode = false;

        FString NodeName = Node->GetName();
        FString NodeTitle = NodeName; // Fallback
        
        // Settings for Title
        if (const UPCGSettings* Settings = Node->GetSettings())
        {
#if WITH_EDITOR
            // GetNodeTitle might not be exposed or exists on Node instead
            // Fallback to simpler name
            NodeTitle = Settings->GetClass()->GetName();
#endif
        }

        Json += FString::Printf(TEXT("{\"Name\": \"%s\", \"Title\": \"%s\", \"Outbound\": ["), *NodeName, *NodeTitle);

        // Iterate Outputs
        bool bFirstEdge = true;
        for (const UPCGPin* Pin : Node->GetOutputPins())
        {
            if (!Pin) continue;
            
            // Edges
            // Note: In some PCG versions, Pin->Edges is not directly public?
            // Usually it is. If 'Edges' is private, we might need GetEdges().
            for (const UPCGEdge* Edge : Pin->Edges)
            {
                if (!Edge) continue;

                // Edge connects OutputPin (Upstream) -> InputPin (Downstream)
                // However, previous test showed InputPin pointed to the *Source* node.
                // Assuming PCGEdge flow is InputPin(Source) -> OutputPin(Target).
                
                if (const UPCGPin* TargetPin = Edge->OutputPin)
                {
                     if (const UPCGNode* TargetNode = TargetPin->Node)
                     {
                         if (!bFirstEdge) Json += TEXT(",");
                         bFirstEdge = false;
                         Json += FString::Printf(TEXT("\"%s\""), *TargetNode->GetName());
                     }
                }
            }
        }
        
        Json += TEXT("]}");
	}

	Json += TEXT("]}");
	return Json;
}

bool UDungeonAssetUtils::ConnectPCGNodes(FString GraphPath, FString UpstreamNodeName, FString DownstreamNodeName, FString UpPinLabel, FString DownPinLabel)
{
    UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectPCGNodes: Graph not found %s"), *GraphPath);
        return false;
    }

    UPCGNode* UpNode = nullptr;
    UPCGNode* DownNode = nullptr;

    // Helper to Match
    auto MatchNode = [&](UPCGNode* Node, FString SearchName) -> bool {
        if (!Node) return false;
        // 1. Name
        if (Node->GetName().Contains(SearchName)) return true;
        // 2. Title seems hard to get robustly in runtime without editor module deps sometimes
        // Try Settings Name
        if (const UPCGSettings* Settings = Node->GetSettings())
        {
             if (Settings->GetClass()->GetName().Contains(SearchName)) return true;
        }
        return false;
    };

    for (UPCGNode* Node : Graph->GetNodes())
    {
        // Try to match specific titles if possible, PCGNode usually has GetNodeTitle() in Editor builds
#if WITH_EDITOR
        FString Title = Node->GetNodeTitle(EPCGNodeTitleType::ListView).ToString();
        if (Title.Contains(UpstreamNodeName)) UpNode = Node;
        if (Title.Contains(DownstreamNodeName)) DownNode = Node;
#endif
        if (!UpNode && MatchNode(Node, UpstreamNodeName)) UpNode = Node;
        if (!DownNode && MatchNode(Node, DownstreamNodeName)) DownNode = Node;
    }

    if (!UpNode)
    {
         UE_LOG(LogTemp, Error, TEXT("ConnectPCGNodes: Upstream Node '%s' not found"), *UpstreamNodeName);
         return false;
    }
    if (!DownNode)
    {
         UE_LOG(LogTemp, Error, TEXT("ConnectPCGNodes: Downstream Node '%s' not found"), *DownstreamNodeName);
         return false;
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectPCGNodes: Attempting to connect %s (%s) -> %s (%s)"), *UpNode->GetName(), *UpPinLabel, *DownNode->GetName(), *DownPinLabel);
    
    // UE5 PCGGraph::AddEdge takes (FromNode, FromLabel, ToNode, ToLabel)
    // Convert FString labels to FName
    Graph->AddEdge(UpNode, FName(*UpPinLabel), DownNode, FName(*DownPinLabel));

    // Save
    // UE5 SavePackage uses FSavePackageArgs
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Standalone;
    SaveArgs.Error = GError;
    SaveArgs.SaveFlags = SAVE_NoError;
    
    FString PackageName = Graph->GetOutermost()->GetName();
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Graph->GetOutermost(), Graph, *PackageFileName, SaveArgs);
    
    return true;
}

#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"

void UDungeonAssetUtils::RefreshBlueprint(UObject* Asset)
{
    if (!Asset)
    {
        return;
    }

    // 1. Try PCG Graph (User Request)
    if (UPCGGraph* PCGGraph = Cast<UPCGGraph>(Asset))
    {
        PCGGraph->Modify();
        
        // Notify Editor that the graph structure changed
        FPropertyChangedEvent ChangeEvent(nullptr, EPropertyChangeType::Unspecified);
        PCGGraph->PostEditChangeProperty(ChangeEvent);
        
        PCGGraph->MarkPackageDirty();
        
        UE_LOG(LogTemp, Log, TEXT("RefreshBlueprint: Refreshed PCG Graph '%s'"), *PCGGraph->GetName());
        return;
    }

    // 2. Try Blueprint
    if (UBlueprint* BP = Cast<UBlueprint>(Asset))
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        UE_LOG(LogTemp, Log, TEXT("RefreshBlueprint: Marked BP '%s' as structurally modified."), *BP->GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("RefreshBlueprint: Asset '%s' is neither UPCGGraph nor UBlueprint."), *Asset->GetName());
}
