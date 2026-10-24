using UnrealBuildTool;

public class PeshmergaGame : ModuleRules
{
	public PeshmergaGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"EnhancedInput", "UMG", "Slate", "SlateCore",
			"AIModule", "NavigationSystem", "Niagara"
		});
	}
}
