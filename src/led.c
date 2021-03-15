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


#define LED_THREAD_PRIORITY      (RT_THREAD_PRIORITY_MAX - 1)
#define LED_THREAD_STACK_SIZE    (512)
#define LED_THREAD_TIMESLICE     (10)

#define MAX_MSGS                 (5)



//led链表
static struct led_list_head led_list;
//led线程
static rt_thread_t led_thread = RT_NULL;
//led消息队列
static rt_mq_t led_mq = RT_NULL;


//led注册
int led_register(rt_base_t pin, rt_base_t logic)
{
    /* Create node */
    struct led_node *new_led = (struct led_node *)rt_calloc(1, sizeof(struct led_node));
    if (new_led == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    new_led->pin = pin;
    new_led->logic = logic;

    /* Operate list */
    rt_mutex_take(led_list.lock, RT_WAITING_FOREVER);

    rt_slist_append(&led_list.head, &new_led->list);
    new_led->ld = led_list.ld_max++;

    LOG_D("led [%d] register, len = %d", new_led->ld, rt_slist_len(&led_list.head));

    rt_mutex_release(led_list.lock);

    /* Init pin */
    rt_pin_mode(new_led->pin, PIN_MODE_OUTPUT);
    rt_pin_write(new_led->pin, !new_led->logic);

    return new_led->ld;
}
//led注销
void led_unregister(int ld)
{
    rt_slist_t *node = RT_NULL;
    struct led_node *led = RT_NULL;

    rt_mutex_take(led_list.lock, RT_WAITING_FOREVER);

    rt_slist_for_each(node, &led_list.head)
    {
        led = rt_slist_entry(node, struct led_node, list);
        if (ld == led->ld)
            break;
    }
    if (node != RT_NULL)
    {
        if (led->tid)
        {
            rt_pin_write(led->pin, !led->logic);
            rt_thread_delete(led->tid);
        }
        rt_slist_remove(&led_list.head, node);
        rt_free(led);
    }

    rt_mutex_release(led_list.lock);
}

//led控制
int led_mode(int ld, rt_uint32_t frequency, rt_uint8_t duty,  rt_uint32_t count)
{
    /* pack msg */
    struct led_msg msg;

    msg.ld     = ld;
    msg.frequency = frequency;
    msg.duty  = duty;
    msg.count  = count;

    /* send message queue */
    rt_mq_send(led_mq, (void *)&msg, sizeof(msg));

    return RT_EOK;
}

///////////////////////////////////////////////////////////////////////////

static void led_task_entry(void *args)
{
    struct led_node *led = (struct led_node *)args;

    rt_base_t   pin          = led->pin;
    rt_base_t   logic = led->logic;
    rt_uint32_t frequency    = led->frequency;
    rt_uint8_t duty          = led->duty;
    rt_uint32_t count        = led->count;

    rt_uint32_t cur_count  =  0;
    rt_uint32_t time_ms    =  1000/frequency;
    rt_uint32_t time_ms_h  = time_ms/100*duty;
    rt_uint32_t time_ms_l  = time_ms-time_ms_h;;

    LOG_D("[%d] led task running", led->ld);

    if(count==0xffffffff)
    {
        while(1)
        {
            rt_pin_write(pin, logic);
            rt_thread_mdelay(time_ms_h);
            rt_pin_write(pin, !logic);
            rt_thread_mdelay(time_ms_l);
        }
    }
    else
    {
        while(cur_count < count)
        {
            rt_pin_write(pin, logic);
            rt_thread_mdelay(time_ms_h);
            rt_pin_write(pin, !logic);
            rt_thread_mdelay(time_ms_l);
            cur_count++;
        }
    }

    led->tid = RT_NULL;
}




//led后台线程
static void led_daemon_entry(void *args)
{
    struct led_msg msg;
    char led_thread_name[8];

    while(1)
    {
        /* Wait message from queue */
        if (RT_EOK == rt_mq_recv(led_mq, (void *)&msg, sizeof(msg), RT_WAITING_FOREVER))
        {
            LOG_D("led recv message");

            //创建一个led节点
            rt_slist_t *node = RT_NULL;
            struct led_node *led = RT_NULL;
            //获取互斥量
            rt_mutex_take(led_list.lock, RT_WAITING_FOREVER);

            /* Find led node */
            rt_slist_for_each(node, &led_list.head)
            {
                led = rt_slist_entry(node, struct led_node, list);
                if (msg.ld == led->ld)
                    break;
            }
            if (node == RT_NULL)
            {
                LOG_D("led node [%d] not yet registered", msg.ld);
                continue;
            }
            /* Save message */
            led->frequency = msg.frequency;
            led->duty  = msg.duty;
            led->count  = msg.count;
            /*预处理 */
            if (led->tid)
            {
                rt_thread_delete(led->tid);
                led->tid = RT_NULL;
            }


            if(led->count==0)
            {
                //只执行一次的操作
                if (led->frequency == 0)     /* frequency = 0,切换状态*/
                {
                    rt_base_t value = rt_pin_read(led->pin);
                    rt_pin_write(led->pin, !value);
                    continue;
                }
                else
                {
                    if (led->duty == 0)      /* period > 0, but duty = 0, means LED off */
                    {
                        rt_pin_write(led->pin, !led->logic);
                        continue;
                    }
                    else
                    {
                        rt_pin_write(led->pin, led->logic);
                        continue;
                    }
                }
            }
            else
            {
                /* Start thread processing task */
                rt_sprintf(led_thread_name, "led_%d", led->ld);
                rt_thread_t tid = rt_thread_create(led_thread_name, led_task_entry, led,
                                                    512, RT_THREAD_PRIORITY_MAX - 1, 5);

                rt_thread_startup(tid);
                led->tid = tid;
            }

            //释放
            rt_mutex_release(led_list.lock);
        }
    }
}



//led初始化
static int led_init(void)
{
    /* Makesure singleton */
    if (led_thread)
    {
        LOG_W("led thread has been created");
        return -RT_ERROR;
    }
    led_list.ld_max = 0;
    led_list.lock   = RT_NULL;
    led_list.head.next   = RT_NULL;

    //创建链表锁
    led_list.lock = rt_mutex_create("led", RT_IPC_FLAG_FIFO);
    if (led_list.lock == RT_NULL)
        goto __exit;

    //创建led消息队列
    led_mq = rt_mq_create("led", sizeof(struct led_msg), MAX_MSGS, RT_IPC_FLAG_FIFO);
    if (led_mq == RT_NULL)
        goto __exit;

    //创建led后台线程
    led_thread = rt_thread_create("led", led_daemon_entry, RT_NULL,
                                          1024, RT_THREAD_PRIORITY_MAX / 2, 20);
    if (led_thread == RT_NULL)
        goto __exit;

    rt_thread_startup(led_thread);
    return RT_EOK;

__exit:
    //释放申请的资源
    if (led_thread)
        rt_thread_delete(led_thread);

    if (led_mq)
        rt_mq_delete(led_mq);

    if (led_list.lock)
        rt_mutex_delete(led_list.lock);

    return -RT_ERROR;
}

#ifdef PKG_USING_LED
INIT_APP_EXPORT(led_init);
#endif


