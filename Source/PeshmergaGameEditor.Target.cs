using UnrealBuildTool;

public class PeshmergaGameEditorTarget : TargetRules
{
	public PeshmergaGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PeshmergaGame");
	}
}
