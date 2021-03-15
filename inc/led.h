/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-15     Administrator       the first version
 */
#ifndef APPLICATIONS_MYDRIVER_LED_H_
#define APPLICATIONS_MYDRIVER_LED_DRIVER_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdbg.h>


//led节点
typedef struct led_node
{
    int ld;
    rt_base_t pin;
    rt_base_t logic;

    rt_uint32_t frequency;
    rt_uint8_t  duty;
    rt_uint32_t count;

    struct rt_thread *tid;

    rt_mutex_t  lock;
    rt_slist_t  list;
}LED_NODE_Typedef;

//led头
typedef struct led_list_head
{
    rt_uint32_t ld_max;//当前注册 LED 的最大描述符
    rt_mutex_t  lock;
    rt_slist_t  head;
}LED_LIST_HEAD_Typedef;


typedef struct led_msg
{
    int ld;
    rt_uint32_t frequency;
    rt_uint32_t duty;
    rt_uint32_t count;
}LED_MSG_typedef;


int led_register(rt_base_t pin, rt_base_t logic);
void led_unregister(int ld);
int led_mode(int ld, rt_uint32_t frequency, rt_uint8_t duty,  rt_uint32_t count);

#endif /* APPLICATIONS_MYDRIVER_LED_H_ */
