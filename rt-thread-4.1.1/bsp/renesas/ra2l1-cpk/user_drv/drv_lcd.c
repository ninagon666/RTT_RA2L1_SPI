/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author           Notes
 * 2022-10-18     Ninagon666       the first version
 */

#include "drv_lcd.h"

#define DBG_TAG           "st7735"  
#define DBG_LVL           DBG_INFO  //开启的debug级别
#include <rtdbg.h>                  //must after of DBG_LVL, DBG_TAG or other options

struct lcd_device
{
    struct rt_device parent;

    struct rt_device_graphic_info lcd_info;

    struct rt_semaphore lcd_lock;

    struct rt_spi_device *spi_device;

    rt_base_t reset_pin;
    rt_base_t dc_pin;
};

static struct lcd_device _lcd;

static const uint8_t init_cmd[] = {
    0xB1, 3, 0x05, 0x3C, 0x3C,
    0xB2, 3, 0x05, 0x3C, 0x3C,
    0xB3, 6, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C,
    0xB4, 1, 0x03,
    0xC0, 3, 0x28, 0x08, 0x04,
    0xC1, 1, 0xC0,
    0xC2, 2, 0x0D, 0x00,
    0xC3, 2, 0x8D, 0x2A,
    0xC4, 2, 0x8D, 0xEE,
    0xC5, 1, 0x1A,
#if(USE_HORIZONTAL==0)
    0x36, 1, 0x00,
#elif(USE_HORIZONTAL==1)
    0x36, 1, 0xC0,
#elif(USE_HORIZONTAL==2)
    0x36, 1, 0x70,
#elif(USE_HORIZONTAL==3)
    0x36, 1, 0xA0,
#endif
    0xE0, 16, 0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A, 
              0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13,
    0xE1, 16, 0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27, 
              0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13,
    0x3A, 1, 0x05,
    0x29, 0,
    0x00
};

static rt_err_t lcd_device_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd)
    {
    case RTGRAPHIC_CTRL_GET_INFO:
        RT_ASSERT(args != RT_NULL);
        *(struct rt_device_graphic_info *)args = _lcd.lcd_info;
        break;
    }

    return RT_EOK;
}

void lcd_send_command_data(uint8_t cmd, uint8_t *data, rt_size_t data_len)
{
    /* Set D/C pin to 0 */
    rt_pin_write(_lcd.dc_pin, PIN_LOW);
    rt_spi_send(_lcd.spi_device, &cmd, 1);

    if (data_len)
    {
        rt_pin_write(_lcd.dc_pin, PIN_HIGH);
        rt_spi_send(_lcd.spi_device, data, data_len);
    }
}

void lcd_send_data(uint8_t *data, rt_size_t data_len)
{
    rt_pin_write(_lcd.dc_pin, PIN_HIGH);
    rt_spi_send(_lcd.spi_device, data, data_len);
}

rt_err_t lcd_init(const char *dev_name, rt_base_t dc_pin,
                      rt_base_t reset_pin)
{
    rt_err_t result = RT_EOK;
    struct rt_device *device = &_lcd.parent;

    _lcd.spi_device = (struct rt_spi_device *)rt_device_find(dev_name);
    if (_lcd.spi_device == RT_NULL)
    {
        LOG_E("can not find %s device", dev_name);
        return -RT_ERROR;
    }

    _lcd.reset_pin = reset_pin;
    _lcd.dc_pin = dc_pin;
    rt_pin_mode(_lcd.reset_pin, PIN_MODE_OUTPUT);
    rt_pin_mode(_lcd.dc_pin, PIN_MODE_OUTPUT);
    lcd_reset();
    lcd_send_command_data(0x11, RT_NULL, 0);//Sleep out
    rt_thread_mdelay(100);

    /* init ST7735 */
    uint8_t cmd, x, num_args;
    uint8_t *addr = (uint8_t *)init_cmd;
    while ((cmd = *(addr++)) > 0)
    {
        x = *(addr++);
        num_args = x & 0x7F;
        lcd_send_command_data(cmd, addr, num_args);
        addr += num_args;
        if (x & 0x80)
            rt_thread_mdelay(150);
    }

    /* init lcd_lock semaphore */
    result = rt_sem_init(&_lcd.lcd_lock, "lcd_lock", 0, RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        LOG_E("init semaphore failed!\n");
        result = -RT_ENOMEM;
        goto __exit;
    }

    /* config LCD dev info */
    _lcd.lcd_info.height = LCD_TFTHEIGHT;
    _lcd.lcd_info.width = LCD_TFTWIDTH;
    _lcd.lcd_info.bits_per_pixel = 16;
    _lcd.lcd_info.pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    _lcd.lcd_info.framebuffer = RT_NULL;

    device->type = RT_Device_Class_Graphic;
#ifdef RT_USING_DEVICE_OPS
    device->ops = &lcd_ops;
#else
    device->init = RT_NULL;
    device->control = lcd_device_control;
#endif

    /* register lcd device */
    rt_device_register(device, "lcd", RT_DEVICE_FLAG_RDWR);

__exit:
    return result;
}

void lcd_reset(void)
{
    rt_pin_write(_lcd.reset_pin, PIN_LOW);
    rt_thread_mdelay(20);
    rt_pin_write(_lcd.reset_pin, PIN_HIGH);
    rt_thread_mdelay(200);
}

