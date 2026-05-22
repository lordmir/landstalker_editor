#!/bin/bash
STANDARD_BUILDOPTS="/p /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_"
AS="wine tools/build/asm68k.exe"
Z80AS="wine tools/build/asw/asw.exe"
Z80LINK="wine tools/build/asw/p2bin.exe"

Z80AS_DRV_OPTS=""
Z80LINK_DRV_OPTS="-l 0xff -r 0x0000-0x1f7f -k"
Z80DRV_SRC="code/audio/main.asm"
Z80DRV_OBJ="cube.p"
Z80DRV_BIN="cube.bin"
Z80AS_BANK3_OPTS=""
Z80LINK_BANK3_OPTS="-l 0xff -r 0x8000-0xdfff -k"
Z80BANK3_SRC="code/audio/soundbank3.asm"
Z80BANK3_OBJ="soundbank3.p"
Z80BANK3_BIN="soundbank3.bin"
Z80AS_BANK4_OPTS=""
Z80LINK_BANK4_OPTS="-l 0xff -r 0x8000-0xffff -k"
Z80BANK4_SRC="code/audio/soundbank4.asm"
Z80BANK4_OBJ="soundbank4.p"
Z80BANK4_BIN="soundbank4.bin"

REGION="US"
EXPANDED=0

while getopts r:o:e flag; do
    case "${flag}" in
        r) REGION=${OPTARG};;
        o) MANUAL_OUTPUT=${OPTARG};;
        e) EXPANDED=1;;
        *) echo "Usage: ${0} [-r (US|JP|EUR|FR|DE|BETA)] [-e] [-o <Output ROM Filename>]";;
    esac
done

