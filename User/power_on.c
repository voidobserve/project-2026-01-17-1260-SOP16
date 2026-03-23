#include "power_on.h"
#include "include.h"
#include "pwm.h"
#include <math.h>

volatile bit flag_is_in_power_on = 0; // 是否处于开机缓启动
static volatile u32 pwm0_power_on_step = 0;
static volatile u32 pwm1_power_on_step = 0;
volatile bit flag_time_comes_during_power_on = 0; // 标志位，开机缓启动期间，调节时间到来（由定时器置位）

void power_on_handle(void)
{
    cur_pwm_channel_0_duty = 0;
    cur_pwm_channel_1_duty = 0;
    flag_is_in_power_on = 1; // 表示到了开机缓启动

    while (1)
    {
        if (cur_pwm_channel_0_duty >= PWM0_DEST_POWER_ON_DUTY_VAL &&
            cur_pwm_channel_1_duty >= PWM1_DEST_POWER_ON_DUTY_VAL)
        {
            // 当两路pwm都到对应的占空比值之后，才退出开机缓启动
            break;
        }

        if (flag_time_comes_during_power_on) // 如果调节时间到来
        {
            flag_time_comes_during_power_on = 0;
            pwm0_power_on_step += PWM0_POWER_ON_ADJUST_STEP; // 累计步长
            if (pwm0_power_on_step >= 1000)
            {
                pwm0_power_on_step -= 1000;

                if (cur_pwm_channel_0_duty < PWM0_DEST_POWER_ON_DUTY_VAL)
                {
                    cur_pwm_channel_0_duty++;
                }
            }

            pwm1_power_on_step += PWM1_POWER_ON_ADJUST_STEP;
            if (pwm1_power_on_step >= 1000)
            {
                pwm1_power_on_step -= 1000;

                if (cur_pwm_channel_1_duty < PWM1_DEST_POWER_ON_DUTY_VAL)
                {
                    cur_pwm_channel_1_duty++;
                }
            }
        }

        // printf("cur pwm channel 0 duty: %u\n", cur_pwm_channel_0_duty);
        // printf("cur pwm channel 1 duty: %u\n", cur_pwm_channel_1_duty);

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