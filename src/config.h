#ifndef __CONFIG_H
#define __CONFIG_H

// INCLUDE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_image.h>


#ifndef _MAX_PATH
#define _MAX_PATH 256
#endif // _MAX_PATH

// window config
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// define color codes for gfx draw
#define COLOR_CURSOR 255,0,0,255
#define COLOR_LASTMOVE 0,255,0,128
#define COLOR_LEDON 255,0,0,255
#define COLOR_LEDOFF 0,0,0,255

// src png for figures
#define PNG_FIGURE_W 160
#define PNG_FIGURE_H 156

#define PNG_BOARD_W 810
#define PNG_BOARD_H 980

//const int PNG_BOARDX[8] = { 82,163,244,325,406,487,568,649 };
#define PNG_BOARDX0 82
#define PNG_BOARDX1 163
#define PNG_BOARDX2 244
#define PNG_BOARDX3 325
#define PNG_BOARDX4 406
#define PNG_BOARDX5 487
#define PNG_BOARDX6 568
#define PNG_BOARDX7 649
//const int PNG_BOARDY[10] = { 891,836,636,556,477,397,318,239,159,80 };
#define PNG_BOARDY0 891
#define PNG_BOARDY1 836
#define PNG_BOARDY2 636
#define PNG_BOARDY3 556
#define PNG_BOARDY4 477
#define PNG_BOARDY5 397
#define PNG_BOARDY6 318
#define PNG_BOARDY7 239
#define PNG_BOARDY8 159
#define PNG_BOARDY9 80
//const int PNG_FIELD_W = 78;
#define PNG_FIELD_W 80
//const int PNG_FIELD_H = 80;
#define PNG_FIELD_H 80


#endif // __CONFIG_H
