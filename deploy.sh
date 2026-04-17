#!/bin/bash
set -e
/usr/local/bin/cmake --build /Users/eriksode/Projects/DFAF/build
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/VST3/DFAF.vst3 ~/Library/Audio/Plug-Ins/VST3/
echo "✓ DFAF deployed"
