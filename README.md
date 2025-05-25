# MapWoader
Small soon-to-be map editor in MS2 based on OpenGL.
EXTREMELY WIP at the moment, doesn't even function and the way to load things is extremely unorthodox.

mettaur the goat

## Instructions
1. Download/clone repo
2. Choose whatever map .xblock you want to use and put it in /resources/, then rename it to map.xblock
3. Extract "Textures.m2d" contents and put contents in /resources/textures if a textures folder doesn't exist make one, so when you enter the "textures" folder you should be able to see "cave", "common", "effect" etc.
4. Build, run the .exe, enjoy buggy mess

## To-do
- Load maps + textures directly from the client using an .ini as a reference to them
- Fix block selecting (selects a block from like 5 miles away, maybe ray or camera related?)
- Make it so you can move/modify blocks, which updates the .xblock file and always you to save it
- Imgui to display all the maps you can pick from a list, maybe convert them to the mapname xml string + have a search bar?
