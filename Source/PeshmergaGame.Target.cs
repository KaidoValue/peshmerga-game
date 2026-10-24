using UnrealBuildTool;

public class PeshmergaGameTarget : TargetRules
{
	public PeshmergaGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PeshmergaGame");
	}
}
