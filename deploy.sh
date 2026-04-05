#!/bin/bash
set -e
/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/
echo "✓ DFAF deployed"
