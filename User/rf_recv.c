#include "rf_recv.h"

volatile bit flag_is_recved_rf_data = 0; // 是否接收到了rf信号
volatile u32 rf_data = 0;                // 存放接收到的rf数据

// 按键键值与按键事件关系表
static const u8 rf_key_event_table[][RF_KEY_EFFECT_EVENT_NUMS + 1] = {
    {RF_433_KEY_VAL_R1C1, RF_433_KEY_EVENT_R1C1_CLICK, RF_433_KEY_EVENT_R1C1_LONG},
    {RF_433_KEY_VAL_R1C2, RF_433_KEY_EVENT_R1C2_CLICK, RF_433_KEY_EVENT_R1C2_LONG},
    {RF_433_KEY_VAL_R2C1, RF_433_KEY_EVENT_R2C1_CLICK, RF_433_KEY_EVENT_R2C1_LONG},
    {RF_433_KEY_VAL_R2C2, RF_433_KEY_EVENT_R2C2_CLICK, RF_433_KEY_EVENT_R2C2_LONG},

    {RF_433_KEY_VAL_R3C1, RF_433_KEY_EVENT_R3C1_CLICK, RF_433_KEY_EVENT_R3C1_LONG},
    {RF_433_KEY_VAL_R3C2, RF_433_KEY_EVENT_R3C2_CLICK, RF_433_KEY_EVENT_R3C2_LONG},
    {RF_433_KEY_VAL_R4C1, RF_433_KEY_EVENT_R4C1_CLICK, RF_433_KEY_EVENT_R4C1_LONG},
    {RF_433_KEY_VAL_R4C2, RF_433_KEY_EVENT_R4C2_CLICK, RF_433_KEY_EVENT_R4C2_LONG},

};

extern u8 rf_key_get_key_id(void);
volatile struct key_driver_para rf_key_para = {
    // 编译器不支持指定成员赋值的写法，会报错

    RF_KEY_SCAN_CIRCLE_TIMES, // .scan_times 扫描频率，单位：ms
    0,                        // .cur_scan_times 按键扫描频率, 单位ms，由1ms的定时器中断内累加，在key_driver_scan()中清零
    NO_KEY,                   // .last_key

    0,                   // .filter_value
    0,                   // .filter_cnt
    RF_KEY_FILTER_TIMES, // .filter_time 按键消抖次数，与扫描频率有关（rf按键不消抖）

    RF_LONG_PRESS_TIME_THRESHOLD_MS / RF_KEY_SCAN_CIRCLE_TIMES,                                     // .long_time
    (RF_LONG_PRESS_TIME_THRESHOLD_MS + RF_HOLD_PRESS_TIME_THRESHOLD_MS) / RF_KEY_SCAN_CIRCLE_TIMES, // .hold_time
    0,                                                                                              // .press_cnt

    0,                              // .click_cnt
    0,                              // .click_delay_cnt
    200 / RF_KEY_SCAN_CIRCLE_TIMES, // .click_delay_time
    NO_KEY,                         // .notify_value
    KEY_TYPE_RF,                    // .key_type
    rf_key_get_key_id,              // .get_value

    NO_KEY,         // .latest_key_val
    KEY_EVENT_NONE, // .latest_key_event
}; // volatile struct key_driver_para rf_key_para

/**
 * @brief 获取 rf 遥控器按键键值，供 key_driver 调用
 *          这里不能区分遥控器类型，只能获取遥控器按键
 *
 * @return u8
 */
static u8 rf_key_get_key_id(void)
{
    u8 ret = NO_KEY;

    if (0 == flag_is_recved_rf_data)
    {
        return NO_KEY;
    }

    flag_is_recved_rf_data = 0;

    if ((u8)(rf_data >> 16) != RF_KEY_ADDR_BYTE_1 ||
        (u8)(rf_data >> 8) != RF_KEY_ADDR_BYTE_2)
    {
        return NO_KEY;
    }

    ret = (u8)(rf_data & 0xFF); // 获取低8位作为键值
    // printf("rf_data 0x %lx\n", rf_data);

    // rf_data = 0;    // 接收完成后，清除接收到的数据
    return (u8)ret; // 直接获取键值
}

/**
 * @brief 将按键值和 key_driver_scan 得到的按键事件转换成触摸按键的事件
 *
 * @param key_val 按键键值
 * @param key_event 在 key_driver_scan 得到的按键事件 KEY_EVENT
 * @return u8 在 rf_key_event_table 中找到的对应的按键事件，如果没有则返回 RF_433_KEY_EVENT_NONE
 */
