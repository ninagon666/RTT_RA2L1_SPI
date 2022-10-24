/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author           Notes
 * 2022-10-18     Ninagon666       the first version
 */

#include <board.h>
#include <rtthread.h>
#include <drivers/pin.h>
#include <drivers/spi.h>
#include <rtdbg.h>
#include "hal_data.h"
#include "drv_lcd.h"

#define LCD_CS   "P500"
#define LCD_DC   "P501"
#define LCD_RST  "P100"
#define LCD_BLK  "P103"
                                        
extern void rt_hw_spi_device_attach(struct rt_spi_device *device, 
                                    const char *device_name, 
                                    const char *bus_name, 
                                    void *user_data);
                                    
extern rt_err_t rt_spi_configure(struct rt_spi_device *device,
                          struct rt_spi_configuration *cfg);
                                    
static void rt_spi_device_attach(const char *bus_name, 
                                 const char *device_name, 
                                 rt_uint32_t *cs_pin);
  
int spi10_device_init(void)
{
    rt_uint32_t lcd_cs_pin = rt_pin_get(LCD_CS);
    rt_pin_mode(lcd_cs_pin, PIN_MODE_OUTPUT);
    //设备挂载到SPI总线，抽象为 spi10 设备
    rt_spi_device_attach("spi0", "spi10", (rt_uint32_t *)lcd_cs_pin);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(spi10_device_init);


void rt_spi_device_attach(const char *bus_name, const char *device_name, rt_uint32_t *cs_pin)
{
    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    struct rt_spi_device *spi_device;
    struct rt_spi_configuration cfg;
    
    /* attach the device to spi bus*/
    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    RT_ASSERT(spi_device != RT_NULL);
  
    rt_hw_spi_device_attach(spi_device, device_name, bus_name, (void *)cs_pin);
    
    cfg.data_width = 8;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = 16 * 1000 * 1000;               /* 16M */

    rt_spi_configure(spi_device, &cfg);
}

static void lcd_app(void *parameter);

int lcd_thread(void)
{
    rt_thread_t tid;
  
    rt_uint32_t lcd_dc_pin  = rt_pin_get(LCD_DC);
    rt_uint32_t lcd_rst_pin = rt_pin_get(LCD_RST);
    rt_uint32_t lcd_blk_pin = rt_pin_get(LCD_BLK);
  
    rt_pin_mode(lcd_dc_pin,  PIN_MODE_OUTPUT);
    rt_pin_mode(lcd_rst_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(lcd_blk_pin, PIN_MODE_OUTPUT);
    
    rt_pin_write(lcd_blk_pin, PIN_LOW);//开启lcd背光，手里有的模组是低电平点亮，实际情况要参考模组原理图

    if(lcd_init("spi10", lcd_dc_pin, lcd_rst_pin) == RT_EOK) {
        lcd_clear(WHITE);
        rt_kprintf("lcd init ok\r\n");
    }
    else {
        rt_kprintf("lcd init fail\r\n");
        return RT_ERROR;
    }
    
    tid = rt_thread_create("lcd task", lcd_app, RT_NULL, 4096, 12, 10);
    if (RT_NULL != tid)
          rt_thread_startup(tid);

    return RT_EOK;
}
INIT_APP_EXPORT(lcd_thread);

#define Rect_W 30
#define Rect_H 24

void lcd_test1(void)
{
    lcd_draw_rect(40 - Rect_W, 32 - Rect_H, 40 + Rect_W, 32 + Rect_H, BLUE);
    lcd_draw_rect(120 - Rect_W, 32 - Rect_H, 120 + Rect_W, 32 + Rect_H, YELLOW);
    lcd_draw_rect(40 - Rect_W, 96 - Rect_H, 40 + Rect_W, 96 + Rect_H, GRAY);
    lcd_draw_rect(120 - Rect_W, 96 - Rect_H, 120 + Rect_W, 96 + Rect_H, GREEN);
    lcd_draw_rect(80 - Rect_W, 64 - Rect_H, 80 + Rect_W, 64 + Rect_H, PINK);
    
    rt_thread_mdelay(1000);
    lcd_clear(WHITE);
  
    lcd_draw_circ(80, 64, 5 * 2, RED);
    lcd_draw_circ(80, 64, 10 * 2, GREEN);
    lcd_draw_circ(80, 64, 15 * 2, BLUE);
    lcd_draw_circ(80, 64, 20 * 2, GRAY);
    lcd_draw_circ(80, 64, 25 * 2, BROWN);
    lcd_draw_circ(80, 64, 30 * 2, PINK);
    
    rt_thread_mdelay(1000);
    lcd_clear(WHITE);
  
    lcd_showpicture(0, 0, 160, 80, RT_Thread_logo_map);
    lcd_showstr(20, 5, (uint8_t *)"Powered by");
    lcd_showstr(40, 6, (uint8_t *)"Renesas RA2L1");

    return ;
}
MSH_CMD_EXPORT(lcd_test1, "lcd basic test");

void lcd_test2(void)
{
//    rt_tick_t tick_start, tick_end;
//    tick_start = rt_tick_get();
    for(uint8_t i = 0; i < 2; ++i)
    {
        lcd_clear(RED);
        lcd_clear(GREEN);
        lcd_clear(BLUE);
    }
//    tick_end = rt_tick_get();
//    rt_kprintf("clear screen use %d ms\n", (tick_end - tick_start) / 9);
    lcd_clear(WHITE);
    return ;
}
MSH_CMD_EXPORT(lcd_test2, "lcd test fill RGB");

void lcd_app(void *parameter)
{
    
    while(1)
    {
      //unused
      rt_thread_mdelay(10);//让出线程
    }
}
