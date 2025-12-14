// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class RdGameServerTarget : TargetRules
{
	public RdGameServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4; // 사용자의 엔진 버전에 맞게 조정 (여기선 5.4 가정, 다르면 수정 필요할 수 있음)
		ExtraModuleNames.Add("RdGame");
	}
}
