mkdir Bin\
copy Dependencies\dll\* Bin\
call "Premake/premake5" --defaultproject=Platformer_with_Editor --file=Source/Examples/workspace.lua vs2022
pause