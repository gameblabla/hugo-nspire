#include "osd_sdl_gfx.h"

//! PC Engine rendered screen
SDL_Surface *screen = NULL;

/* Overlay for hardware scaling */
SDL_Overlay *olay = NULL;
SDL_Color olay_cmap[256];

//! Host machine rendered screen
SDL_Surface *physical_screen = NULL;

SDL_Rect physical_screen_rect;

int blit_x,blit_y;
// where must we blit the screen buffer on screen

int screen_blit_x, screen_blit_y;
// where on the screen we must blit XBuf

UChar* XBuf;
// buffer for video flipping

UChar index_to_RGB[256];
// convertion array from bero's reduced pce palette to x11 palette

int osd_gfx_init();
int osd_gfx_init_normal_mode();
void osd_gfx_put_image_normal();
void osd_gfx_shut_normal_mode();

void osd_gfx_dummy_func();

osd_gfx_driver osd_gfx_driver_list[3] =
{
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },  
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode }
};

void osd_gfx_dummy_func(void)
{
}

/* Not used */
/*
void DrawPixel(SDL_Surface *screen, int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
	Uint32 color = SDL_MapRGB(screen->format, R, G, B);
	Uint16 *bufp;
	bufp = (Uint16 *)screen->pixels + y * screen->pitch + x;
	*bufp = color;
}*/
 
void Slock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    SDL_LockSurface(screen);
  }
}

void Sulock(SDL_Surface *screen)
{
  if ( SDL_MUSTLOCK(screen) )
  {
    SDL_UnlockSurface(screen);
  }
}

/*****************************************************************************

    Function: osd_gfx_put_image_normal

    Description: draw the raw computed picture to screen, without any effect
       trying to center it (I bet there is still some work on this, maybe not
                            in this function)
    Parameters: none
    Return: nothing

*****************************************************************************/
void osd_gfx_put_image_normal(void)
{
	UInt16 y;
	
	SDL_PixelFormat* fmt = physical_screen->format; 

    Slock(screen);

    for (y = 0; y < io.screen_h; y++)
	#if defined(NEW_GFX_ENGINE)
		memmove(screen->pixels + y * io.screen_w, osd_gfx_buffer + y * XBUF_WIDTH, io.screen_w);	
	#else
		memmove(screen->pixels + y * io.screen_w, XBuf + y * WIDTH + (WIDTH - io.screen_w), io.screen_w);	
	#endif

    Sulock(screen);
    
    //SDL_BlitSurface(screen,NULL,physical_screen,NULL);

	memmove(physical_screen->pixels, screen->pixels, io.screen_w * io.screen_h);
    

    SDL_Flip(physical_screen);
}

/*****************************************************************************

    Function: osd_gfx_set_message

    Description: compute the message that will be displayed to create a sprite
       to blit on screen
    Parameters: char* mess, the message to display
    Return: nothing but set OSD_MESSAGE_SPR

*****************************************************************************/
void osd_gfx_set_message(char* mess)
{
/*
 if (OSD_MESSAGE_SPR)
   destroy_bitmap(OSD_MESSAGE_SPR);

 OSD_MESSAGE_SPR=create_bitmap(text_length(font,mess)+1,text_height(font)+1);
 clear(OSD_MESSAGE_SPR);
 textout(OSD_MESSAGE_SPR,font,mess,1,1,3);
 textout(OSD_MESSAGE_SPR,font,mess,0,0,255);
*/

#warning implement set_message
  printf("%s\n",mess);
}


/*
 * osd_gfx_init:
 * One time initialization of the main output screen
 */
int osd_gfx_init(void)
{
	struct generic_rect rect;
	// We can't rely anymore on io variable being accessible at this stage of a game launching
	const int fake_io_screen_w = 352;
	const int fake_io_screen_h = 256;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
   	{
   		if (SDL_InitSubSystem(SDL_INIT_VIDEO))
   		{
   			printf("SDL_InitSubSystem(VIDEO) failed at %s:%d - %s\n", __FILE__, __LINE__, SDL_GetError());
   			return 0;
		}
   	}
   	
    SDL_ShowCursor(SDL_DISABLE);

	if ((physical_screen = SDL_SetVideoMode(option.want_fullscreen ? option.fullscreen_width : fake_io_screen_w * option.window_size,
                                          option.want_fullscreen ? option.fullscreen_height : fake_io_screen_h * option.window_size,
                                          8, SDL_SWSURFACE)) == NULL)
	{
		printf("SDL_SetVideoMode failed at %s:%d - %s\n", __FILE__, __LINE__, SDL_GetError());
		return 0;
	}

	host.video.hardware_scaling = 0;

	calc_fullscreen_aspect(physical_screen->w, physical_screen->h, &rect, fake_io_screen_w, fake_io_screen_h);

	physical_screen_rect.x = rect.start_x;
	physical_screen_rect.y = rect.start_y;
	physical_screen_rect.w = rect.end_x;
	physical_screen_rect.h = rect.end_y;
	
	SetPalette();

	screen = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_LOGPAL | SDL_PHYSPAL, fake_io_screen_w, fake_io_screen_h, 8, 0, 0, 0, 0) ;

  return 1;
}


