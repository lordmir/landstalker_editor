@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

SET STANDARD_BUILDOPTS="/p /d /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_"
SET AS="tools\build\asm68k.exe"
SET Z80AS="tools\build\asw\asw.exe"
SET Z80LINK="tools\build\asw\p2bin.exe"

SET Z80AS_DRV_OPTS=""
SET Z80LINK_DRV_OPTS="-l 0xff -r 0x0000-0x1f7f -k"
SET Z80DRV_SRC="code/audio/main.asm"
SET Z80DRV_OBJ="cube.p"
SET Z80DRV_BIN="cube.bin"
SET Z80AS_BANK3_OPTS=""
SET Z80LINK_BANK3_OPTS="-l 0xff -r 0x8000-0xdfff -k"
SET Z80BANK3_SRC="code/audio/soundbank3.asm"
SET Z80BANK3_OBJ="soundbank3.p"
SET Z80BANK3_BIN="soundbank3.bin"
SET Z80AS_BANK4_OPTS=""
SET Z80LINK_BANK4_OPTS="-l 0xff -r 0x8000-0xffff -k"
SET Z80BANK4_SRC="code/audio/soundbank4.asm"
SET Z80BANK4_OBJ="soundbank4.p"
SET Z80BANK4_BIN="soundbank4.bin"

SET REGION=US
SET EXPANDED=0
SET MANUAL_OUTPUT=

:PARSE
IF "%~1"=="-r" (
    SET REGION=%2
    SHIFT
    SHIFT
    GOTO PARSE
)
IF "%~1"=="-e" (
    SET /A EXPANDED=1
    SHIFT
    GOTO PARSE    
)
IF "%~1"=="-o" (
    SET MANUAL_OUTPUT=%~2
    SHIFT
    SHIFT
    GOTO PARSE    
)
IF "%~1"=="" (
    GOTO ENDPARSE
)
ECHO Usage: %~0% [-r ^(US^|JP^|EUR^|FR^|DE^|BETA^)] [-e] [-o <Output ROM Filename>]
EXIT 1

:ENDPARSE

SET SOURCE="landstalker_us.asm"
SET OUTPUT="landstalker_us.bin"
SET SYMBOL="landstalker_us.sym"
SET LISTING="landstalker_us.lst"
SET /A REGION_CODE=0
SET /A NTSC=1
SET /A REGION_CHECK=1
SET /A FIX_COLL_1=1
SET /A FIX_COLL_2=1
SET /A FIX_ARMLET_SKIP=1
SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
SET /A FIX_SPRITE_HIDE=1
SET /A ENABLE_GOLD_COUNT_ON_TREASURE=1
SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
SET /A FIX_GOLD_CAP_ON_FILE_LOAD=1
SET /A FIX_END_CREDS_GLITCH=0
SET /A REFRESH_GOLD_CTR=1
SET /A FIX_TS_GLITCH=1

