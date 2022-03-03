/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-25     kyle      the first version
 */

#include <kmulti_rtimer.h>
#include <rtthread.h>
#include <rthw.h>

/* hard timer list */
static rt_list_t kmulti_rtimer_list[RT_TIMER_SKIP_LIST_LEVEL];

#ifdef RT_USING_HOOK
extern void (*rt_object_take_hook)(struct rt_object *object);
extern void (*rt_object_put_hook)(struct rt_object *object);
static void (*kmulti_rtimer_enter_hook)(struct kmulti_rtimer *timer);
static void (*kmulti_rtimer_exit_hook)(struct kmulti_rtimer *timer);

/**
 * @addtogroup Hook
 */

/**@{*/

/**
 * This function will set a hook function, which will be invoked when enter
 * timer timeout callback function.
 *
 * @param hook the hook function
 */
void kmulti_rtimer_enter_sethook(void (*hook)(struct kmulti_rtimer *timer))
{
    kmulti_rtimer_enter_hook = hook;
}

/**
 * This function will set a hook function, which will be invoked when exit
 * timer timeout callback function.
 *
 * @param hook the hook function
 */
void kmulti_rtimer_exit_sethook(void (*hook)(struct kmulti_rtimer *timer))
{
    kmulti_rtimer_exit_hook = hook;
}

/**@}*/
#endif

static void _kmulti_rtimer_init(kmulti_rtimer_t timer,
                           void (*timeout)(void *parameter),
                           void      *parameter,
                           rt_tick_t  time,
                           rt_uint8_t flag)
{
    int i;

    /* set flag */
    timer->parent.flag  = flag;

    /* set deactivated */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    timer->timeout_func = timeout;
    timer->parameter    = parameter;

    timer->timeout_tick = 0;
    timer->init_tick    = time;

    /* initialize timer list */
    for (i = 0; i < RT_TIMER_SKIP_LIST_LEVEL; i++)
    {
        rt_list_init(&(timer->row[i]));
    }
}

/* the fist timer always in the last row */
static rt_tick_t kmulti_rtimer_list_next_timeout(rt_list_t timer_list[])
{
    struct kmulti_rtimer *timer;
    register rt_base_t level;
    rt_tick_t timeout_tick = RT_TICK_MAX;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    if (!rt_list_isempty(&timer_list[RT_TIMER_SKIP_LIST_LEVEL - 1]))
    {
        timer = rt_list_entry(timer_list[RT_TIMER_SKIP_LIST_LEVEL - 1].next,
                              struct kmulti_rtimer, row[RT_TIMER_SKIP_LIST_LEVEL - 1]);
        timeout_tick = timer->timeout_tick;
    }

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return timeout_tick;
}

rt_inline void _kmulti_rtimer_remove(kmulti_rtimer_t timer)
{
    int i;

    for (i = 0; i < RT_TIMER_SKIP_LIST_LEVEL; i++)
    {
        rt_list_remove(&timer->row[i]);
    }
}

#if RT_DEBUG_TIMER
static int kmulti_rtimer_count_height(struct kmulti_rtimer *timer)
{
    int i, cnt = 0;

    for (i = 0; i < RT_TIMER_SKIP_LIST_LEVEL; i++)
    {
        if (!rt_list_isempty(&timer->row[i]))
            cnt++;
    }
    return cnt;
}

void kmulti_rtimer_dump(rt_list_t timer_heads[])
{
    rt_list_t *list;

    for (list = timer_heads[RT_TIMER_SKIP_LIST_LEVEL - 1].next;
         list != &timer_heads[RT_TIMER_SKIP_LIST_LEVEL - 1];
         list = list->next)
    {
        struct kmulti_rtimer *timer = rt_list_entry(list,
                                               struct kmulti_rtimer,
                                               row[RT_TIMER_SKIP_LIST_LEVEL - 1]);
        rt_kprintf("%d", kmulti_rtimer_count_height(timer));
    }
    rt_kprintf("\n");
}
#endif

/**
 * @addtogroup Clock
 */

/**@{*/

