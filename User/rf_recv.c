#include "rf_recv.h"

// rf对码期间，用于临时存放遥控器地址的变量
// 在 rf_key_get_key_id() 函数中取得，如果对码成功，则直接应用该地址，写入flash
volatile u32 tmp_rf_addr = 0xFFFFFFFF;
volatile rf_remote_info_t rf_remote_info = {0};

volatile bit flag_is_recved_rf_data = 0; // 是否接收到了rf信号
volatile u32 rf_data = 0;                // 存放接收到的rf数据

// 控制一路PWM遥控器对应的表格
static const u8 rf_key_event_table[][RF_KEY_EFFECT_EVENT_NUMS + 1] = {
    {RF_433_KEY_VAL_R1C1, RF_433_KEY_EVENT_R1C1_CLICK, RF_KEY_EVENT_R1C1_LONG, RF_KEY_EVENT_R1C1_HOLD},

};

extern u8 rf_key_get_key_id(void);
volatile struct key_driver_para rf_key_para = {
    // 编译器不支持指定成员赋值的写法，会报错

    RF_KEY_SCAN_CIRCLE_TIMES, // .scan_times 扫描频率，单位：ms
    0,                        // .cur_scan_times 按键扫描频率, 单位ms，由1ms的定时器中断内累加，在key_driver_scan()中清零
    // NO_KEY,
    0, // .last_key

    0,                   // .filter_value
    0,                   // .filter_cnt
    RF_KEY_FILTER_TIMES, // .filter_time 按键消抖次数，与扫描频率有关（rf按键不消抖）

    RF_LONG_PRESS_TIME_THRESHOLD_MS / RF_KEY_SCAN_CIRCLE_TIMES,                                     // .long_time
    (RF_LONG_PRESS_TIME_THRESHOLD_MS + RF_HOLD_PRESS_TIME_THRESHOLD_MS) / RF_KEY_SCAN_CIRCLE_TIMES, // .hold_time
    0,                                                                                              // .press_cnt

    0,                              // .click_cnt
    0,                              // .click_delay_cnt
    200 / RF_KEY_SCAN_CIRCLE_TIMES, // .click_delay_time
    // NO_KEY,
    0,                 // .notify_value
    KEY_TYPE_RF,       // .key_type
    rf_key_get_key_id, // .get_value

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
    if (flag_is_recved_rf_data)
    {
        flag_is_recved_rf_data = 0;
        if (rf_data)
        {
            // u8 ret = (u8)rf_data;
            // u8 ret = ((u8)rf_data) & 0x0F; // 获取低4位作为键值
            u8 ret = ((u8)rf_data) & 0xFF; // 获取低8位作为键值

            // printf("rf_data 0x %lx\n", rf_data);

            // 如果不在rf对码期间，并且遥控器的地址不一致
            // if ((rf_data >> 8) != rf_remote_info.rf_addr)
            // if ((rf_data >> 4) != rf_remote_info.rf_addr) // 数据高20位是地址，低4位是键值

            if (rf_remote_info.is_addr_valid == 0xC5)
            {
                // 如果之前存放了有效的遥控器的地址
                // 判断遥控器类型：
                if (1 == rf_remote_info.rf_remote_type &&
                    (rf_data >> 4) == rf_remote_info.rf_addr)
                {
                    // 如果是控制一路PWM的遥控器，接收到的数据低4位是键值
                    ret &= 0x0F;
                }
                else if (2 == rf_remote_info.rf_remote_type &&
                         (rf_data >> 8) == rf_remote_info.rf_addr)
                {
                    // 如果是控制两路PWM的遥控器，接收到的数据低8位是键值
                    // ret &= 0xFF; // 可以不写这一句
                }
                else
                {
                    // 如果遥控器的地址不一样
                    ret = NO_KEY;
                }
            }
            else
            {
                // 如果之前没有存放有效的遥控器的地址
                ret = NO_KEY;
            }

            // printf("key id %bu\n", ret);
            rf_data = 0;    // 接收完成后，清除接收到的数据
            return (u8)ret; // 直接获取键值
        }
        else
        {
            return NO_KEY;
        }
    }
    else
    {
        return NO_KEY;
    }
}

/**
 * @brief __rf_key_get_event的子函数，将按键值和 key_driver_scan 得到的按键事件转换成触摸按键的事件
 *
 * @param key_val 触摸按键键值
 * @param key_event 在key_driver_scan得到的按键事件 KEY_EVENT
 * @param table_index 在哪个表格中进行查表，
 *                      1--在控制一路PWM遥控器对应的表格
 *                      2--控制两路PWM遥控器对应的表格
 * @return u8
 */
static u8 __sub_fun_rf_key_get_event(const u8 key_val, const u8 key_event, const u8 table_index)
{
    u8 ret_key_event = RF_KEY_EVENT_NONE;
    u8 i = 0;
    for (i = 0; i < ARRAY_SIZE(rf_key_event_table); i++)
    {
        // 如果往 KEY_EVENT 枚举中添加了新的按键事件，这里查表的方法就会失效，需要手动修改
        if (key_val == rf_key_1_event_table[i][0])
        {
            ret_key_event = rf_key_1_event_table[i][key_event];
            break;
        }
    }

    return ret_key_event;
}

/**
 * @brief 将按键值和 key_driver_scan 得到的按键事件转换成触摸按键的事件
 *          函数内部会做遥控器类型区分
 *
 * @param key_val 触摸按键键值
 * @param key_event 在key_driver_scan得到的按键事件 KEY_EVENT
 * @return u8 在 rf_key_event_table 中找到的对应的按键事件，如果没有则返回 RF_KEY_EVENT_NONE
 */
static u8 __rf_key_get_event(const u8 key_val, const u8 key_event)
{
    volatile u8 ret_key_event = RF_KEY_EVENT_NONE;
    u8 i = 0;

    {
        // 如果不在对码阶段
        if (tmp_rf_addr == rf_remote_info.rf_addr && rf_remote_info.is_addr_valid == 0xC5)
        {
            // 之前存放了有效的遥控器，并且现在读到的遥控器的地址一样
            if (1 == rf_remote_info.rf_remote_type)
            {
                // 单路PWM遥控器
                ret_key_event = __sub_fun_rf_key_get_event(key_val, key_event, 1);
            }
            else if (2 == rf_remote_info.rf_remote_type)
            {
                // 1+2两路PWM遥控器
                ret_key_event = __sub_fun_rf_key_get_event(key_val, key_event, 2);
            }
        }
        else
        {
            // ret_key_event = RF_KEY_EVENT_NONE;
        }
    }

    return ret_key_event;
}

//  函数内部会做遥控器类型区分
void rf_key_handle(void)
{
    u8 rf_key_event = RF_KEY_EVENT_NONE;

    // 如果是无效的按键信息，函数直接返回
    if (rf_key_para.latest_key_val == NO_KEY)
    {
        return;
    }

    rf_key_event = __rf_key_get_event(rf_key_para.latest_key_val, rf_key_para.latest_key_event);
    rf_key_para.latest_key_val = NO_KEY;
    rf_key_para.latest_key_event = KEY_EVENT_NONE;

    // printf("rf event %bu\n", rf_key_event);

    // 如果按键地址无效，未进行对码，函数直接返回，不进行键值处理
    // 正在进行对码，不进行键值处理
    // if (0xC5 != rf_remote_info.is_addr_valid || flag_is_in_rf_learning)
    if (0xC5 != rf_remote_info.is_addr_valid) /* 如果之前未进行对码，不进行键值处理 */
    {
        return;
    }

    switch (rf_key_event)
    {
    default:
        break;
    }
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
