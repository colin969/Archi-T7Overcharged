[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## Useful Links

- AP World: https://github.com/colin969/Call-of-Duty-BO3-Zombies-Archipelago
- BO3 Mod: https://github.com/colin969/bo3_archipelago
- BO3 APClient DLL: https://github.com/colin969/Archi-T7Overcharged

# Archi-T7 Overcharged

This is a custom fork of T7Overcharged to include [apclientpp](https://github.com/black-sliver/apclientpp) inside Call of Duty Black Ops 3. This is to enable the Black Ops 3 Zombies AP Mod. In addition it also removes components of T7Overcharged that are not used in AP.

Current Target: AP 0.6.5

# Build Instructions (Windows x64)

- Prerequisities: Visual Studio, v143 build tools, ATL for v143 component, vcpkg

1. Clone repository
2. Run `./generate.bat` to generate a solution inside `build`
3. Run `vcpkg install --triplet=x64-windows-static` to set up openssl and zlib requirements
4. Open solution in Visual Studio (do not upgrade build targets)
5. Build the solution, enjoy your Archi-T7Overcharged.dll

## License 
As with T7Overcharged, this is liscenced under GLP3 (linked above). 



### The Original Readme.MD for T7Overcharged is included below for reference. 

# T7Overcharged

**T7Overcharged** gives Call of Duty: Black Ops 3 modders the ability to take their mods to the next level by providing an API between Lua and C++ and providing already worked out components.

## Features
- Hot reloading of lua scripts
- UI Errors now give a stack trace and don't cause a freeze
- Extending the asset limits
- External console and printing to the console from lua
- HTTP requests from lua
- Discord Rich Presence
- Bullet depletion
- More to come!

## How to use
- Clone this repo or download the latest release.
- Copy the lua files to the mod/map you want to use this in.
- Copy the dll to the zone folder
- Add the following lines to your zone file:
```
rawfile,ui/util/T7Overcharged.lua
rawfile,ui/util/T7OverchargedUtil.lua
```
- Add the following lines in your main lua file:
```
require("ui.util.T7Overcharged")
```
- Under that add the following code but change variables to your map/mod's information

for a mod:
```
InitializeT7Overcharged({
	modname = "t7overcharged",
	filespath = [[.\mods\t7overcharged\]],
	workshopid = "45131545",
	discordAppId = nil,--"{{DISCORD_APP_ID}}" -- Not required, create your application at https://discord.com/developers/applications/
	showExternalConsole = true
})
```
for a map:
```
InitializeT7Overcharged({
	mapname = "zm_t7overcharged",
	filespath = [[.\usermaps\zm_t7overcharged\]],
	workshopid = "45131545",
	discordAppId = nil,--"{{DISCORD_APP_ID}}" -- Not required, create your application at https://discord.com/developers/applications/
	showExternalConsole = true
})
```
The [dvar hash list](usage/dvar_hash_list.txt) as of now isn't shipped with the DLL, it has to be manually placed inside of the root folder of Black Ops 3.


## Download

The latest version can be found on the [Releases Page](https://github.com/JariKCoding/T7Overcharged/releases).

## Compile from source

- Clone the Git repo. Do NOT download it as ZIP, that won't work.
- Update the submodules and run `premake5 vs2022` or simply use the delivered `generate.bat`.
- Build via solution file in `build\T7Overcharged.sln`.

## Credits

- [XLabsProjects](https://github.com/XLabsProject): Great project setup
- [Approved](https://github.com/approved): Asset limit extension
- [Serious](https://github.com/shiversoftdev): UI Error hash dvar

## License 

T7Overcharged is licensed under the GNUv3 license and its source code is free to use and modify. T7Overcharged comes with NO warranty, any damages caused are solely the responsibility of the user. See the LICENSE file for more information.