:: Options
IF %REGION%==US (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_us_expanded.asm"
        SET OUTPUT="landstalker_us_expanded.bin"
        SET SYMBOL="landstalker_us_expanded.sym"
        SET LISTING="landstalker_us_expanded.lst"
    )
    SET /A EXPANDED=%EXPANDED%
) ELSE IF %REGION%==JP (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_jp_expanded.asm"
        SET OUTPUT="landstalker_jp_expanded.bin"
        SET SYMBOL="landstalker_jp_expanded.sym"
        SET LISTING="landstalker_jp_expanded.lst"
        SET /A EXPANDED=1
    ) ELSE (
        SET SOURCE="landstalker_jp.asm"
        SET OUTPUT="landstalker_jp.bin"
        SET SYMBOL="landstalker_jp.sym"
        SET LISTING="landstalker_jp.lst"
        SET /A EXPANDED=0
    )
    SET /A REGION_CODE=1
    SET /A NTSC=1
    SET /A REGION_CHECK=0
    SET /A FIX_COLL_1=0
    SET /A FIX_COLL_2=0
    SET /A FIX_ARMLET_SKIP=0
    SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=0
    SET /A FIX_SPRITE_HIDE=0
    SET /A ENABLE_GOLD_COUNT_ON_TREASURE=0
    SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=0
    SET /A FIX_GOLD_CAP_ON_FILE_LOAD=0
    SET /A FIX_END_CREDS_GLITCH=0
    SET /A FIX_TS_GLITCH=0
) ELSE IF %REGION%==EUR (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_eur_expanded.asm"
        SET OUTPUT="landstalker_eur_expanded.bin"
        SET SYMBOL="landstalker_eur_expanded.sym"
        SET LISTING="landstalker_eur_expanded.lst"
        SET /A EXPANDED=1
    ) ELSE (
        SET SOURCE="landstalker_eur.asm"
        SET OUTPUT="landstalker_eur.bin"
        SET SYMBOL="landstalker_eur.sym"
        SET LISTING="landstalker_eur.lst"
        SET /A EXPANDED=0
    )
    SET /A REGION_CODE=2
    SET /A NTSC=0
    SET /A REGION_CHECK=1
    SET /A FIX_COLL_1=1
    SET /A FIX_COLL_2=1
    SET /A FIX_ARMLET_SKIP=1
    SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    SET /A FIX_SPRITE_HIDE=1
    SET /A ENABLE_GOLD_COUNT_ON_TREASURE=1
    SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    SET /A FIX_GOLD_CAP_ON_FILE_LOAD=1
    SET /A FIX_END_CREDS_GLITCH=1
    SET /A FIX_TS_GLITCH=1
) ELSE IF %REGION%==FR (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_fr_expanded.asm"
        SET OUTPUT="landstalker_fr_expanded.bin"
        SET SYMBOL="landstalker_fr_expanded.sym"
        SET LISTING="landstalker_fr_expanded.lst"
        SET /A EXPANDED=1
    ) ELSE (
        SET SOURCE="landstalker_fr.asm"
        SET OUTPUT="landstalker_fr.bin"
        SET SYMBOL="landstalker_fr.sym"
        SET LISTING="landstalker_fr.lst"
        SET /A EXPANDED=0
    )
    SET /A REGION_CODE=3
    SET /A NTSC=0
    SET /A REGION_CHECK=1
    SET /A FIX_COLL_1=1
    SET /A FIX_COLL_2=1
    SET /A FIX_ARMLET_SKIP=1
    SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    SET /A FIX_SPRITE_HIDE=1
    SET /A ENABLE_GOLD_COUNT_ON_TREASURE=1
    SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    SET /A FIX_GOLD_CAP_ON_FILE_LOAD=1
    SET /A FIX_END_CREDS_GLITCH=1
    SET /A REFRESH_GOLD_CTR=0
    SET /A FIX_TS_GLITCH=1
) ELSE IF %REGION%==DE (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_de_expanded.asm"
        SET OUTPUT="landstalker_de_expanded.bin"
        SET SYMBOL="landstalker_de_expanded.sym"
        SET LISTING="landstalker_de_expanded.lst"
        SET /A EXPANDED=1
    ) ELSE (
        SET SOURCE="landstalker_de.asm"
        SET OUTPUT="landstalker_de.bin"
        SET SYMBOL="landstalker_de.sym"
        SET LISTING="landstalker_de.lst"
        SET /A EXPANDED=0
    )
    SET /A REGION_CODE=4
    SET /A NTSC=0
    SET /A REGION_CHECK=1
    SET /A FIX_COLL_1=1
    SET /A FIX_COLL_2=1
    SET /A FIX_ARMLET_SKIP=1
    SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    SET /A FIX_SPRITE_HIDE=1
    SET /A ENABLE_GOLD_COUNT_ON_TREASURE=1
    SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    SET /A FIX_GOLD_CAP_ON_FILE_LOAD=1
    SET /A FIX_END_CREDS_GLITCH=0
    SET /A REFRESH_GOLD_CTR=0
    SET /A FIX_TS_GLITCH=1
) ELSE IF %REGION%==BETA (
    IF %EXPANDED%==1 (
        SET SOURCE="landstalker_beta_expanded.asm"
        SET OUTPUT="landstalker_beta_expanded.bin"
        SET SYMBOL="landstalker_beta_expanded.sym"
        SET LISTING="landstalker_beta_expanded.lst"
        SET /A EXPANDED=1
    ) ELSE (
        SET SOURCE="landstalker_beta.asm"
        SET OUTPUT="landstalker_beta.bin"
        SET SYMBOL="landstalker_beta.sym"
        SET LISTING="landstalker_beta.lst"
        SET /A EXPANDED=0
    )
    SET /A REGION_CODE=5
    SET /A NTSC=1
    SET /A REGION_CHECK=0
    SET /A FIX_COLL_1=1
    SET /A FIX_COLL_2=0
    SET /A FIX_ARMLET_SKIP=0
    SET /A FIX_EINSTEIN_WHISTLE_FLAG_CHECK=0
    SET /A FIX_SPRITE_HIDE=0
    SET /A ENABLE_GOLD_COUNT_ON_TREASURE=0
    SET /A FIX_POTENTIAL_CORRUPTION_ON_GOLA=0
    SET /A FIX_GOLD_CAP_ON_FILE_LOAD=0
    SET /A FIX_END_CREDS_GLITCH=0
    SET /A FIX_TS_GLITCH=0
) ELSE (
    ECHO Unsupported Region
    EXIT 1
)

