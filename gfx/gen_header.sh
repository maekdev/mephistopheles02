#!/bin/sh
xxd -i mondial2_white.png > ../src/gfx/png_chessboard.h
xxd -i mondial2_black.png >> ../src/gfx/png_chessboard.h
xxd -i MephistoMondial2_white.png > ../src/gfx/png_newgfx.h
xxd -i MephistoMondial2_black.png >> ../src/gfx/png_newgfx.h
xxd -i MephistoMondial2_figures.png >> ../src/gfx/png_newgfx.h
