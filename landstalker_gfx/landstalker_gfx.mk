##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=landstalker_gfx
ConfigurationName      :=Release
WorkspacePath          :=C:/Users/Tom/Documents/landstalker_gfx
ProjectPath            :=C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Tom
Date                   :=03/07/2016
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=C:/MinGW/bin/g++.exe
SharedObjectLinkerName :=C:/MinGW/bin/g++.exe -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="landstalker_gfx.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := $(shell wx-config --rcflags)
RcCompilerName         :=C:/MinGW/bin/windres.exe
LinkOptions            :=  $(shell wx-config --libs) -mwindows
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := C:/MinGW/bin/ar.exe rcu
CXX      := C:/MinGW/bin/g++.exe
CC       := C:/MinGW/bin/gcc.exe
CXXFLAGS :=  -O2 -Wall $(shell wx-config --cflags) $(Preprocessors)
CFLAGS   :=  -O2 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := C:/MinGW/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
WXCFG:=gcc_dll\mswu
WXWIN:=C:\wxWidgets-3.1.0
Objects0=$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/MainFrame.cpp$(ObjectSuffix) $(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) $(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) $(IntermediateDirectory)/LZ77.cpp$(ObjectSuffix) $(IntermediateDirectory)/BigTilesCmp.cpp$(ObjectSuffix) $(IntermediateDirectory)/BitBarrel.cpp$(ObjectSuffix) $(IntermediateDirectory)/BitBarrelWriter.cpp$(ObjectSuffix) $(IntermediateDirectory)/Tile.cpp$(ObjectSuffix) $(IntermediateDirectory)/TileAttributes.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/BigTile.cpp$(ObjectSuffix) $(IntermediateDirectory)/TileBitmap.cpp$(ObjectSuffix) $(IntermediateDirectory)/Palette.cpp$(ObjectSuffix) $(IntermediateDirectory)/LSTilemapCmp.cpp$(ObjectSuffix) $(IntermediateDirectory)/win_resources.rc$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@$(MakeDirCommand) "./Release"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "./Release"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM "main.cpp"

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) "main.cpp"

$(IntermediateDirectory)/MainFrame.cpp$(ObjectSuffix): MainFrame.cpp $(IntermediateDirectory)/MainFrame.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/MainFrame.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/MainFrame.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/MainFrame.cpp$(DependSuffix): MainFrame.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/MainFrame.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/MainFrame.cpp$(DependSuffix) -MM "MainFrame.cpp"

$(IntermediateDirectory)/MainFrame.cpp$(PreprocessSuffix): MainFrame.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/MainFrame.cpp$(PreprocessSuffix) "MainFrame.cpp"

$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix): wxcrafter.cpp $(IntermediateDirectory)/wxcrafter.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/wxcrafter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wxcrafter.cpp$(DependSuffix): wxcrafter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wxcrafter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/wxcrafter.cpp$(DependSuffix) -MM "wxcrafter.cpp"

$(IntermediateDirectory)/wxcrafter.cpp$(PreprocessSuffix): wxcrafter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wxcrafter.cpp$(PreprocessSuffix) "wxcrafter.cpp"

