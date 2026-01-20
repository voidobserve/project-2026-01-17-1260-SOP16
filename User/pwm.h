#ifndef _PWM_H
#define _PWM_H

#include "my_config.h"
#include "include.h"
#include <stdio.h>

#define MAX_PWM_DUTY (6000) // 100%占空比   (SYSCLK 4800 0000 /  8000  == 6000)
enum
{
    PWM_DUTY_100_PERCENT = (u16)MAX_PWM_DUTY,
    PWM_DUTY_80_PERCENT = (u16)((u32)MAX_PWM_DUTY * 80 / 100),
    PWM_DUTY_60_PERCENT = (u16)((u32)MAX_PWM_DUTY * 60 / 100),
    PWM_DUTY_50_PERCENT = (u16)((u32)MAX_PWM_DUTY * 50 / 100),
    PWM_DUTY_40_PERCENT = (u16)((u32)MAX_PWM_DUTY * 40 / 100),
    PWM_DUTY_30_PERCENT = (u16)((u32)MAX_PWM_DUTY * 30 / 100),
    PWM_DUTY_25_PERCENT = (u16)((u32)MAX_PWM_DUTY * 25 / 100),
    PWM_DUTY_20_PERCENT = (u16)((u32)MAX_PWM_DUTY * 20 / 100),
    PWM_DUTY_0_PERCENT = (u16)((u32)MAX_PWM_DUTY * 0 / 100),
};

// 定义 pwm 模式
enum
{
    PWM_MODE_COLOR_TEMPERATURE_1, // 色温1
    PWM_MODE_COLOR_TEMPERATURE_2,
    PWM_MODE_COLOR_TEMPERATURE_3,
    PWM_MODE_COLOR_TEMPERATURE_4,

    PWM_MODE_PULSE, // 脉冲模式

    PWM_MODE_COLOR_BLUE,  // 蓝光
    PWM_MODE_COLOR_CYAN,  // 青光
    PWM_MODE_COLOR_GREEN, // 绿光
};

// 由温度限制的PWM占空比 （对所有PWM通道都生效）
extern volatile u16 limited_pwm_duty_due_to_temp;
// 由于发动机不稳定，而限制的可以调节到的占空比（对所有PWM通道都生效，默认为最大占空比）
extern volatile u16 limited_pwm_duty_due_to_unstable_engine;
// 由于风扇异常，限制的可以调节到的最大占空比（对所有PWM通道都生效，默认为最大占空比）
extern volatile u16 limited_pwm_duty_due_to_fan_err;

extern volatile u16 cur_pwm_channel_0_duty;           // 当前设置的、 pwm_channle_0 的占空比
extern volatile u16 expect_adjust_pwm_channel_0_duty; // 存放期望调节到的 pwm_channle_0 占空比
extern volatile u16 adjust_pwm_channel_0_duty;        // pwm_channle_0 要调整到的占空比

extern volatile u16 cur_pwm_channel_1_duty;           // 当前设置的第二路PWN的占空比
extern volatile u16 expect_adjust_pwm_channel_1_duty; // 存放期望调节到的 pwm_channle_1 占空比
extern volatile u16 adjust_pwm_channel_1_duty;        // pwm_channle_1 要调整到的占空比

extern volatile u8 pwm_mode; 

void pwm_init(void);

// 电源电压低于170V-AC,启动低压保护，电源电压高于170V-AC，关闭低压保护
void according_pin9_to_adjust_pwm(void);
// void according_pin9_to_adjust_pin16(void); // 根据9脚的电压来设定16脚的电平

extern u8 get_pwm_channel_0_status(void); // 获取第一路PWM的运行状态
extern u8 get_pwm_channel_1_status(void); // 获取第二路PWM的运行状态

extern void pwm_channel_0_enable(void);
extern void pwm_channel_0_disable(void);

extern void pwm_channel_1_enable(void);
extern void pwm_channel_1_disable(void);

void set_pwm_channel_0_duty(u16 channel_duty);
void set_pwm_channel_1_duty(u16 channel_duty);

u16 get_pwm_channel_x_adjust_duty(const u16 pwm_adjust_duty);

void pwm_mode_handle(void);

#endif