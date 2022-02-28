/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-25     kyle      the first version
 */

#ifndef __MULTI_RTIMER_H__
#define __MULTI_RTIMER_H__

#include <rtconfig.h>
#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTIMER_TICK_PER_SECOND  10000

/**
 * timer structure
 */
struct multi_rtimer
{
    struct rt_object parent;                            /**< inherit from rt_object */

    rt_list_t        row[RT_TIMER_SKIP_LIST_LEVEL];

    void (*timeout_func)(void *parameter);              /**< timeout function */
    void            *parameter;                         /**< timeout function's parameter */

    rt_tick_t        init_tick;                         /**< timer timeout tick */
    rt_tick_t        timeout_tick;                      /**< timeout tick */
};
typedef struct multi_rtimer *multi_rtimer_t;


rt_tick_t rtimer_tick_get(void);
void rtimer_tick_set(rt_tick_t tick);
void rtimer_tick_increase(void);
rt_tick_t  rtimer_tick_from_millisecond(rt_int32_t ms);

void rt_system_multi_rtimer_init(void);

void multi_rtimer_init(multi_rtimer_t  timer,
                   const char *name,
                   void (*timeout)(void *parameter),
                   void       *parameter,
                   rt_tick_t   time,
                   rt_uint8_t  flag);
rt_err_t multi_rtimer_detach(multi_rtimer_t timer);
multi_rtimer_t multi_rtimer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void       *parameter,
                           rt_tick_t   time,
                           rt_uint8_t  flag);
rt_err_t multi_rtimer_delete(multi_rtimer_t timer);
rt_err_t multi_rtimer_start(multi_rtimer_t timer);
rt_err_t multi_rtimer_stop(multi_rtimer_t timer);
rt_err_t multi_rtimer_control(multi_rtimer_t timer, int cmd, void *arg);

rt_tick_t multi_rtimer_next_timeout_tick(void);
void multi_rtimer_check(void);

#ifdef RT_USING_HOOK
void multi_rtimer_enter_sethook(void (*hook)(struct multi_rtimer *timer));
void multi_rtimer_exit_sethook(void (*hook)(struct multi_rtimer *timer));
#endif


/**@}*/

#ifdef __cplusplus
}
#endif

#endif
