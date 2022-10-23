#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__

/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author           Notes
 * 2022-10-18     Ninagon666       the first version
 */

#include <rtdevice.h>
#include <rtthread.h>
#include <stdint.h>
#include "font.h"

#define USE_HORIZONTAL 3 //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
  #define LCD_TFTWIDTH  128
  #define LCD_TFTHEIGHT 160
#else
  #define LCD_TFTWIDTH  160
  #define LCD_TFTHEIGHT 128
#endif

//-------常用颜色----------
#define RED     0xF800	  //红色
#define BLUE    0x001F	  //蓝色
#define YELLOW  0xFFE0    //黄色
#define GREEN   0x07E0    //绿色
#define WHITE   0xFFFF    //白色
#define BLACK   0x0000    //黑色
#define GRAY    0X8430	  //灰色
#define BROWN   0XBC40    //棕色
#define PURPLE  0XF81F    //紫色
#define PINK    0XFE19	  //粉色

#define PEN_COLOR  PURPLE  //画笔颜色
#define BG_COLOR   WHITE  //背景颜色

rt_err_t lcd_init(const char *dev_name, rt_base_t dc_pin, rt_base_t reset_pin);
void lcd_reset(void);
void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void lcd_clear(uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color);
void lcd_draw_circ(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void lcd_showchar(uint16_t x, uint16_t y, uint8_t dat);
void lcd_showstr(uint16_t x, uint16_t y, uint8_t dat[]);
void lcd_showpicture(uint16_t x,uint16_t y,uint16_t width,uint16_t hight,const uint8_t picture[]);

#endif
