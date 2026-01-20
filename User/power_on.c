#include "power_on.h"
#include "include.h"
#include "pwm.h"
#include <math.h>

float step = 70;
float mi; // 幂

volatile bit flag_is_in_power_on = 0; // 是否处于开机缓启动
static volatile u32 power_on_step = 0;
volatile bit flag_time_comes_during_power_on = 0; // 标志位，开机缓启动期间，调节时间到来（由定时器置位）
  
void power_on_handle(void)
{ 
    cur_pwm_channel_0_duty = 0;
    cur_pwm_channel_1_duty = 0;
    flag_is_in_power_on = 1; // 表示到了开机缓启动

    while (1)
    {
#if USE_MY_DEBUG // 直接打印0，防止在串口+图像上看到错位
                 // printf(",b=0,"); // 防止在串口图像错位
#endif
        if (cur_pwm_channel_0_duty >= DEST_POWER_ON_DUTY_VAL &&
            cur_pwm_channel_1_duty >= DEST_POWER_ON_DUTY_VAL)
        {
            // 当两路pwm都到对应的占空比值之后，才退出开机缓启动
            break;
        }

        if (flag_time_comes_during_power_on) // 如果调节时间到来
        {
            flag_time_comes_during_power_on = 0;
            power_on_step += POWER_ON_ADJUST_STEP; // 累计步长
            if (power_on_step >= 1000)
            {
                power_on_step -= 1000;
                cur_pwm_channel_0_duty++;
                cur_pwm_channel_1_duty = cur_pwm_channel_0_duty;
            }
        }

        set_pwm_channel_0_duty(cur_pwm_channel_0_duty);
        set_pwm_channel_1_duty(cur_pwm_channel_1_duty);
    }

    // 缓启动后，立即更新 adjust_duty 的值：（ 要给下面这些变量赋值，上电后会根据这些变量的值来调节 ）
    adjust_pwm_channel_0_duty = cur_pwm_channel_0_duty;
    adjust_pwm_channel_1_duty = cur_pwm_channel_1_duty;
    expect_adjust_pwm_channel_0_duty = cur_pwm_channel_0_duty;
    expect_adjust_pwm_channel_1_duty = cur_pwm_channel_1_duty;
    flag_is_in_power_on = 0; // 表示退出了开机缓启动
}