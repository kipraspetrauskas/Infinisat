using UnrealBuildTool;

public class CosmosLogic : ModuleRules
{
	public CosmosLogic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject", "Engine", "InputCore",
			"PhysicsCore", "ChaosVehiclesCore", "Niagara"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Slate", "SlateCore", "UMG", "ApplicationCore",
			"RHI", "RenderCore", "Projects", "Sockets", "Http", "Json", "SQLite3"
		});

		PublicIncludePaths.AddRange(new string[] {
			"CosmosLogic/CosmosLogic/Public",
		});

		PrivateIncludePaths.AddRange(new string[] {
			"CosmosLogic/CosmosLogic/Private",
		});
	}
}