#if(USE_HORIZONTAL==0 || USE_HORIZONTAL==1)
    #define Column_offset 2
    #define Row_offset    1
#elif(USE_HORIZONTAL==2 || USE_HORIZONTAL==3)
    #define Column_offset 1
    #define Row_offset    2
#endif

#define ST7735_CASET 0x2A  ///< Column Address Set
#define ST7735_PASET 0x2B  ///< Page Address Set
#define ST7735_RAMWR 0x2C  ///< Memory Write

void lcd_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint16_t cmd1, cmd2;
    uint8_t data_buf[4];
  
    cmd1 = x1 + Column_offset;
    cmd2 = x2 + Column_offset;

    data_buf[0] = (uint8_t)(cmd1 >> 8);
    data_buf[1] = (uint8_t)(cmd1 & 0xFF);
    data_buf[2] = (uint8_t)(cmd2 >> 8);
    data_buf[3] = (uint8_t)(cmd2 & 0xFF);
    lcd_send_command_data(ST7735_CASET, data_buf, 4); // Column address set
  
    cmd1 = y1 + Row_offset;
    cmd2 = y2 + Row_offset;

    data_buf[0] = (uint8_t)(cmd1 >> 8);
    data_buf[1] = (uint8_t)(cmd1 & 0xFF);
    data_buf[2] = (uint8_t)(cmd2 >> 8);
    data_buf[3] = (uint8_t)(cmd2 & 0xFF);
    lcd_send_command_data(ST7735_PASET, data_buf, 4); // Row address set
    lcd_send_command_data(ST7735_RAMWR, RT_NULL, 0);  // Write to RAM
}

void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;
    uint8_t data_buf[2];
    
    data_buf[0] = color >> 8;
    data_buf[1] = (uint8_t)(color & 0xFF);
    
    lcd_set_address(xsta, ysta, xend - 1, yend - 1);
    for (i = ysta; i < yend; i++)
          for (j = xsta; j < xend; j++)
              lcd_send_data(data_buf, 2);
}

inline void lcd_clear(uint16_t color)
{
    lcd_fill(0, 0, LCD_TFTWIDTH, LCD_TFTHEIGHT, color);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data_buf[2];
  
    data_buf[0] = color >> 8;
    data_buf[1] = (uint8_t)(color & 0xFF);
  
    lcd_set_address(x, y, x, y);
    lcd_send_data(data_buf, 2);
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1; //画线起点坐标
    uCol = y1;
    if (delta_x > 0)
        incx = 1; //设置单步方向
    else if (delta_x == 0)
        incx = 0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴
    else
        distance = delta_y;
    for (t = 0; t < distance + 1; t++)
    {
        lcd_draw_pixel(uRow, uCol, color); //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

void lcd_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

void lcd_draw_circ(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    a = 0;
    b = r;
    while (a <= b)
    {
        lcd_draw_pixel(x0 - b, y0 - a, color); // 3
        lcd_draw_pixel(x0 + b, y0 - a, color); // 0
        lcd_draw_pixel(x0 - a, y0 + b, color); // 1
        lcd_draw_pixel(x0 - a, y0 - b, color); // 2
        lcd_draw_pixel(x0 + b, y0 + a, color); // 4
        lcd_draw_pixel(x0 + a, y0 - b, color); // 5
        lcd_draw_pixel(x0 + a, y0 + b, color); // 6
        lcd_draw_pixel(x0 - b, y0 + a, color); // 7
        a++;
        if ((a * a + b * b) > (r * r)) //判断要画的点是否过远
        {
            b--;
        }
    }
}

void lcd_showchar(uint16_t x, uint16_t y, uint8_t dat)
{
    uint8_t temp, data_buf[2];

    for (uint8_t i = 0; i < 16; ++i)
    {
        lcd_set_address(x, y + i, x + 7, y + i);
        
        temp = tft_ascii[dat - 32][i];   //减32因为是取模是从空格开始取得 空格在ascii中序号是32
        for (uint8_t j = 0; j < 8; ++j)
        {
            if (temp & 0x01)
            {
                data_buf[0] = PEN_COLOR >> 8;
                data_buf[1] = (uint8_t)(PEN_COLOR & 0xFF);
                lcd_send_data(data_buf, 2);
            }
            else
            {
                data_buf[0] = BG_COLOR >> 8;
                data_buf[1] = (uint8_t)(BG_COLOR & 0xFF);
                lcd_send_data(data_buf, 2);
            }
            temp >>= 1;  //一位一位的取出字模数据
        }
    }
}

void lcd_showstr(uint16_t x, uint16_t y, uint8_t dat[])
{
    uint16_t j = 0;

    while (dat[j] != '\0')
    {
        lcd_showchar(x + 8 * j, y * 16, dat[j]);
        ++j;
    }
}

void lcd_showpicture(uint16_t x,uint16_t y,uint16_t width,uint16_t hight,const uint8_t picture[])
{
    uint32_t k=0;
    
    lcd_set_address(x, y, x + width - 1, y + hight - 1);
    
    for(uint16_t i = 0;i < width; ++i)
    {
        for(uint16_t j = 0; j < hight; ++j)
        {
            lcd_send_data((uint8_t *)&picture[k * 2], 1);
            lcd_send_data((uint8_t *)&picture[k * 2 + 1], 1);
            ++k;
        }
    }			
}

