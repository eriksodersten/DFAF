#!/bin/bash
set -e
cd /Users/eriksode/Projects/DFAF && cmake --build build
cp -r build/DFAF_artefacts/Debug/AU/DFAF.component ~/Library/Audio/Plug-Ins/Components/
echo "✓ DFAF deployed"