$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix): wxcrafter_bitmaps.cpp $(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/wxcrafter_bitmaps.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(DependSuffix): wxcrafter_bitmaps.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(DependSuffix) -MM "wxcrafter_bitmaps.cpp"

$(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(PreprocessSuffix): wxcrafter_bitmaps.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/wxcrafter_bitmaps.cpp$(PreprocessSuffix) "wxcrafter_bitmaps.cpp"

$(IntermediateDirectory)/LZ77.cpp$(ObjectSuffix): LZ77.cpp $(IntermediateDirectory)/LZ77.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/LZ77.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/LZ77.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/LZ77.cpp$(DependSuffix): LZ77.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/LZ77.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/LZ77.cpp$(DependSuffix) -MM "LZ77.cpp"

$(IntermediateDirectory)/LZ77.cpp$(PreprocessSuffix): LZ77.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/LZ77.cpp$(PreprocessSuffix) "LZ77.cpp"

$(IntermediateDirectory)/BigTilesCmp.cpp$(ObjectSuffix): BigTilesCmp.cpp $(IntermediateDirectory)/BigTilesCmp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/BigTilesCmp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/BigTilesCmp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/BigTilesCmp.cpp$(DependSuffix): BigTilesCmp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/BigTilesCmp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/BigTilesCmp.cpp$(DependSuffix) -MM "BigTilesCmp.cpp"

$(IntermediateDirectory)/BigTilesCmp.cpp$(PreprocessSuffix): BigTilesCmp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/BigTilesCmp.cpp$(PreprocessSuffix) "BigTilesCmp.cpp"

$(IntermediateDirectory)/BitBarrel.cpp$(ObjectSuffix): BitBarrel.cpp $(IntermediateDirectory)/BitBarrel.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/BitBarrel.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/BitBarrel.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/BitBarrel.cpp$(DependSuffix): BitBarrel.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/BitBarrel.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/BitBarrel.cpp$(DependSuffix) -MM "BitBarrel.cpp"

$(IntermediateDirectory)/BitBarrel.cpp$(PreprocessSuffix): BitBarrel.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/BitBarrel.cpp$(PreprocessSuffix) "BitBarrel.cpp"

$(IntermediateDirectory)/BitBarrelWriter.cpp$(ObjectSuffix): BitBarrelWriter.cpp $(IntermediateDirectory)/BitBarrelWriter.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/BitBarrelWriter.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/BitBarrelWriter.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/BitBarrelWriter.cpp$(DependSuffix): BitBarrelWriter.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/BitBarrelWriter.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/BitBarrelWriter.cpp$(DependSuffix) -MM "BitBarrelWriter.cpp"

$(IntermediateDirectory)/BitBarrelWriter.cpp$(PreprocessSuffix): BitBarrelWriter.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/BitBarrelWriter.cpp$(PreprocessSuffix) "BitBarrelWriter.cpp"

$(IntermediateDirectory)/Tile.cpp$(ObjectSuffix): Tile.cpp $(IntermediateDirectory)/Tile.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/Tile.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Tile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Tile.cpp$(DependSuffix): Tile.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Tile.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Tile.cpp$(DependSuffix) -MM "Tile.cpp"

$(IntermediateDirectory)/Tile.cpp$(PreprocessSuffix): Tile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Tile.cpp$(PreprocessSuffix) "Tile.cpp"

$(IntermediateDirectory)/TileAttributes.cpp$(ObjectSuffix): TileAttributes.cpp $(IntermediateDirectory)/TileAttributes.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/TileAttributes.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/TileAttributes.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/TileAttributes.cpp$(DependSuffix): TileAttributes.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/TileAttributes.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/TileAttributes.cpp$(DependSuffix) -MM "TileAttributes.cpp"

$(IntermediateDirectory)/TileAttributes.cpp$(PreprocessSuffix): TileAttributes.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/TileAttributes.cpp$(PreprocessSuffix) "TileAttributes.cpp"

$(IntermediateDirectory)/BigTile.cpp$(ObjectSuffix): BigTile.cpp $(IntermediateDirectory)/BigTile.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/BigTile.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/BigTile.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/BigTile.cpp$(DependSuffix): BigTile.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/BigTile.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/BigTile.cpp$(DependSuffix) -MM "BigTile.cpp"

$(IntermediateDirectory)/BigTile.cpp$(PreprocessSuffix): BigTile.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/BigTile.cpp$(PreprocessSuffix) "BigTile.cpp"

$(IntermediateDirectory)/TileBitmap.cpp$(ObjectSuffix): TileBitmap.cpp $(IntermediateDirectory)/TileBitmap.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/TileBitmap.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/TileBitmap.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/TileBitmap.cpp$(DependSuffix): TileBitmap.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/TileBitmap.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/TileBitmap.cpp$(DependSuffix) -MM "TileBitmap.cpp"

$(IntermediateDirectory)/TileBitmap.cpp$(PreprocessSuffix): TileBitmap.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/TileBitmap.cpp$(PreprocessSuffix) "TileBitmap.cpp"

$(IntermediateDirectory)/Palette.cpp$(ObjectSuffix): Palette.cpp $(IntermediateDirectory)/Palette.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/Palette.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Palette.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Palette.cpp$(DependSuffix): Palette.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Palette.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Palette.cpp$(DependSuffix) -MM "Palette.cpp"

$(IntermediateDirectory)/Palette.cpp$(PreprocessSuffix): Palette.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Palette.cpp$(PreprocessSuffix) "Palette.cpp"

$(IntermediateDirectory)/LSTilemapCmp.cpp$(ObjectSuffix): LSTilemapCmp.cpp $(IntermediateDirectory)/LSTilemapCmp.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/LSTilemapCmp.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/LSTilemapCmp.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/LSTilemapCmp.cpp$(DependSuffix): LSTilemapCmp.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/LSTilemapCmp.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/LSTilemapCmp.cpp$(DependSuffix) -MM "LSTilemapCmp.cpp"

$(IntermediateDirectory)/LSTilemapCmp.cpp$(PreprocessSuffix): LSTilemapCmp.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/LSTilemapCmp.cpp$(PreprocessSuffix) "LSTilemapCmp.cpp"

$(IntermediateDirectory)/win_resources.rc$(ObjectSuffix): win_resources.rc
	$(RcCompilerName) -i "C:/Users/Tom/Documents/GitHub/landstalker_gfx/landstalker_gfx/win_resources.rc" $(RcCmpOptions)   $(ObjectSwitch)$(IntermediateDirectory)/win_resources.rc$(ObjectSuffix) $(RcIncludePath)

-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


