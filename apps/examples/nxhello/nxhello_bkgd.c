/****************************************************************************
 * examples/nxhello/nxhello_bkgd.c
 *
 *   Copyright (C) 2011, 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>
#include <iconv.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "nxhello.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Select renderer -- Some additional logic would be required to support
 * pixel depths that are not directly addressable (1,2,4, and 24).
 */

#if CONFIG_EXAMPLES_NXHELLO_BPP == 1
#  define RENDERER nxf_convert_1bpp
#elif CONFIG_EXAMPLES_NXHELLO_BPP == 2
#  define RENDERER nxf_convert_2bpp
#elif CONFIG_EXAMPLES_NXHELLO_BPP == 4
#  define RENDERER nxf_convert_4bpp
#elif CONFIG_EXAMPLES_NXHELLO_BPP == 8
#  define RENDERER nxf_convert_8bpp
#elif CONFIG_EXAMPLES_NXHELLO_BPP == 16
#  define RENDERER nxf_convert_16bpp
#elif CONFIG_EXAMPLES_NXHELLO_BPP == 24
#  define RENDERER nxf_convert_24bpp
#elif  CONFIG_EXAMPLES_NXHELLO_BPP == 32
#  define RENDERER nxf_convert_32bpp
#else
#  error "Unsupported CONFIG_EXAMPLES_NXHELLO_BPP"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/
/** added by cshuo **/
#define GBKLEN 255

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void nxhello_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool morem, FAR void *arg);
static void nxhello_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg);
#ifdef CONFIG_NX_XYINPUT
static void nxhello_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg);
#endif

#ifdef CONFIG_NX_KBD
static void nxhello_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_hello[] = "诸葛高   皇甫杰   魑魅魍魉";
static const char g_sm[] = "啊啊啊 诸葛高 皇甫杰 魑魅魍魉";

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Background window call table */

