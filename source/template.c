/*---------------------------------------------------------------------------------

PongDS - Simple Pong-like game for the Nintendo DS

Author: Asier Iturralde Sarasola
License: GPL v3

---------------------------------------------------------------------------------*/

#include <nds.h>
#include <maxmod9.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h> // C99 defines bool, true and false in stdbool.h

#include "soundbank.h"
#include "soundbank_bin.h"

#include "background.h"
#include <digits.h>
#include <splash.h>
#include <language_menu.h>
#include <main_menu_en.h>
#include <main_menu_eu.h>
#include <main_menu_es.h>
#include <main_menu_fr.h>
#include <one_p_game_menu_en.h>
#include <one_p_game_menu_eu.h>
#include <one_p_game_menu_es.h>
#include <two_p_game_menu_en.h>
#include <two_p_game_menu_eu.h>
#include <two_p_game_menu_es.h>

#define DEGREE_TO_RADIAN 0.01745329251
#define INITIAL_SPEED 1.5
#define BALL_HEIGHT 8
#define BALL_WIDTH 8
#define PADDLE_HEIGHT 32
#define PADDLE_WIDTH 8
#define SCORE_LIMIT 10

typedef struct {
   double x;
   double y;
   double speed;
   int angle;
} ball;

typedef struct {
    int x;
    int y;
    int speed;
    int height;
    int width;
    int score;
} paddle;

enum state_options {
    LANGUAGE_MENU = 0,
    MAIN_MENU = 1,
    ONE_PLAYER_GAME = 2,
    TWO_PLAYERS_GAME = 3
};

enum languages {
    EN = 0,
    EU = 1,
    ES = 2,
    FR = 3
};

bool isBitSet(unsigned int value, unsigned int bit_flag) {
    return (value & bit_flag) != 0;
}

unsigned int setBit(unsigned int value, unsigned int bit_flag) {

    return value | bit_flag;
}

unsigned int unsetBit(unsigned int value, unsigned int bit_flag) {

    return value & ~(bit_flag);

}

//---------------------------------------------------------------------
// Returns a random number between 0 and limit inclusive
// http://stackoverflow.com/a/2999130/2855012
//---------------------------------------------------------------------
int rand_lim(int limit) {

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;

}

//---------------------------------------------------------------------
// Set the video modes of the screens
// and set the VRAM banks to the corresponding values
//---------------------------------------------------------------------
void initScreensAndVRAM() {

    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    vramSetBankC(VRAM_C_SUB_BG);

}

//---------------------------------------------------------------------
// Load all the digits into memory
//---------------------------------------------------------------------
void initDigits(u8* gfx, u16* sprite_gfx_mem[]) {
	int i;

	for(i = 0; i < 12; i++) {
		sprite_gfx_mem[i] = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
		dmaCopy(gfx, sprite_gfx_mem[i], 32*32);
		gfx += 32*32;
	}

    dmaCopy(digitsPal, SPRITE_PALETTE, 512);
}

//---------------------------------------------------------------------
// Displays the splash screen
//---------------------------------------------------------------------
int showSplash() {

    // set up the bitmap background of the main screen (splash screen)
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
    decompress(splashBitmap, BG_GFX,  LZ77Vram);

    return 0;
}

