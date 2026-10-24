using UnrealBuildTool;

public class PeshmergaGameTarget : TargetRules
{
	public PeshmergaGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PeshmergaGame");
	}
}
