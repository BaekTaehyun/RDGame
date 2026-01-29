import unreal
import json
import os

def scan_assets_lightweight(path="/Game", recursive=True):
    """
    에셋을 로드하지 않고(Hard Load 방지), 레지스트리 정보만 빠르게 긁어옵니다.
    """
    # 1. 에셋 레지스트리 시스템 가져오기
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    
    # 2. 필터 설정 (스태틱 메시만 검색)
    ar_filter = unreal.ARFilter(
        package_paths=[path],
        recursive_paths=recursive,
        class_names=["StaticMesh"] 
    )
    
    # 3. 에셋 데이터 가져오기 (이 단계는 매우 빠름)
    asset_data_list = asset_registry.get_assets(ar_filter)
    
    scanned_results = []
    
    for data in asset_data_list:
        # 4. 메타데이터(Tag) 추출
        asset_info = {
            "name": str(data.asset_name),
            "path": str(data.package_name),
            "class": str(data.asset_class_path.asset_name),
            "metadata": {}
        }
        
        # 핵심 태그값 추출
        # 'Triangles', 'Vertices' 등의 정보는 여기서 나옵니다.
        tags_to_check = ['Triangles', 'Vertices', 'LODs', 'Materials', 'ApproxSize']
        
        for tag in tags_to_check:
            value = data.get_tag_value(tag)
            if value:
                asset_info["metadata"][tag] = value
                
        # 5. 크기(Dimension) 정보 해석
        size_str = data.get_tag_value('ApproxSize')
        if size_str:
            asset_info["metadata"]["Size_Str"] = size_str

        scanned_results.append(asset_info)
        
    return scanned_results

if __name__ == "__main__":
    # --- 실행 테스트 ---
    print(">>> Scanning Assets in /Game...")
    try:
        results = scan_assets_lightweight("/Game") 
        print(f">>> Found {len(results)} assets.")
        
        # 결과 3개만 출력
        if len(results) > 0:
            print(json.dumps(results[:3], indent=2))
        else:
            print("No StaticMesh assets found in /Game")
            
    except Exception as e:
        print(f"Error: {e}")