/**
 * This function will initialize a timer, normally this function is used to
 * initialize a static timer object.
 *
 * @param timer the static timer object
 * @param name the name of timer
 * @param timeout the timeout function
 * @param parameter the parameter of timeout function
 * @param time the tick of timer
 * @param flag the flag of timer
 */
void kmulti_rtimer_init(kmulti_rtimer_t  timer,
                   const char *name,
                   void (*timeout)(void *parameter),
                   void       *parameter,
                   rt_tick_t   time,
                   rt_uint8_t  flag)
{
    /* timer check */
    RT_ASSERT(timer != RT_NULL);

    /* timer object initialization */
    rt_object_init((rt_object_t)timer, RT_Object_Class_Timer, name);

    _kmulti_rtimer_init(timer, timeout, parameter, time, flag);
}
RTM_EXPORT(kmulti_rtimer_init);

/**
 * This function will detach a timer from timer management.
 *
 * @param timer the static timer object
 *
 * @return the operation status, RT_EOK on OK; RT_ERROR on error
 */
rt_err_t kmulti_rtimer_detach(kmulti_rtimer_t timer)
{
    register rt_base_t level;

    /* timer check */
    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(rt_object_get_type(&timer->parent) == RT_Object_Class_Timer);
    RT_ASSERT(rt_object_is_systemobject(&timer->parent));

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    _kmulti_rtimer_remove(timer);
    /* stop timer */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    rt_object_detach((rt_object_t)timer);

    return RT_EOK;
}
RTM_EXPORT(kmulti_rtimer_detach);

#ifdef RT_USING_HEAP
/**
 * This function will create a timer
 *
 * @param name the name of timer
 * @param timeout the timeout function
 * @param parameter the parameter of timeout function
 * @param time the tick of timer
 * @param flag the flag of timer
 *
 * @return the created timer object
 */
kmulti_rtimer_t kmulti_rtimer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void       *parameter,
                           rt_tick_t   time,
                           rt_uint8_t  flag)
{
    struct kmulti_rtimer *timer;

    /* allocate a object */
    timer = (struct kmulti_rtimer *)rt_object_allocate(RT_Object_Class_Timer, name);
    if (timer == RT_NULL)
    {
        return RT_NULL;
    }

    _kmulti_rtimer_init(timer, timeout, parameter, time, flag);

    return timer;
}
RTM_EXPORT(kmulti_rtimer_create);

/**
 * This function will delete a timer and release timer memory
 *
 * @param timer the timer to be deleted
 *
 * @return the operation status, RT_EOK on OK; RT_ERROR on error
 */
rt_err_t kmulti_rtimer_delete(kmulti_rtimer_t timer)
{
    register rt_base_t level;

    /* timer check */
    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(rt_object_get_type(&timer->parent) == RT_Object_Class_Timer);
    RT_ASSERT(rt_object_is_systemobject(&timer->parent) == RT_FALSE);

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    _kmulti_rtimer_remove(timer);
    /* stop timer */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    rt_object_delete((rt_object_t)timer);

    return RT_EOK;
}
RTM_EXPORT(kmulti_rtimer_delete);
#endif

