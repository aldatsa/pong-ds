/*---------------------------------------------------------------------------------

PongDS - Simple Pong-like game for the Nintendo DS

Author: Asier Iturralde Sarasola
License: GPL v3

---------------------------------------------------------------------------------*/

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

//---------------------------------------------------------------------------------
int main(void) {
	//---------------------------------------------------------------------------------
	int i = 0;
    touchPosition touch;
    
    int keys_pressed, keys_held, keys_released;
    
    typedef struct {
       unsigned char x;
       unsigned char y;
       char speed_x;
       char speed_y;
       unsigned char height;
       unsigned char width;
    } ball;

    typedef struct {
        unsigned char x;
        unsigned char y;
        char speed;
        unsigned char height;
        unsigned char width;
    } paddle;
    
    // Ball
    ball b = {SCREEN_WIDTH / 2 - 1 - 4, SCREEN_HEIGHT / 2 - 1 - 4, 1, 1, 8, 8};
    
    // Left paddle
    paddle p1 = {0, SCREEN_HEIGHT / 2 - 1 - 16, 1, 32, 8};
    
    // Rigth paddle
    paddle p2 = {SCREEN_WIDTH - 8, SCREEN_HEIGHT / 2 - 1 - 16, 1, 32, 8};
    
	videoSetMode(MODE_0_2D);
    //videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_SPRITE);
	//vramSetBankD(VRAM_D_SUB_SPRITE);
    
    // Initialize the 2D sprite engine of the main (top) screen
	oamInit(&oamMain, SpriteMapping_1D_32, false);
	
    // Initialize the 2D sprite engine of the sub (bottom) screen
    //oamInit(&oamSub, SpriteMapping_1D_32, false);
    
    // Allocate graphics memory for the sprites of the ball and the paddles
	u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_256Color);
    u16* gfx_p1 = oamAllocateGfx(&oamMain, SpriteSize_8x32, SpriteColorFormat_256Color);
    u16* gfx_p2 = oamAllocateGfx(&oamMain, SpriteSize_8x32, SpriteColorFormat_256Color);
    
	//u16* gfxSub = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_256Color);

	for(i = 0; i < b.height * b.width / 2; i++)
	{
		gfx[i] = 1 | (1 << 8);
		//gfxSub[i] = 1 | (1 << 8);
	}

    for(i = 0; i < p1.height * p1.width / 2; i++)
	{
		gfx_p1[i] = 1 | (1 << 8);
	}

    for(i = 0; i < p2.height * p2.width / 2; i++)
	{
		gfx_p2[i] = 1 | (1 << 8);
	}
    
	SPRITE_PALETTE[1] = RGB15(31,31,31);    // White
	//SPRITE_PALETTE_SUB[1] = RGB15(0,31,0);

    consoleDemoInit();
    
    iprintf("PongDS\n");
    
	while(1) {
        
		scanKeys();
        
        keys_pressed = keysDown();
        keys_held = keysHeld();
        keys_released = keysUp();
        
		if(keysHeld() & KEY_TOUCH) {
			touchRead(&touch);
		}
        
        // Artificial intelligence for the paddle controlled by the CPU
        //
        // If the ball is moving towards the paddle controlled by the CPU
        if (b.speed_x < 0) {
            
            // If the ball is above the paddle
            if (b.y < p1.y) {
                
                // Don't let the paddle move above the top of the screen
                if (p1.y > 0) {
                    
                    // Move the paddle up
                    p1.y = p1.y - 1;
                    
                }
                
            // If the ball is below the paddle    
            } else {
                
                // Don't let the paddle move below the bottom of the screen
                if (p1.y < SCREEN_HEIGHT - p1.height) {
                    
                    // Move the paddle down
                    p1.y = p1.y + 1;
                    
                }
                
            }
            
        // If the ball is moving towards the paddle controlled by the user    
        } else {
            
            // If the paddle controlled by the CPU is above the center of the screen
            if (p1.y > SCREEN_HEIGHT / 2 - 1 - p1.height) {
                
                // Move the paddle controlled byt the CPU down
                p1.y = p1.y - 1;
                
            // If the paddle controlled by the CPU is below the center of the screen
            } else {
                
                // Move the paddle controlled by the CPU up
                p1.y = p1.y + 1;
                
            }
        }
        
        // If the player is holding the up button
        if (keys_held & KEY_UP) {
            
            // Don't let the paddle move above the top of the screen
            if (p2.y > 0 && p2.y) {
                
                // Move the right paddle up
                p2.y = p2.y - 1;
                
            }
            
        // Else if the player is holding the down button
        } else if (keys_held & KEY_DOWN) {
            
            // Don't let the paddle move below the bottom of the screen
            if (p2.y < SCREEN_HEIGHT - p2.height) {
                
                // Move the right paddle down
                p2.y = p2.y + 1;
                
            }
            
        }
        
        // Bottom and top borders of the screen
        if (b.y == 0 || b.y == SCREEN_HEIGHT - 1 - b.height) {
            b.speed_y = -1 * b.speed_y;
        }
        
        // Left and right borders of the screen
        if (b.x == 0 || b.x == SCREEN_WIDTH - 1 - b.width) {
            b.speed_x = -1 * b.speed_x;
        }
        
        // Left paddle collision detection
        if (b.x == p1.x + p1.width && b.y > p1.y && b.y < p1.y + p1.height) {
            b.speed_x = -1 * b.speed_x;
        }
        
        // Right paddle collision detection
        if (b.x == p2.x - p2.width && b.y > p2.y && b.y < p2.y + p2.height) {
            b.speed_x = -1 * b.speed_x;
        }
        
        // Update the position of the ball
        b.x = b.x + b.speed_x;
        b.y = b.y + b.speed_y;
        
        iprintf("\x1b[10;0Hspeed_x = %d",b.speed_x);
        iprintf("\x1b[11;0Hspeed_y = %d",b.speed_y);
        
        // Set the oam entry for the ball
		oamSet(&oamMain, //main graphics engine context
			0,           //oam index (0 to 127)  
			b.x, b.y,   //x and y pixle location of the sprite
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
        
		/*oamSet(&oamSub, 
			0, 
			touch.px, 
			touch.py, 
			0, 
			0,
			SpriteSize_16x16, 
			SpriteColorFormat_256Color, 
			gfxSub, 
			-1, 
			false, 
			false,			
			false, false, 
			false	
			);*/
        
        // Wait for a vertical blank interrupt
		swiWaitForVBlank();
        
		// Update the oam memories of the main and sub screens
		oamUpdate(&oamMain);
		//oamUpdate(&oamSub);
	}

	return 0;
}