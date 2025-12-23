#!/bin/bash
  
REAL_PATH=$(realpath "$0")
BIN_PATH=$(dirname ${REAL_PATH})
TOOL_PATH=$(realpath ${BIN_PATH}/../)
LIB_PATH=$(realpath ${TOOL_PATH}/lib)
 
if [ -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH="${LIB_PATH}"
else
    export LD_LIBRARY_PATH="${LIB_PATH}:${LD_LIBRARY_PATH}"
fi
 
concat_path="/usr/share/terminfo:/etc/terminfo:/lib/terminfo"
if [ -z "$TERMINFO" ]; then
    export TERMINFO="$concat_path"
else
    export TERMINFO="$concat_path:${TERMINFO}"
fi
 
${BIN_PATH}/msdebug.bin "$@"