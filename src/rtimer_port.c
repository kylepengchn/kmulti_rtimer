/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-25     kyle      the first version
 */

#include <multi_rtimer.h>
#include <rtdevice.h>

#define DBG_TAG              "rtimer"
#define DBG_LVL              DBG_ERROR
#include <rtdbg.h>

#define RTIMER_CHECK_PERIOD (1000000 / RTIMER_TICK_PER_SECOND) /* usec */


static rt_tick_t rtimer_tick = 0;

/**
 * This function will return current tick from operating system startup
 *
 * @return current tick
 */
rt_tick_t rtimer_tick_get(void)
{
    /* return the global tick */
    return rtimer_tick;
}
RTM_EXPORT(rtimer_tick_get);

/**
 * This function will set current tick
 */
void rtimer_tick_set(rt_tick_t tick)
{
//    rt_base_t level;

//    level = rt_hw_interrupt_disable();
    rtimer_tick = tick;
//    rt_hw_interrupt_enable(level);
}

/**
 * This function will notify kernel there is one tick passed. Normally,
 * this function is invoked by clock ISR.
 */
void rtimer_tick_increase(void)
{
    /* increase the global tick */
    ++ rtimer_tick;

    /* check timer */
    multi_rtimer_check();
}

/**
 * This function will calculate the tick from millisecond.
 *
 * @param ms the specified millisecond
 *           - Negative Number wait forever
 *           - Zero not wait
 *           - Max 0x7fffffff
 *
 * @return the calculated tick
 */
rt_tick_t rtimer_tick_from_millisecond(rt_int32_t ms)
{
    rt_tick_t tick;

    if (ms < 0)
    {
        tick = (rt_tick_t)RT_WAITING_FOREVER;
    }
    else
    {
        tick = RT_TICK_PER_SECOND * (ms / 1000);
        tick += (RT_TICK_PER_SECOND * (ms % 1000) + 999) / 1000;
    }

    /* return the calculated tick */
    return tick;
}
RTM_EXPORT(rtimer_tick_from_millisecond);

static rt_err_t rtimer_tick_timeout(rt_device_t dev, rt_size_t size)
{
    rtimer_tick_increase();

    return RT_EOK;  
}

static int rtimer_port_init(void)
{
    rt_err_t ret = RT_EOK;

    rt_system_multi_rtimer_init();

    rt_device_t hw_dev = RT_NULL;
    rt_uint32_t freq = 60000000;
    rt_hwtimer_mode_t mode = HWTIMER_MODE_PERIOD;
    rt_hwtimerval_t timeout_s = {.sec = 0, .usec = RTIMER_CHECK_PERIOD};

    /* find dev */
    hw_dev = rt_device_find(PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
    if (!hw_dev)
    {
        LOG_E("cant`t find %s device", PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
        return -RT_ERROR;
    }
    
    /* open dev */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        LOG_E("open %s device failed", PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
        return ret;
    }

    /* set rx indicate */
    rt_device_set_rx_indicate(hw_dev, rtimer_tick_timeout);
    
    /* set freq */
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
    if (ret != RT_EOK)
    {
        LOG_E("set %s device freq failed", PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
        return ret;
    }

    /* set mode */
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        LOG_E("set %s device mode failed", PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
        return ret;
    }

    /* set timer timeout value */
    if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        LOG_E("set %s device timeout value failed", PKG_MULTI_TIMER_HWTIMER_DEV_NAME);
        return -RT_ERROR;
    }

    return ret;
}
INIT_PREV_EXPORT(rtimer_port_init);