static u8 rf_key_get_event(const u8 key_val, const u8 key_event)
{
    volatile u8 ret_key_event = RF_433_KEY_EVENT_NONE;
    u8 key_event_index = 0;
    u8 i = 0;

    // if (key_event != KEY_EVENT_NONE)
    // {
    //     printf("key_val %bu, key_event %bu\n", key_val, key_event);
    // }

    if (key_event == KEY_EVENT_CLICK)
    {
        key_event_index = 1;
    }
    else if (key_event == KEY_EVENT_LONG)
    {
        key_event_index = 2;
    }
    else
    {
        return RF_433_KEY_EVENT_NONE;
    }

    for (i = 0; i < ARRAY_SIZE(rf_key_event_table); i++)
    {
        // 如果往 KEY_EVENT 枚举中添加了新的按键事件，这里查表的方法就会失效，需要手动修改
        if (key_val == rf_key_event_table[i][0])
        {
            ret_key_event = rf_key_event_table[i][key_event_index];
            break;
        }
    }

    return ret_key_event;
}

//  函数内部会做遥控器类型区分
void rf_key_handle(void)
{
    u8 rf_key_event = RF_433_KEY_EVENT_NONE;

    // 如果是无效的按键信息，函数直接返回
    if (rf_key_para.latest_key_val == NO_KEY)
    {
        return;
    }
#if 1

    rf_key_event = rf_key_get_event(rf_key_para.latest_key_val, rf_key_para.latest_key_event);

    // rf_key_para.latest_key_val = NO_KEY;
    rf_key_para.latest_key_event = KEY_EVENT_NONE;

    // printf("rf event %bu\n", rf_key_event);

    switch (rf_key_event)
    {
    case RF_433_KEY_EVENT_R1C1_CLICK:
    case RF_433_KEY_EVENT_R1C1_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_TEMPERATURE_1; 
    }
    break;
    // =====================================================
    case RF_433_KEY_EVENT_R1C2_CLICK:
    case RF_433_KEY_EVENT_R1C2_LONG:
    {
        pwm_mode = PWM_MODE_PULSE; 
    }
    break;
    // =====================================================
    case RF_433_KEY_EVENT_R2C1_CLICK:
    case RF_433_KEY_EVENT_R2C1_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_TEMPERATURE_2; 
    }
    break;
    // =====================================================
    case RF_433_KEY_EVENT_R2C2_CLICK:
    case RF_433_KEY_EVENT_R2C2_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_BLUE; 
    }
    break;
        // =====================================================
    case RF_433_KEY_EVENT_R3C1_CLICK:
    case RF_433_KEY_EVENT_R3C1_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_TEMPERATURE_3; 
    }
    break;
    // =====================================================
    case RF_433_KEY_EVENT_R3C2_CLICK:
    case RF_433_KEY_EVENT_R3C2_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_CYAN; 
    }
    break;
    // =====================================================
    case RF_433_KEY_EVENT_R4C1_CLICK:
    case RF_433_KEY_EVENT_R4C1_LONG:
    {
        pwm_mode = PWM_MODE_COLOR_TEMPERATURE_4; 
    }
    break; 
        // =====================================================
    case RF_433_KEY_EVENT_R4C2_CLICK: 
    case RF_433_KEY_EVENT_R4C2_LONG:
    { 
        pwm_mode = PWM_MODE_COLOR_GREEN; 
    }
    break;
        // =====================================================

    default:
    {
        return;
    }
        break;
    }

    // 根据对应的模式，立即设置 pwm 占空比
    pwm_mode_handle();
    adjust_pwm_channel_0_duty = get_pwm_channel_x_adjust_duty(expect_adjust_pwm_channel_0_duty);
    adjust_pwm_channel_1_duty = get_pwm_channel_x_adjust_duty(expect_adjust_pwm_channel_1_duty);
    cur_pwm_channel_0_duty = adjust_pwm_channel_0_duty;
    cur_pwm_channel_1_duty = adjust_pwm_channel_1_duty;
    set_pwm_channel_0_duty(cur_pwm_channel_0_duty);
    set_pwm_channel_1_duty(cur_pwm_channel_1_duty);

#endif
}

void rf_recv_init(void)
{
// MY_DEBUG:
#if USE_MY_TEST_PIN // 测试时使用，在开发板上使用 P01 脚，用于检测rf信号

    P0_PU |= GPIO_P01_PULL_UP(0x01);      // 上拉
    P0_MD0 &= ~(GPIO_P01_MODE_SEL(0x03)); // 输入模式

#else // 实际用到的、非测试时使用的rf信号检测引脚：

    P0_PU |= GPIO_P03_PULL_UP(0x01);      // 上拉
    P0_MD0 &= ~(GPIO_P03_MODE_SEL(0x03)); // 输入模式

#endif // #if USE_MY_TEST_PIN
}