SET DEFINES="/e "
SET DEFINES=%DEFINES%"REGION="%REGION_CODE%
SET DEFINES=%DEFINES%";NTSC="%NTSC%
SET DEFINES=%DEFINES%";EXPANDED="%EXPANDED%
SET DEFINES=%DEFINES%";REGION_CHECK="%REGION_CHECK%
SET DEFINES=%DEFINES%";FIX_COLL_1="%FIX_COLL_1%
SET DEFINES=%DEFINES%";FIX_COLL_2="%FIX_COLL_2%
SET DEFINES=%DEFINES%";FIX_ARMLET_SKIP="%FIX_ARMLET_SKIP%
SET DEFINES=%DEFINES%";FIX_WHISTLE_CHECK="%FIX_EINSTEIN_WHISTLE_FLAG_CHECK%
SET DEFINES=%DEFINES%";FIX_SPRITE_HIDE="%FIX_SPRITE_HIDE%
SET DEFINES=%DEFINES%";ENABLE_GOLD_COUNT="%ENABLE_GOLD_COUNT_ON_TREASURE%
SET DEFINES=%DEFINES%";FIX_GOLA_BUG="%FIX_POTENTIAL_CORRUPTION_ON_GOLA%
SET DEFINES=%DEFINES%";FIX_GOLD_CAP="%FIX_GOLD_CAP_ON_FILE_LOAD%
SET DEFINES=%DEFINES%";FIX_END_CREDS="%FIX_END_CREDS_GLITCH%
SET DEFINES=%DEFINES%";REFRESH_GOLD_CTR="%REFRESH_GOLD_CTR%
SET DEFINES=%DEFINES%";FIX_TS_GLITCH="%FIX_TS_GLITCH%
SET DEFINES=%DEFINES:"=%

SET Z80DEFINES="-D "
SET Z80DEFINES=%Z80DEFINES%"EXPANDED="%EXPANDED%
SET Z80DEFINES=%Z80DEFINES:"=%

SET Z80AS_DRV_OPTS=%Z80AS_DRV_OPTS:"=%
SET Z80LINK_DRV_OPTS=%Z80LINK_DRV_OPTS:"=%
SET Z80AS_BANK3_OPTS=%Z80AS_BANK3_OPTS:"=%
SET Z80LINK_BANK3_OPTS=%Z80LINK_BANK3_OPTS:"=%
SET Z80AS_BANK4_OPTS=%Z80AS_BANK4_OPTS:"=%
SET Z80LINK_BANK4_OPTS=%Z80LINK_BANK4_OPTS:"=%

IF NOT "%MANUAL_OUTPUT%"=="" SET OUTPUT=%MANUAL_OUTPUT%

@ECHO ON
:: Build Sound Bank 3
%Z80AS% %Z80BANK3_SRC% %Z80AS_BANK3_OPTS% -o %Z80BANK3_OBJ% %Z80DEFINES%
%Z80LINK% %Z80BANK3_OBJ% %Z80BANK3_BIN% %Z80LINK_BANK3_OPTS%
:: Build Sound Bank 4
%Z80AS% %Z80BANK4_SRC% %Z80AS_BANK4_OPTS% -o %Z80BANK4_OBJ% %Z80DEFINES%
%Z80LINK% %Z80BANK4_OBJ% %Z80BANK4_BIN% %Z80LINK_BANK4_OPTS%
:: Build Cube/Iwadare Driver
%Z80AS% %Z80DRV_SRC% %Z80AS_DRV_OPTS% -o %Z80DRV_OBJ% %Z80DEFINES%
%Z80LINK% %Z80DRV_OBJ% %Z80DRV_BIN% %Z80LINK_DRV_OPTS%
:: Build ROM
%AS% %STANDARD_BUILDOPTS% %DEFINES% %SOURCE%,%OUTPUT%,%SYMBOL%,%LISTING%
:: Fix checksum
python tools\build\fix_checksum.py %OUTPUT%

@ECHO OFF
ENDLOCAL
