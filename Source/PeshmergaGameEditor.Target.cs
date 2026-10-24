using UnrealBuildTool;

public class PeshmergaGameEditorTarget : TargetRules
{
	public PeshmergaGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PeshmergaGame");
	}
}
