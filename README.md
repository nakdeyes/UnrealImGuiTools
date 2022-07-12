# UnrealImGuiTools
A set of tools and utilities for use with Unreal Engine projects using ImGui.

# Description
This plugin includes tools for use with Unreal Engine projects. It also provides a small framework to make writing your own game specific tool windows a bit more quickly and easily.

The plugin itself does not contain ImGui, but has [segross' UnrealImGui plugin](https://github.com/segross/UnrealImGui) as a dependant plugin. This plugin, segross' plugin, and [Omar Cornut's fantastic Dear ImGui](https://github.com/ocornut/imgui) are all released under MIT license.

### Compatibility
Most of these tools were written with UE 4.27, then it was later updated to UE 5.0 (it was a very minimal change). So currently only UE 5.0 is currently supported, but I plan to formalize 4.27 support with a flag or something in the future. For now, if you drop this in 4.27 it will still mostly work with perhaps 1 or 2 tweaks.

# Controlling ImGui Tools and ImGui
### Editor Button
To increase discoverability and ease of use, there is an editor button menu that provides options for interacting with both ImGui Tools plugin and the ImGui plugin itself. Find it in the main editor toolbar:
<img width="827" alt="image" src="https://user-images.githubusercontent.com/15803559/178165495-6f463492-0ca9-4f76-987a-2b66294df0ff.png">

Here you will find:
 * Top level ImGui Tools enable switch
 * Buttons to enable / disable individual tools
 * Buttons to enable different Input features of the ImGui plugin

### CVars
When outside the editor (like in Standalone builds) the main way to interact with the plugin is via CVars. Here is a list of useful ones:
```imgui.tools.enabled true```

<!-- Generated at https://www.tablesgenerator.com/markdown_tables# -->
| CVAR                                                 | Example                                                                                   | Description                                                                                                                                                                                                                                                             |
|------------------------------------------------------|-------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ```imgui.tools.enabled <bool: enable>```             | ```imgui.tools.enabled true imgui.tools.enabled false```                                  | Top-level switch that will enable / disable all ImGui Tools.                                                                                                                                                                                                            |
| ```imgui.tools.toggle_tool_vis <string:tool name>``` | ```imgui.tools.toggle_tool_vis MemoryDebugger imgui.tools.toggle_tool_vis LoadDebugger``` | Toggle visibility of a particular imgui tool. This list auto-completes with all registered tools, so it is a good way to explore all registered tools. If the top level switch is disabled when this CVAR is executed, the top level switch will be toggled on as well. |
| ```imgui.tools.file_load.toggle_record```            | ```imgui.tools.file_load.toggle_record```                                                 | For the Load Debugger - toggle recording of UE file loads.                                                                                                                                                                                                              |
 
![image](https://user-images.githubusercontent.com/15803559/178166803-b6f8494c-2fbd-49ce-8f98-a85c94417487.png)

### Creating game level ImGui Tool windows
Create your own ImGui tool windows in your game module! 

There is example game module code in the ```ExampleGameCode``` directory that will show you how to register tools with the plugin.

***Question:*** Why the heck should I pipe my game ImGui tools through the plugin? 

***Answer:*** Really, it just gives you a nice ImGui drawing entry point, an editor button, in-game menu button, and a CVAR via ```imgui.tools.toggle_tool_vis``` to toggle the visibility of your tool. Boiler plate that you might find yourself doing often when writing bespoke ImGui tool windows.

# Included Tools
## Memory Debugger
The Memory Debugger was built to show detailed memory information at runtime. While Memory Insights is largely useful for debugging individual allocations and some high level data, and the STAT commands can show you some useful buckets at a glance, this memory debugger is meant to fit somewhere in the middle. It is meant to compliment the existing memory debugging tools. There are a few parts to the Memory Debugger
#### Platform Memory Stats
These are real-time top level memory stats.

![image](https://user-images.githubusercontent.com/15803559/178167968-c0cb0aae-16e0-4eb5-a5f6-b11e3c81dc86.png)

#### Texture Memory
These are real-time stats for the textures currently loaded into the texture streaming system. This is useful to sanity check texture sizes at a glance, see which textures are marked for streaming or not, see usage counts and more! 

![image](https://user-images.githubusercontent.com/15803559/178168099-097a906b-3357-4cc4-b15f-282eda38b8c4.png)

#### Object Memory
This one is super cool! This will show you the memory size of all loaded UObjects in a cool heirarchical view. However, it is not real-time but instead 'snapshot' based. When you are ready to inspect your memory press the "Update (SLOW!)' button and give it ~10 seconds while the memory data is gathered. Then you can search through it by name, sort it by size / instances, and more! You can 'Inspect' any UClass to show a dedicated window for instances of that class (with an option to update those in real time). 

***NOTE: UObjects must implement UObject::GetResourceSizeEx() for the tool to correctly gather resource sizes. This is implemented for 99% of Epic UObjects (but watch out for the occasional exception!), but may require closer inspection for some third party libs/plugins (this had to be implemented manually for WWISE UObjects for instance)***

![image](https://user-images.githubusercontent.com/15803559/178168475-9c4a678b-17ba-48e1-bbef-f4cb29c419a6.png)
![image](https://user-images.githubusercontent.com/15803559/178168651-b0a98998-bfd0-443b-a741-80d58d95fdb1.png)

## Load Debugger
Synchronous loading hitches got you down? We have you covered! The LoadDebugger is a very small and simple tools that can track all sync and async class loads that happen during gameplay. Simply enable recording, spew the loads to the log output if desired, and clear the load list when needed. This is a great tool for hunting down unintentional / unknown sync file loads to eliminate those last hitches before ship by enabling recording and playing through the game a few times.

<img width="386" alt="image" src="https://user-images.githubusercontent.com/15803559/178175432-220c778f-de33-4e62-8c31-171293352c4b.png">

## Actor Component Debugger (ALPHA)
***NOTE: This tool is very much in progress and maybe only 50% complete. But perhaps is already a tiny bit useful***

This is a tool meant to help explore, analyze, and debug AActors and UActorComponents ( and more! ). It provides an in-game actor and component explorer and shows you useful information like counts of actors and components that replicate or tick, all represented in a heirarchical class view. You can tear off a window for any given actor or component to inspect it more closely. 

***Coming soon: advanced text search and sorting options a'la the memory debugger, and a mechanism to provide custom debug per actor or component class***

<img width="886" alt="image" src="https://user-images.githubusercontent.com/15803559/178176100-98cb1172-1f25-46d7-adc3-e4acd3dcbaf7.png">