//---------------------------------------------------------------------
// Shows the corresponding menu
//---------------------------------------------------------------------
int showMenu(int state, unsigned int language) {

    // set up the bitmap background of the main menu on the sub screen
	bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);

    switch (state) {

        case LANGUAGE_MENU:
            decompress(language_menuBitmap, BG_GFX_SUB,  LZ77Vram);
            break;

        case MAIN_MENU:

            if (language == EN) {
                decompress(main_menu_enBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == EU) {
                decompress(main_menu_euBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == ES) {
                decompress(main_menu_esBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == FR) {
                decompress(main_menu_frBitmap, BG_GFX_SUB,  LZ77Vram);
            }
            break;

        case ONE_PLAYER_GAME:
            if (language == EN) {
                decompress(one_p_game_menu_enBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == EU) {
                decompress(one_p_game_menu_euBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == ES) {
                decompress(one_p_game_menu_esBitmap, BG_GFX_SUB,  LZ77Vram);
            }
            break;

        case TWO_PLAYERS_GAME:
            if (language == EN) {
                decompress(two_p_game_menu_enBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == EU) {
                decompress(two_p_game_menu_euBitmap, BG_GFX_SUB,  LZ77Vram);
            } else if (language == ES) {
                decompress(two_p_game_menu_esBitmap, BG_GFX_SUB,  LZ77Vram);
            }
            break;

        default:
            decompress(language_menuBitmap, BG_GFX_SUB,  LZ77Vram);

    }

    return 0;

}

//---------------------------------------------------------------------
// Initializes the game field
//---------------------------------------------------------------------
int initGameField() {

    // set up the bitmap background of the main screen (game field)
    bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
    decompress(backgroundBitmap, BG_GFX,  LZ77Vram);

    return 0;
}

//---------------------------------------------------------------------
// Initializes the game
//---------------------------------------------------------------------
int initGame(ball *b, paddle *p1, paddle *p2, u16* sprite_gfx_mem[]) {

    b->x = SCREEN_WIDTH / 2 - 1 - BALL_WIDTH / 2;
    b->y = SCREEN_HEIGHT / 2 - 1 - BALL_HEIGHT / 2;
    b->speed = INITIAL_SPEED;
    b->angle = rand_lim(360);

    p1->x = 0;
    p1->y = SCREEN_HEIGHT / 2 - 1 - PADDLE_HEIGHT / 2;
    p1->speed = 1;
    p1->score = 0;

    p2->x = SCREEN_WIDTH - PADDLE_WIDTH;
    p2->y = SCREEN_HEIGHT / 2 - 1 - PADDLE_HEIGHT / 2;
    p2->speed = 1;
    p2->score = 0;

    // Set the oam entry for the score of the first player
    oamSet(&oamMain,                    // main graphics engine context
           3,                           // oam index (0 to 127)
           SCREEN_WIDTH / 2 - 40,       // x location of the sprite
           8,                           // y location of the sprite
           0,                           // priority, lower renders last (on top)
           0,                           // this is the palette index if multiple palettes or the alpha value if bmp sprite
           SpriteSize_32x32,
           SpriteColorFormat_256Color,
           sprite_gfx_mem[p1->score],    // pointer to the loaded graphics
           -1,                          // sprite rotation data
           false,                       // double the size when rotating?
           false,                       // hide the sprite?
           false,                       // vflip
           false,                       // hflip
           false);                      // apply mosaic

    // Set the oam entry for the score of the second player
    oamSet(&oamMain,                    // main graphics engine context
           4,                           // oam index (0 to 127)
           SCREEN_WIDTH / 2 + 8,       // x location of the sprite
           8,                           // y location of the sprite
           0,                           // priority, lower renders last (on top)
           0,                           // this is the palette index if multiple palettes or the alpha value if bmp sprite
           SpriteSize_32x32,
           SpriteColorFormat_256Color,
           sprite_gfx_mem[p2->score],    // pointer to the loaded graphics
           -1,                          // sprite rotation data
           false,                       // double the size when rotating?
           false,                       // hide the sprite?
           false,                       // vflip
           false,                       // hflip
           false);                      // apply mosaic

    return 0;
}

//---------------------------------------------------------------------------------
int main(void) {
	//---------------------------------------------------------------------------------
	int i = 0;

    // The digit sprites
    u16* sprite_gfx_mem[12];

    int keys_pressed, keys_held, keys_released;

    unsigned int language;

    unsigned int state;

    bool game_ended = false;

    // Ball
    ball b;

    // Left paddle
    paddle p1;

    // Rigth paddle
    paddle p2;

    state = LANGUAGE_MENU;
    language = EN;

    initScreensAndVRAM();

    showSplash();

    // Show the language menu
    showMenu(state, language);

    // Initialize the 2D sprite engine of the main (top) screen
	oamInit(&oamMain, SpriteMapping_1D_128, false);

    initDigits((u8*)digitsTiles, sprite_gfx_mem);

    // Allocate graphics memory for the sprites of the ball and the paddles
	u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_256Color);
    u16* gfx_p1 = oamAllocateGfx(&oamMain, SpriteSize_8x32, SpriteColorFormat_256Color);
    u16* gfx_p2 = oamAllocateGfx(&oamMain, SpriteSize_8x32, SpriteColorFormat_256Color);

	for(i = 0; i < BALL_HEIGHT * BALL_WIDTH / 2; i++) {
		gfx[i] = 1 | (1 << 8);
	}

    for(i = 0; i < PADDLE_HEIGHT * PADDLE_WIDTH / 2; i++) {
		gfx_p1[i] = 1 | (1 << 8);
	}

    for(i = 0; i < PADDLE_HEIGHT * PADDLE_WIDTH / 2; i++) {
		gfx_p2[i] = 1 | (1 << 8);
	}

	SPRITE_PALETTE[1] = RGB15(31,31,31);    // White

    mmInitDefaultMem((mm_addr)soundbank_bin);

	// load sound effects
	mmLoadEffect(SFX_TXALAPARTA1);
    mmLoadEffect(SFX_TXALAPARTA2);

	mm_sound_effect txalaparta1 = {
		{ SFX_TXALAPARTA1 },	// id
		(int)(1.0f * (1<<10)),	// rate
		0,		                // handle
		255,	                // volume
		255,	                // panning
	};

    mm_sound_effect txalaparta2 = {
		{ SFX_TXALAPARTA2 },	// id
		(int)(1.0f * (1<<10)),	// rate
		0,		                // handle
		255,	                // volume
		255,	                // panning
	};

	while(1) {

		scanKeys();

        keys_pressed = keysDown();
        keys_held = keysHeld();
        keys_released = keysUp();

        touchPosition touch;

        // The user tapped on a menu option
        if(keys_pressed & KEY_TOUCH) {

            touchRead(&touch);

            // The user pressed the first button
            if (touch.px >= 52 && touch.px <= 211 && touch.py >= 53 && touch.py <= 73) {

                // English button pressed in the language menu
                if (state == LANGUAGE_MENU) {

                    language = EN;
                    state = MAIN_MENU;

                    showMenu(state, language);

                // 1 player mode button pressed in the main menu
                } else if (state == MAIN_MENU) {

                    state = ONE_PLAYER_GAME;

                    initGameField();

                    initGame(&b, &p1, &p2, sprite_gfx_mem);

                    game_ended = false;

                    showMenu(state, language);

                // Restart button pressed (1 player mode)
                } else if (state == ONE_PLAYER_GAME) {

                    initGame(&b, &p1, &p2, sprite_gfx_mem);

                    game_ended = false;

                // Restart button pressed (2 players mode)
                } else if (state == TWO_PLAYERS_GAME) {

                    initGame(&b, &p1, &p2, sprite_gfx_mem);

                    game_ended = false;

                }

            // The user pressed the second button
            } else if (touch.px >= 52 && touch.px <= 211 && touch.py >= 77 && touch.py <= 97) {

                // Basque button pressed in the language menu
                if (state == LANGUAGE_MENU) {

                    language = EU;
                    state = MAIN_MENU;

                    showMenu(state, language);


                // 2 players mode button pressed in the main menu
                } else if (state == MAIN_MENU) {

                    state = TWO_PLAYERS_GAME;

                    initGameField();

                    initGame(&b, &p1, &p2, sprite_gfx_mem);

                    game_ended = false;

                    showMenu(state, language);

                // Back to main menu button pressed (1 player mode)
                } else if (state == ONE_PLAYER_GAME) {

                    state = MAIN_MENU;

                    // Clear all the sprites of the game
                    oamClear(&oamMain, 0, 128);

                    showSplash();

                    // Display the main menu
                    showMenu(state, language);

                // Back to main menu button pressed (2 players mode)
                } else if (state == TWO_PLAYERS_GAME) {

                    state = MAIN_MENU;

                    // Clear all the sprites of the game
                    oamClear(&oamMain, 0, 128);

                    showSplash();

                    // Display the main menu
                    showMenu(state, language);

                }

            // The user tapped the third button
            } else if (touch.px >= 52 && touch.px <= 211 && touch.py >= 103 && touch.py <= 123) {

                // Spanish button pressed in the language menu
                if (state == LANGUAGE_MENU) {

                    language = ES;
                    state = MAIN_MENU;

                    showMenu(state, language);

                }

            // The user pressed the fourth button
            } else if (touch.px >= 52 && touch.px <= 211 && touch.py >= 128 && touch.py <= 148) {

                // French button pressed in the language menu
                if (state == LANGUAGE_MENU) {

                    language = FR;
                    state = MAIN_MENU;

                    showMenu(state, language);

                }

            }

        } else if (game_ended == false && (state == ONE_PLAYER_GAME || state == TWO_PLAYERS_GAME)) {

            // One player mode (VS CPU)
            if (state == ONE_PLAYER_GAME) {

                // Artificial intelligence for the paddle controlled by the CPU (only in one player mode)

                // If the ball is moving towards the paddle controlled by the CPU
                if (b.speed * cos(b.angle * DEGREE_TO_RADIAN) > 0) {

                    // If the ball is above the paddle
                    if (b.y < p2.y) {

                        // Don't let the paddle move above the top of the screen
                        if (p2.y > 0) {

                            // Move the paddle up
                            p2.y = p2.y - 1;

                        }

                    // If the ball is below the paddle
                    } else {

                        // Don't let the paddle move below the bottom of the screen
                        if (p2.y < SCREEN_HEIGHT - PADDLE_HEIGHT) {

                            // Move the paddle down
                            p2.y = p2.y + 1;

                        }

                    }

                // If the ball is moving towards the paddle controlled by the user
                } else {

                    // If the paddle controlled by the CPU is above the center of the screen
                    if (p2.y > SCREEN_HEIGHT / 2 - 1 - PADDLE_HEIGHT / 2) {

                        // Move the paddle controlled byt the CPU down
                        p2.y = p2.y - 1;

                    // If the paddle controlled by the CPU is below the center of the screen
                    } else {

                        // Move the paddle controlled by the CPU up
                        p2.y = p2.y + 1;

                    }
                }

                // If the player is holding the up button
                if (keys_held & KEY_UP) {

                    // Don't let the paddle move above the top of the screen
                    if (p1.y > 0) {

                        // Move the right paddle up
                        p1.y = p1.y - 1;

                    }

                // Else if the player is holding the down button
                } else if (keys_held & KEY_DOWN) {

                    // Don't let the paddle move below the bottom of the screen
                    if (p1.y < SCREEN_HEIGHT - PADDLE_HEIGHT) {

                        // Move the right paddle down
                        p1.y = p1.y + 1;

                    }

                }

            // Two players mode
            } else {

                // If the first player is holding the up button
                if (keys_held & KEY_UP) {

                    // Don't let the paddle move above the top of the screen
                    if (p1.y > 0) {

                        // Move the right paddle up
                        p1.y = p1.y - 1;

                    }

                // Else if the first player is holding the down button
                } else if (keys_held & KEY_DOWN) {

                    // Don't let the paddle move below the bottom of the screen
                    if (p1.y < SCREEN_HEIGHT - PADDLE_HEIGHT) {

                        // Move the right paddle down
                        p1.y = p1.y + 1;

                    }

                }

                // If the second player is holding the X button
                if (keys_held & KEY_X) {

                    // Don't let the paddle move above the top of the screen
                    if (p2.y > 0) {

                        p2.y = p2.y - 1;

                    }

                // Else if the second player is holding the B button
                } else if (keys_held & KEY_B) {

                    // Don't let the paddle move below the bottom of the screen
                    if (p2.y < SCREEN_HEIGHT - PADDLE_HEIGHT) {

                        p2.y = p2.y + 1;

                    }

                }

            }

            /*
             *         270
             *          |
             *          |
             *          |
             * 180 ----------- 0
             *          |
             *          |
             *          |
             *         90
             *
             */

            // Bottom of the screen
            if (b.y + b.speed * sin(b.angle * DEGREE_TO_RADIAN) >= SCREEN_HEIGHT - 1 - BALL_HEIGHT) {

                b.angle = 180 - (b.angle - 180);

            // Top of the screen
            } else if (b.y + b.speed * sin(b.angle * DEGREE_TO_RADIAN) <= 0) {

                // b.angle = 0 - (b.angle - 0)
                b.angle = -b.angle;

            // Left paddle collision detection
            } else if (b.x <= p1.x + PADDLE_WIDTH && b.y > p1.y - BALL_HEIGHT && b.y < p1.y + PADDLE_HEIGHT + BALL_HEIGHT) {

                int hit_y = b.y - p1.y + BALL_HEIGHT;

                // The return angle is the specular image of the hit angle
                // b.angle = 90 - (b.angle - 90);

                // The return angle is going to be between 300 and 60 degrees depending on the hit position
                b.angle = (int) 300 + (120 * hit_y / 48.0);

                // Increase the speed of the ball
                b.speed = b.speed + 0.1;

                mmEffectEx(&txalaparta1);

            // Right paddle collision detection
            } else if (b.x >= p2.x - PADDLE_WIDTH && b.y > p2.y - BALL_HEIGHT && b.y < p2.y + PADDLE_HEIGHT + BALL_HEIGHT) {

                int hit_y = b.y - p2.y + BALL_HEIGHT;

                // The return angle is the specular image of the hit angle
                //b.angle = 270 - (b.angle - 270);

                // The return angle is going to be between 240 and 60 degrees depending on the hit position
                b.angle = (int) 240 - (120 * hit_y / 48.0);

                // Increase the speed of the ball
                b.speed = b.speed + 0.1;

                mmEffectEx(&txalaparta2);

            // Left border of the screen
            } else if (b.x <= 0) {

                p2.score = p2.score + 1;

                // Set the oam entry for the score of the second player
                oamSet(&oamMain,                    // main graphics engine context
                        4,                           // oam index (0 to 127)
                        SCREEN_WIDTH / 2 + 8,       // x location of the sprite
                        8,                           // y location of the sprite
                        0,                           // priority, lower renders last (on top)
                        0,                           // this is the palette index if multiple palettes or the alpha value if bmp sprite
                        SpriteSize_32x32,
                        SpriteColorFormat_256Color,
                        sprite_gfx_mem[p2.score],    // pointer to the loaded graphics
                        -1,                          // sprite rotation data
                        false,                       // double the size when rotating?
                        false,                       // hide the sprite?
                        false,                       // vflip
                        false,                       // hflip
                        false);                      // apply mosaic

                if (p2.score < SCORE_LIMIT) {

                    b.x = SCREEN_WIDTH / 2 - 1 - BALL_WIDTH / 2;
                    b.y = SCREEN_HEIGHT / 2 - 1 - BALL_HEIGHT / 2;

                    b.angle = rand_lim(180) + 270;

                    b.speed = INITIAL_SPEED;

                } else {

                    game_ended = true;

                    // Hide the ball
                    oamClearSprite(&oamMain, 0);

                }

            // Right border of the screen
            } else if (b.x >= SCREEN_WIDTH - 1) {

                p1.score = p1.score + 1;

                // Set the oam entry for the score of the first player
                oamSet(&oamMain,                    // main graphics engine context
                        3,                           // oam index (0 to 127)
                        SCREEN_WIDTH / 2 - 40,       // x location of the sprite
                        8,                           // y location of the sprite
                        0,                           // priority, lower renders last (on top)
                        0,                           // this is the palette index if multiple palettes or the alpha value if bmp sprite
                        SpriteSize_32x32,
                        SpriteColorFormat_256Color,
                        sprite_gfx_mem[p1.score],    // pointer to the loaded graphics
                        -1,                          // sprite rotation data
                        false,                       // double the size when rotating?
                        false,                       // hide the sprite?
                        false,                       // vflip
                        false,                       // hflip
                        false);                      // apply mosaic

                if (p1.score < SCORE_LIMIT) {

                    b.x = SCREEN_WIDTH / 2 - 1 - BALL_WIDTH / 2;
                    b.y = SCREEN_HEIGHT / 2 - 1 - BALL_HEIGHT / 2;

                    b.angle = rand_lim(180) + 90;

                    b.speed = INITIAL_SPEED;

                } else {

                    game_ended = true;

                    // Hide the ball
                    oamClearSprite(&oamMain, 0);

                }

            }

            if (game_ended == false) {

                // Update the position of the ball
                b.x = b.x + b.speed * cos(b.angle * DEGREE_TO_RADIAN);
                b.y = b.y + b.speed * sin(b.angle * DEGREE_TO_RADIAN);

                // Set the oam entry for the ball
                oamSet(&oamMain, //main graphics engine context
                    0,           //oam index (0 to 127)
                    (int) b.x, (int) b.y,   //x and y pixle location of the sprite
                    0,                    //priority, lower renders last (on top)
                    0,					  //this is the palette index if multiple palettes or the alpha value if bmp sprite
                    SpriteSize_8x8,
                    SpriteColorFormat_256Color,
                    gfx,                  //pointer to the loaded graphics
                    -1,                  //sprite rotation data
                    false,               //double the size when rotating?
                    false,			//hide the sprite?
                    false, false, //vflip, hflip
                    false	//apply mosaic
                    );

                // Set the oam entry for the left paddle
                oamSet(&oamMain, //main graphics engine context
                    1,           //oam index (0 to 127)
                    p1.x, p1.y,   //x and y pixle location of the sprite
                    0,                    //priority, lower renders last (on top)
                    0,					  //this is the palette index if multiple palettes or the alpha value if bmp sprite
                    SpriteSize_8x32,
                    SpriteColorFormat_256Color,
                    gfx_p1,                  //pointer to the loaded graphics
                    -1,                  //sprite rotation data
                    false,               //double the size when rotating?
                    false,			//hide the sprite?
                    false, false, //vflip, hflip
                    false	//apply mosaic
                    );

                // Set the oam entry for the right paddle
                oamSet(&oamMain, //main graphics engine context
                    2,           //oam index (0 to 127)
                    p2.x, p2.y,   //x and y pixle location of the sprite
                    0,                    //priority, lower renders last (on top)
                    0,					  //this is the palette index if multiple palettes or the alpha value if bmp sprite
                    SpriteSize_8x32,
                    SpriteColorFormat_256Color,
                    gfx_p2,                  //pointer to the loaded graphics
                    -1,                  //sprite rotation data
                    false,               //double the size when rotating?
                    false,			//hide the sprite?
                    false, false, //vflip, hflip
                    false	//apply mosaic
                    );

            }

        }

        // Wait for a vertical blank interrupt
        swiWaitForVBlank();

        // Update the oam memories of the main screen
        oamUpdate(&oamMain);

	}

	return 0;
}
