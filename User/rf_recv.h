#ifndef __RF_RECV_H__
#define __RF_RECV_H__

#include "my_config.h"
#include "key_driver.h"

#define RF_ENABLE_PIN P11 // 检测是否有433功能的引脚

#ifdef USE_MY_TEST_PIN // 旧版的编译器会报警告，要加上这一对#ifdef语句
#if USE_MY_TEST_PIN
#define RFIN_PIN P01 // rf信号接收引脚 （测试时使用）
#else
#define RFIN_PIN P03 // rf信号接收引脚， P03 芯片第6脚
#endif               // #if USE_MY_TEST_PIN
#endif               // #ifdef USE_MY_TEST_PIN // 旧版的编译器会报警告，要加上这一对#ifdef语句

/*
    按键的扫描周期，单位：ms
    用在定时器中断，注意不能超过变量的最大值
    这里已经调试好了，不推荐再修改
*/
#define RF_KEY_SCAN_CIRCLE_TIMES (80) // 收到的一帧数据可能有40ms，加上每帧间隔10ms，这里的扫描间隔要更大
#define RF_KEY_FILTER_TIMES (0)               // 按键消抖次数
#define RF_LONG_PRESS_TIME_THRESHOLD_MS (750) // 长按时间阈值（单位：ms）
#define RF_HOLD_PRESS_TIME_THRESHOLD_MS (150) // 长按持续(不松手)的时间阈值(单位：ms)，每隔 xx 时间认为有一次长按持续事件
// #define RF_ADJUST_TOTAL_TIMES_FOR_HOLD (6000) // 长按调节亮度的总时间（从0%占空比 调节到 100%占空比的时间），单位：ms
// #define RF_LEARN_TIMES ((u16)60000)           // 每次上电后，可以进行对码的时间，单位：ms

// 固定地址的遥控器对应的地址码
#define RF_KEY_ADDR ((u16)0x5555)
#define RF_KEY_ADDR_BYTE_1 ((u8)((RF_KEY_ADDR >> 8) & 0xFF)) // 地址码的前8位，第一个字节
#define RF_KEY_ADDR_BYTE_2 ((u8)(RF_KEY_ADDR & 0xFF))        // 地址码的后8位，第二个字节

#define RF_KEY_EFFECT_EVENT_NUMS (2) // 单个触摸按键的有效按键事件个数 (单击、长按)

// 定义433遥控器按键的键值
enum
{
    // RF_433_KEY_VAL_R1C1 = 0x03, // 第一行第一列
    // RF_433_KEY_VAL_R1C2 = 0x0F,
    // RF_433_KEY_VAL_R2C1 = 0x0C, // 第二行第一列
    // RF_433_KEY_VAL_R2C2 = 0x3F,
    // RF_433_KEY_VAL_R3C1 = 0x30,
    // RF_433_KEY_VAL_R3C2 = 0x3C,
    // RF_433_KEY_VAL_R4C1 = 0xC0,
    // RF_433_KEY_VAL_R4C2 = 0xF0,

    RF_433_KEY_VAL_R1C1 = 0x03, // 第一行第一列
    RF_433_KEY_VAL_R1C2 = 0x0C,

    RF_433_KEY_VAL_R2C1 = 0x0F, // 第二行第一列
    RF_433_KEY_VAL_R2C2 = 0x30,

    RF_433_KEY_VAL_R3C1 = 0x33,
    RF_433_KEY_VAL_R3C2 = 0x3C,

    RF_433_KEY_VAL_R4C1 = 0x3F,
    RF_433_KEY_VAL_R4C2 = 0xC0,

    RF_433_KEY_VAL_R5C1 = 0xC3,
    RF_433_KEY_VAL_R5C2 = 0xCC,

    RF_433_KEY_VAL_R6C1 = 0xCF,
    RF_433_KEY_VAL_R6C2 = 0xF0,
};

// 定义433遥控器按键的事件
enum
{
    RF_433_KEY_EVENT_NONE,

    RF_433_KEY_EVENT_R1C1_CLICK,
    RF_433_KEY_EVENT_R1C1_LONG,

    RF_433_KEY_EVENT_R1C2_CLICK,
    RF_433_KEY_EVENT_R1C2_LONG,

    RF_433_KEY_EVENT_R2C1_CLICK,
    RF_433_KEY_EVENT_R2C1_LONG,

    RF_433_KEY_EVENT_R2C2_CLICK,
    RF_433_KEY_EVENT_R2C2_LONG,

    RF_433_KEY_EVENT_R3C1_CLICK,
    RF_433_KEY_EVENT_R3C1_LONG,

    RF_433_KEY_EVENT_R3C2_CLICK,
    RF_433_KEY_EVENT_R3C2_LONG,

    RF_433_KEY_EVENT_R4C1_CLICK,
    RF_433_KEY_EVENT_R4C1_LONG,

    RF_433_KEY_EVENT_R4C2_CLICK,
    RF_433_KEY_EVENT_R4C2_LONG,

    RF_433_KEY_EVENT_R5C1_CLICK,
    RF_433_KEY_EVENT_R5C1_LONG,

    RF_433_KEY_EVENT_R5C2_CLICK,
    RF_433_KEY_EVENT_R5C2_LONG,

    RF_433_KEY_EVENT_R6C1_CLICK,
    RF_433_KEY_EVENT_R6C1_LONG,

    RF_433_KEY_EVENT_R6C2_CLICK,
    RF_433_KEY_EVENT_R6C2_LONG,
};

extern volatile bit flag_is_recved_rf_data; // 是否接收到了rf信号
extern volatile u32 rf_data;                // 存放接收到的rf数据
extern volatile struct key_driver_para rf_key_para;

extern void rf_key_handle(void);
extern void rf_recv_init(void);

#endif