const struct nx_callback_s g_nxhellocb =
{
  nxhello_redraw,   /* redraw */
  nxhello_position  /* position */
#ifdef CONFIG_NX_XYINPUT
  , nxhello_mousein /* mousein */
#endif
#ifdef CONFIG_NX_KBD
  , nxhello_kbdin   /* my kbdin */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxhello_redraw
 ****************************************************************************/

static void nxhello_redraw(NXWINDOW hwnd, FAR const struct nxgl_rect_s *rect,
                        bool more, FAR void *arg)
{
  gvdbg("hwnd=%p rect={(%d,%d),(%d,%d)} more=%s\n",
         hwnd, rect->pt1.x, rect->pt1.y, rect->pt2.x, rect->pt2.y,
         more ? "true" : "false");
}

/****************************************************************************
 * Name: nxhello_position
 ****************************************************************************/

static void nxhello_position(NXWINDOW hwnd, FAR const struct nxgl_size_s *size,
                          FAR const struct nxgl_point_s *pos,
                          FAR const struct nxgl_rect_s *bounds,
                          FAR void *arg)
{
  /* Report the position */

  gvdbg("hwnd=%p size=(%d,%d) pos=(%d,%d) bounds={(%d,%d),(%d,%d)}\n",
        hwnd, size->w, size->h, pos->x, pos->y,
        bounds->pt1.x, bounds->pt1.y, bounds->pt2.x, bounds->pt2.y);

  /* Have we picked off the window bounds yet? */

  if (!g_nxhello.havepos)
    {
      /* Save the background window handle */

      g_nxhello.hbkgd = hwnd;

      /* Save the window limits */

      g_nxhello.xres = bounds->pt2.x + 1;
      g_nxhello.yres = bounds->pt2.y + 1;

      g_nxhello.havepos = true;
      sem_post(&g_nxhello.sem);
      gvdbg("Have xres=%d yres=%d\n", g_nxhello.xres, g_nxhello.yres);
    }
}

/****************************************************************************
 * Name: nxhello_mousein
 ****************************************************************************/

#ifdef CONFIG_NX_XYINPUT
static void nxhello_mousein(NXWINDOW hwnd, FAR const struct nxgl_point_s *pos,
                         uint8_t buttons, FAR void *arg)
{
  printf("nxhello_mousein: hwnd=%p pos=(%d,%d) button=%02x\n",
         hwnd,  pos->x, pos->y, buttons);
}
#endif

/****************************************************************************
 * Name: nxhello_kbdin
 ****************************************************************************/

#ifdef CONFIG_NX_KBD
static void nxhello_kbdin(NXWINDOW hwnd, uint8_t nch, FAR const uint8_t *ch,
                       FAR void *arg)
{
  gvdbg("hwnd=%p nch=%d\n", hwnd, nch);

   /* In this example, there is no keyboard so a keyboard event is not
    * expected.
    */

   printf("nxhello_kbdin: Unexpected keyboard callback\n");
}
#endif

/****************************************************************************
 * Name: nxhello_center
 ****************************************************************************/

static void nxhello_center(FAR struct nxgl_point_s *pos,
                           FAR const struct nx_font_s *fontset)
{
  FAR const struct nx_fontbitmap_s *fbm;
  FAR const char *ptr;
  unsigned int width;

  /* Get the width of the collection of characters so that we can center the
   * hello world message.
   */
  
  printf(".................the length of a en cha: %d\n",strlen(g_hello));

  unsigned char gbk_str[GBKLEN];

  u2g(g_hello,strlen(g_hello),gbk_str,GBKLEN);


  /* Now we know how to center the string.  Create a the position and
   * the bounding box
   */

  //pos->x = (g_nxhello.xres - width) / 3;
  //pos->y = (g_nxhello.yres - fontset->mxheight) / 2;
  pos->x = 0;
  pos->y = 0;
}

/****************************************************************************
 * Name: nxhello_initglyph
 * 初始化每一个字符所在bgd上重叠层的背景色
 ****************************************************************************/

static void nxhello_initglyph(FAR uint8_t *glyph, uint8_t height,
                              uint8_t width, uint8_t stride)
{
  FAR nxgl_mxpixel_t *ptr;
#if CONFIG_EXAMPLES_NXHELLO_BPP < 8
  nxgl_mxpixel_t pixel;
#endif
  unsigned int row;
  unsigned int col;

  /* Initialize the glyph memory to the background color */

#if CONFIG_EXAMPLES_NXHELLO_BPP < 8

  pixel  = CONFIG_EXAMPLES_NXHELLO_BGCOLOR;

#if CONFIG_NX_NPLANES > 1
# warning "More logic is needed for the case where CONFIG_NX_PLANES > 1"
#endif
#  if CONFIG_EXAMPLES_NXHELLO_BPP == 1
  /* Pack 1-bit pixels into a 2-bits */

  pixel &= 0x01;
  pixel  = (pixel) << 1 |pixel;

#  endif
#  if CONFIG_EXAMPLES_NXHELLO_BPP < 4

  /* Pack 2-bit pixels into a nibble */

  pixel &= 0x03;
  pixel  = (pixel) << 2 |pixel;

#  endif

  /* Pack 4-bit nibbles into a byte */

  pixel &= 0x0f;
  pixel  = (pixel) << 4 | pixel;

  ptr    = (FAR nxgl_mxpixel_t *)glyph;
  for (row = 0; row < height; row++)
    {
      for (col = 0; col < stride; col++)
        {
          /* Transfer the packed bytes into the buffer */

          *ptr++ = pixel;
        }
    }

#elif CONFIG_EXAMPLES_NXHELLO_BPP == 24
# error "Additional logic is needed here for 24bpp support"

#else /* CONFIG_EXAMPLES_NXHELLO_BPP = {8,16,32} */

  ptr = (FAR nxgl_mxpixel_t *)glyph;
  for (row = 0; row < height; row++)
    {
      /* Just copy the color value into the glyph memory */
      for (col = 0; col < width; col++)
        {
          *ptr++ = CONFIG_EXAMPLES_NXHELLO_BGCOLOR;
#if CONFIG_NX_NPLANES > 1
# warning "More logic is needed for the case where CONFIG_NX_PLANES > 1"
#endif
        }
    }
#endif
}

 /****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxhello_hello
 *
 * Description:
 *   Print "Hello, World!" in the center of the display.
 *
 ****************************************************************************/

void d_nxhello(NXWINDOW hwnd){
	nxhello_hello(hwnd,g_hello,1);
	nxhello_hello(hwnd,g_hello,2);
	nxhello_hello(hwnd,g_hello,3);
}

void nxhello_hello(NXWINDOW hwnd,const char* str, int sign)
{
  FAR const struct nx_font_s *fontset;
  FAR const struct nx_fontbitmap_s *fbm;
  FAR uint8_t *glyph;
  FAR const unsigned char *ptr;
  FAR struct nxgl_point_s pos;
  FAR struct nxgl_rect_s dest;
  FAR struct nxgl_rect_s test_rec;
  FAR const void *src[CONFIG_NX_NPLANES];
  unsigned int glyphsize;
  unsigned int mxstride;
  int ret;
  FAR struct nxgl_point_s test_pos;

  /* Get information about the font we are going to use */

  if(sign == 1)
	  fontset = nxf_getfontset(g_nxhello.hfont);
  else if(sign == 2) 
	  fontset = nxf_getfontset(g_nxhello.hfont2);
  else {
	  fontset = nxf_getfontset(g_nxhello.hfont3);
  }


  /* Allocate a bit of memory to hold the largest rendered font */

  //glyph申请空间, 潜在问题//TODO
  mxstride  = (fontset->mxwidth * CONFIG_EXAMPLES_NXHELLO_BPP + 7) >> 3;
  glyphsize = (unsigned int)fontset->mxheight * mxstride;
  glyph     = (FAR uint8_t*)malloc(glyphsize);

  /* NOTE: no check for failure to allocate the memory.  In a real application
   * you would need to handle that event.
   */

  /* Get a position so the the "Hello, World!" string will be centered on the
   * display.
   */

  if(sign == 1){
	  pos.x = 0;
	  pos.y = 0;
  }
  else if(sign == 2){
	  pos.x = 0;
	  pos.y = 50;
  }
  else {
	  pos.x = 0;
	  pos.y = 100;
  }
  printf("nxhello_hello: Position (%d,%d)\n", pos.x, pos.y);
 

  unsigned char gbk_str[GBKLEN];

  u2g(str,strlen(str),gbk_str,GBKLEN); //convert utf to gb2312 code
	

  /* Now we can say "hello" in the center of the display. */
  for (ptr = (uint8_t*)gbk_str; *ptr;)
    {
      /* Get the bitmap font for this ASCII code */
	  //获取每一个字符的位图信息
	  if((uint8_t)(*ptr) < 0x80){  //en chara
		  printf("en code is %x......\n",*ptr);
		  if(sign == 1)
			 fbm = nxf_getbitmap(g_nxhello.hfont, *ptr);
		  else if(sign == 2)
			 fbm = nxf_getbitmap(g_nxhello.hfont2, *ptr);
		  else 
			 fbm = nxf_getbitmap(g_nxhello.hfont3, *ptr);
		 ptr++;
	  }
	  else {    //cn chara
		  uint16_t cn_code = *(ptr+1)+((*ptr)<<8);
		  gdbg("cn code is %02x............\n",cn_code);
		  if(sign == 1)
			  fbm = nxf_getbitmap(g_nxhello.hfont, cn_code);
		  else if(sign == 2)
			  fbm = nxf_getbitmap(g_nxhello.hfont2, cn_code);
		  else 
			  fbm = nxf_getbitmap(g_nxhello.hfont3, cn_code);
		  ptr += 2;
	  }
      if (fbm)
        {
          uint8_t fheight;      /* Height of this glyph (in rows) */
          uint8_t fwidth;       /* Width of this glyph (in pixels) */
          uint8_t fstride;      /* Width of the glyph row (in bytes) */

          /* Get information about the font bitmap */
		  //每一个字符处于一个glyph中,宽度是字符宽加上字符相对glyph的x,y偏移
          fwidth  = fbm->metric.width + fbm->metric.xoffset;
          fheight = fbm->metric.height + fbm->metric.yoffset;
          fstride = (fwidth * CONFIG_EXAMPLES_NXHELLO_BPP + 7) >> 3;

          /* Initialize the glyph memory to the background color */

          nxhello_initglyph(glyph, fheight, fwidth, fstride);

          /* Then render the glyph into the allocated memory */

#if CONFIG_NX_NPLANES > 1
# warning "More logic is needed for the case where CONFIG_NX_PLANES > 1"
#endif
          (void)RENDERER((FAR nxgl_mxpixel_t*)glyph, fheight, fwidth,
                         fstride, fbm, CONFIG_EXAMPLES_NXHELLO_FONTCOLOR);

          /* Describe the destination of the font with a rectangle */
          dest.pt1.x = pos.x;
          dest.pt1.y = pos.y;
          dest.pt2.x = pos.x + fwidth - 1;
          dest.pt2.y = pos.y + fheight - 1;

          /* Then put the font on the display */

          src[0] = (FAR const void *)glyph;
#if CONFIG_NX_NPLANES > 1
# warning "More logic is needed for the case where CONFIG_NX_PLANES > 1"
#endif
		  /**  nx_bitmap()
		   * Description: Copy a rectangular region of a larger image into the rectangle in the specified window.
		   *
		   * Input Parameters:
		   *
		   * hwnd
		   * The handle returned by nx_openwindow() or nx_requestbkgd() that specifies the window that will receive the bitmap image.
		   * dest
		   * Describes the rectangular on the display that will receive the bit map.
		   * src
		   * The start of the source image. This is an array source images of size CONFIG_NX_NPLANES (probably 1).
		   * origin
		   * The origin of the upper, left-most corner of the full bitmap. Both dest and origin are in window coordinates, however, the origin may lie outside of the display.
		   * stride
		   * The width of the full source image in bytes.*/
          ret = nx_bitmap((NXWINDOW)hwnd, &dest, src, &pos, fstride);
          if (ret < 0)
            {
              printf("nxhello_write: nx_bitmapwindow failed: %d\n", errno);
            }

           /* Skip to the right the width of the font */

          pos.x += fwidth;
        }
      else
        {
           /* No bitmap (probably because the font is a space).  Skip to the
            * right the width of a space.
            */

          pos.x += fontset->spwidth;
        }
    }

  /* Free the allocated glyph */

  free(glyph);
}



//代码转换:从一种编码转为另一种编码
int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
{
	iconv_t cd;
	int rc;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0) {
		return -1;
		printf("open failed....\n");
	}
	memset(outbuf,0,outlen);
	if (iconv(cd,pin,&inlen,pout,&outlen)==-1){
	   	return -1;
	}
	iconv_close(cd);
	return 0;
}

//UNICODE码转为GB2312码
int u2g(const char *inbuf,int inlen,char *outbuf,int outlen)
{
		return code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
}
