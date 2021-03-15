/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-15     Administrator       the first version
 */
#include "led.h"
#include <drv_common.h>


#define LED1_PIN        GET_PIN(B, 5)

#define LED_ON(ld)     led_mode(ld, 10,100,0)
#define LED_OFF(ld)    led_mode(ld, 10,0,0)
#define LED_TOGGLE(ld) led_mode(ld, 0,0,0)
#define LED_BLINK(ld)  led_mode(ld, 10,50,0xffffffff)
#define LED_COUNT(ld)  led_mode(ld, 10,50,5)

static int led_sample(void)
{
    int led_test = led_register(LED1_PIN, PIN_HIGH);

    LED_ON(led_test);          /* 常亮 */
    rt_thread_mdelay(3000);
    LED_OFF(led_test);         /* 熄灭 */
    rt_thread_mdelay(3000);
//    LED_BEEP(led_test);        /* 闪烁三下 */
//    rt_thread_mdelay(5000);
//    LED_BLINK(led_test);       /* 持续闪烁 */

    led_unregister(led_test);
}
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(led_sample, Driver LED based on led);
#endif
