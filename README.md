### 软件多路定时器

#### 简介

软件多路定时器，基于`rt-thread`的`hwtimer`驱动实现的软件多路定时器

#### 驱动框架

`kmulti_rtimer`软件包目录结构如下：

```
kmulti_rtimer
├── demo
│   ├── kmulti_rtimer_example
│   └── SConscript
├── docs
├── LICENSE
├── README.md
├── SConscript
└── src
    ├── kmulti_rtimer.c
    ├── kmulti_rtimer.h
    ├── SConscript
    └── timer_port.c
```

#### 许可证

kmulti_rtimer 软件包遵循 Apache-2.0 许可，详见 LICENSE 文件。

#### 获取软件包

使用`kmulti_rtimer`软件包需要在RT-Thread的包管理器中选中它，具体路径如下：
```
RT-Thread online packages
    system packages  --->
        --- a multi timer for rt-thread.                           
        (hwtimer1) The hwtimer device name for multi rtimer
        [ ]   Use demo (NEW)                                      
              Version (latest)  --->                                
```
其中`The hwtimer device name for multi rtimer`选择你要使用的`hwtimer`

#### 使用示例

```
    /* 创建定时器1  周期定时器 */
    timer1 = kmulti_rtimer_create("timer1", timeout1, RT_NULL,
                             RTIMER_TICK_PER_SECOND / 10, RT_TIMER_FLAG_PERIODIC);
    /* 启动定时器1 */
    if (timer1 != RT_NULL)
        kmulti_rtimer_start(timer1);
```

#### 联系方式&感谢

 - [kylepengchn](https://github.com/kylepengchn) 

  - 主页：<https://github.com/kylepengchn/kmulti_rtimer>