/*****************************************************************************

    Function:  osd_gfx_init_normal_mode

    Description: initialize the classic 256*224 video mode for normal video_driver
    Parameters: none
    Return: 0 on error
            1 on success

*****************************************************************************/
int osd_gfx_init_normal_mode()
{
  struct generic_rect rect;

	if ((screen->w == io.screen_w) && (screen->h == io.screen_h))
	return 1;
	
    SDL_FreeSurface(screen);

#ifdef GFX_DEBUG
  printf("Mode change: %dx%d\n", io.screen_w, io.screen_h);
#endif

  if (io.screen_w == 0)
  {
    printf("This shouldn't happen? (io.screen_w == 0 in osd_gfx_init_normal_mode)\n");
    io.screen_w = 256;
  }

  if (io.screen_h == 0)
  {
    printf("This shouldn't happen? (io.screen_h == 0 in osd_gfx_init_normal_mode)\n");
    io.screen_h = 224;
  }

  if (physical_screen->flags & SDL_FULLSCREEN)
  {
    SDL_FillRect(physical_screen, NULL, 0);
  }
  else if (((physical_screen->w / option.window_size) != io.screen_w) ||
           ((physical_screen->h / option.window_size) != io.screen_h))
  {
    physical_screen = SDL_SetVideoMode(io.screen_w * option.window_size, io.screen_h * option.window_size,
                                       8,
                                       SDL_SWSURFACE);

    SetPalette();
  }

  calc_fullscreen_aspect(physical_screen->w, physical_screen->h, &rect, io.screen_w, io.screen_h);

  physical_screen_rect.x = rect.start_x;
  physical_screen_rect.y = rect.start_y;
  physical_screen_rect.w = rect.end_x;
  physical_screen_rect.h = rect.end_y;

  screen = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_LOGPAL, io.screen_w, io.screen_h, 8, 0, 0, 0, 0);
  
  /* 
	nSDL for TI Nspire requires SetPalette to be set 
	But what are the colors hugo are expecting ?
	for (cnt=0; cnt < PALETTE_SIZE; cnt++, pptr++)
	{
		colors[cnt].r = pptr->red;
		colors[cnt].g = pptr->green;
		colors[cnt].b = pptr->blue;
	}
	SDL_SetPalette (screen, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
  */

  return (screen && physical_screen) ? 1 : 0;
}


//! Delete the window
void osd_gfx_shut_normal_mode(void)
{
	SDL_FreeSurface(screen);
	screen = NULL;
	/* SDL will free physical_screen internally */
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	physical_screen = NULL;
}
 
/*****************************************************************************

    Function: osd_gfx_savepict

    Description: save a picture in the current directory
    Parameters: none
    Return: the numeric part of the created filename, 0xFFFF meaning that no more
      names were available

*****************************************************************************/
UInt16 osd_gfx_savepict()
  {
    short unsigned tmp=0;
    char filename[PATH_MAX];
    char filename_base[PATH_MAX];
    char* frame_buffer;
    FILE* output_file;
    time_t current_time;
    
    time(&current_time);

    if (!strftime(filename_base, PATH_MAX, "%%s/screenshot_%F_%R-%%d.ppm", localtime(&current_time)))
      return 0xFFFF;

    do {
      snprintf(filename, PATH_MAX, filename_base, video_path, tmp);
    } while (file_exists(filename) && ++tmp < 0xFFFF);
    
    frame_buffer = malloc(3 * (io.screen_w & 0xFFFE) * (io.screen_h & 0xFFFE));
    
    if (frame_buffer == NULL)
      return 0xFFFF;

    dump_rgb_frame(frame_buffer);

    output_file = fopen(filename, "wb");
    if (output_file != NULL)
      {
	char buf[100];
	
	snprintf(buf, sizeof(buf),
		 "P6\n%d %d\n%d\n",
		 io.screen_w & 0xFFFE, io.screen_h & 0xFFFE, 255);
	fwrite(buf, strlen(buf), 1, output_file);
	fwrite(frame_buffer, 3 * (io.screen_w & 0xFFFE) * (io.screen_h & 0xFFFE), 1, output_file);
	fclose(output_file);
      }

    return tmp;
  }


/*****************************************************************************

    Function:  osd_gfx_set_hugo_mode

    Description: change the video mode
    Parameters: mode : mode of video screen
                width, height : minimum size of screen required
    Return: 0 on success
                 1 on error

*****************************************************************************/
SInt32 osd_gfx_set_hugo_mode(SInt32 mode,SInt32 width,SInt32 height)
{

  screen = SDL_SetVideoMode(320,200, 16, SDL_SWSURFACE);
  SetPalette();	
  return !screen;
	 
 }

/*****************************************************************************

    Function: osd_gfx_set_color

    Description: Change the component of the choosen color
    Parameters: UChar index : index of the color to change
    			UChar r	: new red component of the color
                UChar g : new green component of the color
                UChar b : new blue component of the color
    Return:

*****************************************************************************/
void osd_gfx_set_color(UChar index,
                       UChar r,
                       UChar g,
                       UChar b)
{
	SDL_Color R;
	r <<= 2;
	g <<= 2;
	b <<= 2;
    R.r = r; 
    R.g = g; 
    R.b = b;
    SDL_SetColors(physical_screen, &R, index, 1);
}