/**
 * This function will start the timer
 *
 * @param timer the timer to be started
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t kmulti_rtimer_start(kmulti_rtimer_t timer)
{
    unsigned int row_lvl;
    rt_list_t *timer_list;
    register rt_base_t level;
    rt_list_t *row_head[RT_TIMER_SKIP_LIST_LEVEL];
    unsigned int tst_nr;
    static unsigned int random_nr;

    /* timer check */
    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(rt_object_get_type(&timer->parent) == RT_Object_Class_Timer);

    /* stop timer firstly */
    level = rt_hw_interrupt_disable();
    /* remove timer from list */
    _kmulti_rtimer_remove(timer);
    /* change status of timer */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    RT_OBJECT_HOOK_CALL(rt_object_take_hook, (&(timer->parent)));

    /*
     * get timeout tick,
     * the max timeout tick shall not great than RT_TICK_MAX/2
     */
    RT_ASSERT(timer->init_tick < RT_TICK_MAX / 2);
    timer->timeout_tick = rtimer_tick_get() + timer->init_tick;

    {
        /* insert timer to system timer list */
        timer_list = kmulti_rtimer_list;
    }

    row_head[0]  = &timer_list[0];
    for (row_lvl = 0; row_lvl < RT_TIMER_SKIP_LIST_LEVEL; row_lvl++)
    {
        for (; row_head[row_lvl] != timer_list[row_lvl].prev;
             row_head[row_lvl]  = row_head[row_lvl]->next)
        {
            struct kmulti_rtimer *t;
            rt_list_t *p = row_head[row_lvl]->next;

            /* fix up the entry pointer */
            t = rt_list_entry(p, struct kmulti_rtimer, row[row_lvl]);

            /* If we have two timers that timeout at the same time, it's
             * preferred that the timer inserted early get called early.
             * So insert the new timer to the end the the some-timeout timer
             * list.
             */
            if ((t->timeout_tick - timer->timeout_tick) == 0)
            {
                continue;
            }
            else if ((t->timeout_tick - timer->timeout_tick) < RT_TICK_MAX / 2)
            {
                break;
            }
        }
        if (row_lvl != RT_TIMER_SKIP_LIST_LEVEL - 1)
            row_head[row_lvl + 1] = row_head[row_lvl] + 1;
    }

    /* Interestingly, this super simple timer insert counter works very very
     * well on distributing the list height uniformly. By means of "very very
     * well", I mean it beats the randomness of timer->timeout_tick very easily
     * (actually, the timeout_tick is not random and easy to be attacked). */
    random_nr++;
    tst_nr = random_nr;

    rt_list_insert_after(row_head[RT_TIMER_SKIP_LIST_LEVEL - 1],
                         &(timer->row[RT_TIMER_SKIP_LIST_LEVEL - 1]));
    for (row_lvl = 2; row_lvl <= RT_TIMER_SKIP_LIST_LEVEL; row_lvl++)
    {
        if (!(tst_nr & RT_TIMER_SKIP_LIST_MASK))
            rt_list_insert_after(row_head[RT_TIMER_SKIP_LIST_LEVEL - row_lvl],
                                 &(timer->row[RT_TIMER_SKIP_LIST_LEVEL - row_lvl]));
        else
            break;
        /* Shift over the bits we have tested. Works well with 1 bit and 2
         * bits. */
        tst_nr >>= (RT_TIMER_SKIP_LIST_MASK + 1) >> 1;
    }

    timer->parent.flag |= RT_TIMER_FLAG_ACTIVATED;

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}
RTM_EXPORT(kmulti_rtimer_start);

/**
 * This function will stop the timer
 *
 * @param timer the timer to be stopped
 *
 * @return the operation status, RT_EOK on OK, -RT_ERROR on error
 */
rt_err_t kmulti_rtimer_stop(kmulti_rtimer_t timer)
{
    register rt_base_t level;

    /* timer check */
    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(rt_object_get_type(&timer->parent) == RT_Object_Class_Timer);

    if (!(timer->parent.flag & RT_TIMER_FLAG_ACTIVATED))
        return -RT_ERROR;

    RT_OBJECT_HOOK_CALL(rt_object_put_hook, (&(timer->parent)));

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    _kmulti_rtimer_remove(timer);
    /* change status */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}
RTM_EXPORT(kmulti_rtimer_stop);

/**
 * This function will get or set some options of the timer
 *
 * @param timer the timer to be get or set
 * @param cmd the control command
 * @param arg the argument
 *
 * @return RT_EOK
 */