# Options
if [ ${REGION} == "US" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_us_expanded.asm"
        OUTPUT="landstalker_us_expanded.bin"
        SYMBOL="landstalker_us_expanded.sym"
        LISTING="landstalker_us_expanded.lst"
    else
        SOURCE="landstalker_us.asm"
        OUTPUT="landstalker_us.bin"
        SYMBOL="landstalker_us.sym"
        LISTING="landstalker_us.lst"
    fi
    REGION_CODE=0
    NTSC=1
    REGION_CHECK=1
    FIX_COLL_1=1
    FIX_COLL_2=1
    FIX_ARMLET_SKIP=1
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    FIX_SPRITE_HIDE=1
    ENABLE_GOLD_COUNT_ON_TREASURE=1
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    FIX_GOLD_CAP_ON_FILE_LOAD=1
    FIX_END_CREDS_GLITCH=0
    REFRESH_GOLD_CTR=1
    FIX_TS_GLITCH=1
elif [ ${REGION} == "JP" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_jp_expanded.asm"
        OUTPUT="landstalker_jp_expanded.bin"
        SYMBOL="landstalker_jp_expanded.sym"
        LISTING="landstalker_jp_expanded.lst"
    else
        SOURCE="landstalker_jp.asm"
        OUTPUT="landstalker_jp.bin"
        SYMBOL="landstalker_jp.sym"
        LISTING="landstalker_jp.lst"
    fi
    REGION_CODE=1
    NTSC=1
    REGION_CHECK=0
    FIX_COLL_1=0
    FIX_COLL_2=0
    FIX_ARMLET_SKIP=0
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=0
    FIX_SPRITE_HIDE=0
    ENABLE_GOLD_COUNT_ON_TREASURE=0
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=0
    FIX_GOLD_CAP_ON_FILE_LOAD=0
    FIX_END_CREDS_GLITCH=0
    REFRESH_GOLD_CTR=1
    FIX_TS_GLITCH=0
elif [ ${REGION} == "EUR" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_eur_expanded.asm"
        OUTPUT="landstalker_eur_expanded.bin"
        SYMBOL="landstalker_eur_expanded.sym"
        LISTING="landstalker_eur_expanded.lst"
    else
        SOURCE="landstalker_eur.asm"
        OUTPUT="landstalker_eur.bin"
        SYMBOL="landstalker_eur.sym"
        LISTING="landstalker_eur.lst"
    fi
    REGION_CODE=2
    NTSC=0
    REGION_CHECK=1
    FIX_COLL_1=1
    FIX_COLL_2=1
    FIX_ARMLET_SKIP=1
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    FIX_SPRITE_HIDE=1
    ENABLE_GOLD_COUNT_ON_TREASURE=1
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    FIX_GOLD_CAP_ON_FILE_LOAD=1
    FIX_END_CREDS_GLITCH=1
    REFRESH_GOLD_CTR=1
    FIX_TS_GLITCH=1
elif [ ${REGION} == "FR" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_fr_expanded.asm"
        OUTPUT="landstalker_fr_expanded.bin"
        SYMBOL="landstalker_fr_expanded.sym"
        LISTING="landstalker_fr_expanded.lst"
    else
        SOURCE="landstalker_fr.asm"
        OUTPUT="landstalker_fr.bin"
        SYMBOL="landstalker_fr.sym"
        LISTING="landstalker_fr.lst"
    fi
    REGION_CODE=3
    NTSC=0
    REGION_CHECK=1
    FIX_COLL_1=1
    FIX_COLL_2=1
    FIX_ARMLET_SKIP=1
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    FIX_SPRITE_HIDE=1
    ENABLE_GOLD_COUNT_ON_TREASURE=1
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    FIX_GOLD_CAP_ON_FILE_LOAD=1
    FIX_END_CREDS_GLITCH=1
    REFRESH_GOLD_CTR=0
    FIX_TS_GLITCH=1
elif [ ${REGION} == "DE" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_de_expanded.asm"
        OUTPUT="landstalker_de_expanded.bin"
        SYMBOL="landstalker_de_expanded.sym"
        LISTING="landstalker_de_expanded.lst"
    else
        SOURCE="landstalker_de.asm"
        OUTPUT="landstalker_de.bin"
        SYMBOL="landstalker_de.sym"
        LISTING="landstalker_de.lst"
    fi
    REGION_CODE=4
    NTSC=0
    REGION_CHECK=1
    FIX_COLL_1=1
    FIX_COLL_2=1
    FIX_ARMLET_SKIP=1
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=1
    FIX_SPRITE_HIDE=1
    ENABLE_GOLD_COUNT_ON_TREASURE=1
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=1
    FIX_GOLD_CAP_ON_FILE_LOAD=1
    FIX_END_CREDS_GLITCH=0
    REFRESH_GOLD_CTR=0
    FIX_TS_GLITCH=1
elif [ ${REGION} == "BETA" ]; then
    if [ ${EXPANDED} -eq 1 ]; then
        SOURCE="landstalker_beta_expanded.asm"
        OUTPUT="landstalker_beta_expanded.bin"
        SYMBOL="landstalker_beta_expanded.sym"
        LISTING="landstalker_beta_expanded.lst"
    else
        SOURCE="landstalker_beta.asm"
        OUTPUT="landstalker_beta.bin"
        SYMBOL="landstalker_beta.sym"
        LISTING="landstalker_beta.lst"
    fi
    REGION_CODE=5
    NTSC=1
    REGION_CHECK=0
    FIX_COLL_1=1
    FIX_COLL_2=0
    FIX_ARMLET_SKIP=0
    FIX_EINSTEIN_WHISTLE_FLAG_CHECK=0
    FIX_SPRITE_HIDE=0
    ENABLE_GOLD_COUNT_ON_TREASURE=0
    FIX_POTENTIAL_CORRUPTION_ON_GOLA=0
    FIX_GOLD_CAP_ON_FILE_LOAD=0
    FIX_END_CREDS_GLITCH=0
    REFRESH_GOLD_CTR=1
    FIX_TS_GLITCH=0
else
    echo Unsupported Region
    exit 1
fi

if [ -n "${MANUAL_OUTPUT}" ]; then
    OUTPUT=${MANUAL_OUTPUT}
fi

DEFINES="/e "
DEFINES=${DEFINES}"REGION="${REGION_CODE}
DEFINES=${DEFINES}";EXPANDED="${EXPANDED}
DEFINES=${DEFINES}";NTSC="${NTSC}
DEFINES=${DEFINES}";REGION_CHECK="${REGION_CHECK}
DEFINES=${DEFINES}";FIX_COLL_1="${FIX_COLL_1}
DEFINES=${DEFINES}";FIX_COLL_2="${FIX_COLL_2}
DEFINES=${DEFINES}";FIX_ARMLET_SKIP="${FIX_ARMLET_SKIP}
DEFINES=${DEFINES}";FIX_WHISTLE_CHECK="${FIX_EINSTEIN_WHISTLE_FLAG_CHECK}
DEFINES=${DEFINES}";FIX_SPRITE_HIDE="${FIX_SPRITE_HIDE}
DEFINES=${DEFINES}";ENABLE_GOLD_COUNT="${ENABLE_GOLD_COUNT_ON_TREASURE}
DEFINES=${DEFINES}";FIX_GOLA_BUG="${FIX_POTENTIAL_CORRUPTION_ON_GOLA}
DEFINES=${DEFINES}";FIX_GOLD_CAP="${FIX_GOLD_CAP_ON_FILE_LOAD}
DEFINES=${DEFINES}";FIX_END_CREDS="${FIX_END_CREDS_GLITCH}
DEFINES=${DEFINES}";REFRESH_GOLD_CTR="${REFRESH_GOLD_CTR}
DEFINES=${DEFINES}";FIX_TS_GLITCH="${FIX_TS_GLITCH}

Z80DEFINES="-D "
Z80DEFINES=${Z80DEFINES}"EXPANDED="${EXPANDED}

# Build Sound Bank 3
echo ${Z80AS} ${Z80BANK3_SRC} ${Z80AS_BANK3_OPTS} -o ${Z80BANK3_OBJ} ${Z80DEFINES}
${Z80AS} ${Z80BANK3_SRC} ${Z80AS_BANK3_OPTS} -o ${Z80BANK3_OBJ} ${Z80DEFINES}
echo ${Z80LINK} ${Z80BANK3_OBJ} ${Z80BANK3_BIN} ${Z80LINK_BANK3_OPTS}
${Z80LINK} ${Z80BANK3_OBJ} ${Z80BANK3_BIN} ${Z80LINK_BANK3_OPTS}
# Build Sound Bank 4
echo ${Z80AS} ${Z80BANK4_SRC} ${Z80AS_BANK4_OPTS} -o ${Z80BANK4_OBJ} ${Z80DEFINES}
${Z80AS} ${Z80BANK4_SRC} ${Z80AS_BANK4_OPTS} -o ${Z80BANK4_OBJ} ${Z80DEFINES}
echo ${Z80LINK} ${Z80BANK4_OBJ} ${Z80BANK4_BIN} ${Z80LINK_BANK4_OPTS}
${Z80LINK} ${Z80BANK4_OBJ} ${Z80BANK4_BIN} ${Z80LINK_BANK4_OPTS}
# Build Cube/Iwadare Driver
echo ${Z80AS} ${Z80DRV_SRC} ${Z80AS_OPTS} -o ${Z80DRV_OBJ} ${Z80DEFINES}
${Z80AS} ${Z80DRV_SRC} ${Z80AS_OPTS} -o ${Z80DRV_OBJ} ${Z80DEFINES}
echo ${Z80LINK} ${Z80DRV_OBJ} ${Z80DRV_BIN} ${Z80LINK_DRV_OPTS}
${Z80LINK} ${Z80DRV_OBJ} ${Z80DRV_BIN} ${Z80LINK_DRV_OPTS}

# Build ROM
echo ${AS} ${STANDARD_BUILDOPTS} ${DEFINES} ${SOURCE},${OUTPUT},${SYMBOL},${LISTING}
${AS} ${STANDARD_BUILDOPTS} ${DEFINES} ${SOURCE},${OUTPUT},${SYMBOL},${LISTING}

# Fix checksum
python3 tools/build/fix_checksum.py ${OUTPUT}
