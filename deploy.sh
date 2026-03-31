#!/bin/bash
set -e
cd /Users/eriksode/Projects/DFAF && cmake --build build
cp -r /Users/eriksode/Projects/DFAF/build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/DFAF/
echo "✓ DFAF deployed"