rt_err_t kmulti_rtimer_control(kmulti_rtimer_t timer, int cmd, void *arg)
{
    register rt_base_t level;

    /* timer check */
    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(rt_object_get_type(&timer->parent) == RT_Object_Class_Timer);

    level = rt_hw_interrupt_disable();
    switch (cmd)
    {
    case RT_TIMER_CTRL_GET_TIME:
        *(rt_tick_t *)arg = timer->init_tick;
        break;

    case RT_TIMER_CTRL_SET_TIME:
        timer->init_tick = *(rt_tick_t *)arg;
        break;

    case RT_TIMER_CTRL_SET_ONESHOT:
        timer->parent.flag &= ~RT_TIMER_FLAG_PERIODIC;
        break;

    case RT_TIMER_CTRL_SET_PERIODIC:
        timer->parent.flag |= RT_TIMER_FLAG_PERIODIC;
        break;

    case RT_TIMER_CTRL_GET_STATE:
        if(timer->parent.flag & RT_TIMER_FLAG_ACTIVATED)
        {
            /*timer is start and run*/
            *(rt_tick_t *)arg = RT_TIMER_FLAG_ACTIVATED;
        }
        else
        {
            /*timer is stop*/
            *(rt_tick_t *)arg = RT_TIMER_FLAG_DEACTIVATED;
        }
        break;

    default:
        break;
    }
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}
RTM_EXPORT(kmulti_rtimer_control);

/**
 * This function will check timer list, if a timeout event happens, the
 * corresponding timeout function will be invoked.
 *
 * @note this function shall be invoked in operating system timer interrupt.
 */
void kmulti_rtimer_check(void)
{
    struct kmulti_rtimer *t;
    rt_tick_t current_tick;
    register rt_base_t level;
    rt_list_t list;

    rt_list_init(&list);

    RT_DEBUG_LOG(RT_DEBUG_TIMER, ("timer check enter\n"));

    current_tick = rtimer_tick_get();

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    while (!rt_list_isempty(&kmulti_rtimer_list[RT_TIMER_SKIP_LIST_LEVEL - 1]))
    {
        t = rt_list_entry(kmulti_rtimer_list[RT_TIMER_SKIP_LIST_LEVEL - 1].next,
                          struct kmulti_rtimer, row[RT_TIMER_SKIP_LIST_LEVEL - 1]);

        /*
         * It supposes that the new tick shall less than the half duration of
         * tick max.
         */
        if ((current_tick - t->timeout_tick) < RT_TICK_MAX / 2)
        {
            RT_OBJECT_HOOK_CALL(kmulti_rtimer_enter_hook, (t));

            /* remove timer from timer list firstly */
            _kmulti_rtimer_remove(t);
            if (!(t->parent.flag & RT_TIMER_FLAG_PERIODIC))
            {
                t->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
            }
            /* add timer to temporary list  */
            rt_list_insert_after(&list, &(t->row[RT_TIMER_SKIP_LIST_LEVEL - 1]));
            /* call timeout function */
            t->timeout_func(t->parameter);

            /* re-get tick */
            current_tick = rtimer_tick_get();

            RT_OBJECT_HOOK_CALL(kmulti_rtimer_exit_hook, (t));
            RT_DEBUG_LOG(RT_DEBUG_TIMER, ("current tick: %d\n", current_tick));

            /* Check whether the timer object is detached or started again */
            if (rt_list_isempty(&list))
            {
                continue;
            }
            rt_list_remove(&(t->row[RT_TIMER_SKIP_LIST_LEVEL - 1]));
            if ((t->parent.flag & RT_TIMER_FLAG_PERIODIC) &&
                (t->parent.flag & RT_TIMER_FLAG_ACTIVATED))
            {
                /* start it */
                t->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
                kmulti_rtimer_start(t);
            }
        }
        else break;
    }

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    RT_DEBUG_LOG(RT_DEBUG_TIMER, ("timer check leave\n"));
}

/**
 * This function will return the next timeout tick in the system.
 *
 * @return the next timeout tick in the system
 */
rt_tick_t kmulti_rtimer_next_timeout_tick(void)
{
    return kmulti_rtimer_list_next_timeout(kmulti_rtimer_list);
}

/**
 * @ingroup kmulti_rtimer_init
 *
 * This function will initialize multi timer
 */
void rt_system_kmulti_rtimer_init(void)
{
    int i;

    for (i = 0; i < sizeof(kmulti_rtimer_list) / sizeof(kmulti_rtimer_list[0]); i++)
    {
        rt_list_init(kmulti_rtimer_list + i);
    }
}

/**@}*/
