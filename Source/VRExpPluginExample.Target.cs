// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class VRExpPluginExampleTarget : TargetRules
{
	public VRExpPluginExampleTarget(TargetInfo Target) : base(Target)
	{

        DefaultBuildSettings = BuildSettingsVersion.V2;

        //bUseLoggingInShipping = true;
        Type = TargetType.Game;
        ExtraModuleNames.AddRange(new string[] { "VRExpPluginExample" });
        //bUsePCHFiles = false;
        //bUseUnityBuild = false;

        /*
         * This is our Steam App ID.
         * # Define in both server and client targets
         */
        ProjectDefinitions.Add("UE4_PROJECT_STEAMSHIPPINGID=480");



        /*
         * This is used on SetProduct(), and should be the same as your Product Name
         * under Dedicated Game Server Information in Steamworks
         * # Define in the Server target
         */
        //ProjectDefinitions.Add("UE4_PROJECT_STEAMPRODUCTNAME=\"MyGame\"");

        /*
         * This is used on SetModDir(), and should be the same as your Product Name
         * under Dedicated Game Server Information in Steamworks
         * # Define in the client target
         */
        //ProjectDefinitions.Add("UE4_PROJECT_STEAMGAMEDIR=\"MyGame\"");

        /*
         * This is what shows up under the game filter in Steam server browsers.
         * # Define in both server and client targets
         */
        //ProjectDefinitions.Add("UE4_PROJECT_STEAMGAMEDESC=\"My Game\"");
    }

    //
    // TargetRules interface.
    //

    /*public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.AddRange( new string[] { "VRExpPluginExample" } );
	}*/
}
