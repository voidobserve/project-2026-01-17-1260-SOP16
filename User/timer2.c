#include "timer2.h"
#include "power_on.h"
// #define TIMER2_PEROID_VAL (SYSCLK / 128 / 10000 - 1) // 周期值=系统时钟/分频/频率 - 1
#define TIMER2_PEROID_VAL ((u32)24 * 100 * 10000 / 1 / 10000 - 1) // 周期值 = 时钟 /分频/频率 - 1
// #define TIMER2_PEROID_VAL ((u32)2400) //
// #define TIMER2_PEROID_VAL ((u32)2400 - 1) //
// #define TIMER2_PEROID_VAL ((u32)2400 + 1) //

static volatile u16 pwm_duty_add_cnt; // 用于控制pwm增加的时间计数
static volatile u16 pwm_duty_sub_cnt; // 用于控制pwm递减的时间计数

volatile bit flag_is_pwm_add_time_comes = 0; // 标志位，pwm占空比递增时间到来
volatile bit flag_is_pwm_sub_time_comes = 0; // 标志位，pwm占空比递减时间到来

static volatile u16 pwm_duty_change_cnt = 0; // 用于控制pwm变化的时间计数（用在旋钮调节的PWM占空比中）

volatile u16 pwm_mode_pulse_cnt = 0;
volatile bit pwm_mode_pulse_dir = 0;

void timer2_config(void)
{
    __EnableIRQ(TMR2_IRQn); // 使能timer2中断
    IE_EA = 1;              // 使能总中断

    // 设置timer2的计数功能，配置一个频率为 10 kHz的中断
    TMR_ALLCON = TMR2_CNT_CLR(0x1);                               // 清除计数值
    TMR2_PRH = TMR_PERIOD_VAL_H((TIMER2_PEROID_VAL >> 8) & 0xFF); // 周期值
    TMR2_PRL = TMR_PERIOD_VAL_L((TIMER2_PEROID_VAL >> 0) & 0xFF);
    TMR2_CONH = TMR_PRD_PND(0x1) | TMR_PRD_IRQ_EN(0x1);                          // 计数等于周期时允许发生中断
    TMR2_CONL = TMR_SOURCE_SEL(0x7) | TMR_PRESCALE_SEL(0x7) | TMR_MODE_SEL(0x1); // 选择系统时钟，128分频，计数模式

    // // 改用外部晶振给定时器作时钟源
    // CLK_XOSC = CLK_XOSC_HIGH_EN(0x1) | CLK_XOSC_HIGH_DR(0x07); // 使能外部高速晶振
    // // CLK_XOSC = CLK_XOSC_LOW_EN(0x1); // 使能外部低速晶振
    // TMR2_CONL = TMR_SOURCE_SEL(0x4) | TMR_PRESCALE_SEL(0x00) | TMR_MODE_SEL(0x1); // 选择外部晶振时钟， 分频 ，计数模式
}

// 定时器 中断服务函数
void TIMR2_IRQHandler(void) interrupt TMR2_IRQn
{
    // 进入中断设置IP，不可删除
    __IRQnIPnPush(TMR2_IRQn);

    // ---------------- 用户函数处理 -------------------
    // 周期中断
    if (TMR2_CONH & TMR_PRD_PND(0x1)) // 约100us触发一次中断
    {
        TMR2_CONH |= TMR_PRD_PND(0x1); // 清除pending

        // tmr2_cnt++;
        // P13 = ~P13; // 测试中断触发周期

        pwm_duty_add_cnt++;
        pwm_duty_sub_cnt++;
        pwm_duty_change_cnt++;

        if (pwm_duty_sub_cnt >= 13) // 1300us，1.3ms
        // if (pwm_duty_sub_cnt >= 50)
        {
            pwm_duty_sub_cnt = 0;
            flag_is_pwm_sub_time_comes = 1;
        }

        // if (pwm_duty_add_cnt >= 133) // 13300us, 13.3ms
        if (pwm_duty_add_cnt >= 13) //
        {
            pwm_duty_add_cnt = 0;
            flag_is_pwm_add_time_comes = 1;
        }

#if 1 // rf信号接收 （100us调用一次）
        {
            static volatile u8 rf_bit_cnt;             // RF信号接收的数据位计数值
            static volatile u32 __rf_data;             // 定时器中断使用的接收缓冲区，避免直接覆盖全局的数据接收缓冲区
            static volatile bit __flag_is_recved_data; // 表示中断服务函数接收到了rf数据

            static volatile u8 low_level_cnt;  // RF信号低电平计数值
            static volatile u8 high_level_cnt; // RF信号高电平计数值

            static volatile bit last_lev = 0;

            // 在定时器 中扫描端口电平
            if (0 == RFIN_PIN)
            {
                // 如果RF接收引脚为低电平，记录低电平的持续时间
                if (low_level_cnt < 255) // 防止计数溢出
                {
                    low_level_cnt++;
                }

                if (last_lev != 0)
                {
                    // 如果之前是高电平，现在是低电平，说明刚开始记录低电平时长
                    low_level_cnt = 0;
                    last_lev = 0;
                }

                /*
                    下面的判断条件是避免部分遥控器或接收模块只发送24位数据，最后不拉高电平的情况
                */
                if (rf_bit_cnt == 23)
                {
                    // 如果低电平大于3000us，并且已经接收了23位数据
                    if (low_level_cnt >= 30)
                    {
                        if (high_level_cnt >= 6 && high_level_cnt < 20)
                        {
                            __rf_data |= 0x01;
                        }
                        else if (high_level_cnt >= 1 && high_level_cnt < 6)
                        {
                        }

                        high_level_cnt = 0;
                        __flag_is_recved_data = 1; // 接收完成标志位置一
                    }
                }
                else
                {
                    high_level_cnt = 0;
                }
            }
            else
            {
                // 如果RF接收引脚为高电平，记录高电平的持续时间
                if (high_level_cnt < 255) // 防止计数溢出
                {
                    high_level_cnt++;
                }

                if (last_lev != 1)
                {
                    // 如果之前不是高电平，才解析一次数据帧
                    last_lev = 1;
                    // 之前是低电平，现在检测到高电平，重新开始高电平计数
                    high_level_cnt = 0;

                    if (low_level_cnt >= 1 && low_level_cnt <= 7)
                    {
                        // 逻辑1
                        __rf_data |= 0x01;
                        rf_bit_cnt++;
                        if (rf_bit_cnt != 24)
                        {
                            __rf_data <<= 1; // 用于存放接收24位数据的变量左移一位
                        }
                    }
                    else if (low_level_cnt >= 7 && low_level_cnt < 20)
                    {

                        // 逻辑0
                        __rf_data &= ~1;
                        rf_bit_cnt++;
                        if (rf_bit_cnt != 24)
                        {
                            __rf_data <<= 1; // 用于存放接收24位数据的变量左移一位
                        }
                    }
                    else
                    {
                        __rf_data = 0;
                        rf_bit_cnt = 0;
                    }

                    if (24 == rf_bit_cnt)
                    {
                        // 如果接收成了24位的数据
                        __flag_is_recved_data = 1; // 接收完成标志位置一
                    }

                    // 解析完一帧后，清空低电平计数
                    low_level_cnt = 0;
                }
            }

            if (__flag_is_recved_data) //
            {
                rf_bit_cnt = 0;
                __flag_is_recved_data = 0;
                low_level_cnt = 0;
                high_level_cnt = 0;

                // 只要收到新的数据，就覆盖 rf_data
                rf_data = __rf_data;
                flag_is_recved_rf_data = 1;
            }
        }
#endif // rf信号接收 （100us调用一次）

#if 1 // 调节PWM占空比

        // if (pwm_duty_change_cnt >= 10) // 1000us,1ms
        // if (pwm_duty_change_cnt >= 1) // 100us（用遥控器调节，在50%以上调节pwm占空比的时候，灯光会有抖动）
        // if (pwm_duty_change_cnt >= 5) // 500us
        // if (pwm_duty_change_cnt >= 10) // x * 100us （用遥控器调节到50%以下pwm占空比的时候，灯光会有抖动）
        if (pwm_duty_change_cnt >= 20) // x * 100us （ 用遥控器调节时，灯光不会有抖动，样机最高功率为870W--加上风扇）
        // if (pwm_duty_change_cnt >= 30) // x * 100us （用遥控器调节时，灯光不会有抖动，但是调节时间过长，感觉不跟手）
        {
            pwm_duty_change_cnt = 0;

            if (0 == flag_is_in_power_on) // 不处于开机缓启动，才使能PWM占空比调节
            {
                // =================================================================
                // pwm_channel_0                                               //
                // =================================================================
                if (adjust_pwm_channel_0_duty > cur_pwm_channel_0_duty)
                {
                    cur_pwm_channel_0_duty++;
                }
                else if (adjust_pwm_channel_0_duty < cur_pwm_channel_0_duty)
                {
                    cur_pwm_channel_0_duty--;
                }

                // =================================================================
                // pwm_channel_1                                               //
                // =================================================================
                if (adjust_pwm_channel_1_duty > cur_pwm_channel_1_duty)
                {
                    cur_pwm_channel_1_duty++;
                }
                else if (adjust_pwm_channel_1_duty < cur_pwm_channel_1_duty)
                {
                    cur_pwm_channel_1_duty--;
                }

                set_pwm_channel_0_duty(cur_pwm_channel_0_duty);
                set_pwm_channel_1_duty(cur_pwm_channel_1_duty);

#if 0
                if (cur_pwm_channel_0_duty <= 0)
                {
                    // 小于某个值，直接输出0%占空比，关闭PWM输出，引脚配置为输出模式
                    pwm_channel_0_disable();
                }
                else // 如果大于0
                {
                    pwm_channel_0_enable();
                }

                if (cur_pwm_channel_1_duty <= 0)
                {
                    // 小于某个值，直接输出0%占空比，关闭PWM输出，引脚配置为输出模式
                    pwm_channel_1_disable();
                }
                else // 如果大于0
                {
                    pwm_channel_1_enable();
                }
#endif

            } // if (0 == flag_is_in_power_on) // 不处于开机缓启动，才使能PWM占空比调节

#if 0
            // printf("c_duty %u\n", c_duty);
            // printf(",c=%u\n", c_duty);
#endif
        }
#endif // 调节PWM占空比

// USER_TO_DO 在测试时屏蔽
#if 1
        {
            // static volatile u16 cnt = 0; // 控制闪烁灯的时间间隔
            // static volatile bit dir = 0; // 控制闪烁灯的闪烁方向

            if (pwm_mode >= PWM_MODE_PULSE_1 &&
                pwm_mode <= PWM_MODE_PULSE_5)
            {
                volatile u16 dest_cnt = 0;
                if (pwm_mode == PWM_MODE_PULSE_1)
                {
                    // PWM_MODE_PULSE_1 1Hz,500ms亮，500ms灭
                    dest_cnt = 5000;

                    // // PWM_MODE_PULSE_1 0.5Hz,1000ms亮，1000ms灭
                    // dest_cnt = 10000;

                    // // PWM_MODE_PULSE_1 0.33Hz,1500ms亮，1500ms灭
                    // dest_cnt = 15000;
                }
                else if (pwm_mode == PWM_MODE_PULSE_2)
                {
                    // PWM_MODE_PULSE_2 3Hz,166.6ms亮（单片机只能取整），166.6ms灭
                    dest_cnt = 1666;
                }
                else if (pwm_mode == PWM_MODE_PULSE_3)
                {
                    // PWM_MODE_PULSE_3 6Hz,83.3ms亮，83.3ms灭
                    dest_cnt = 833;
                }
                else if (pwm_mode == PWM_MODE_PULSE_4)
                {
                    // PWM_MODE_PULSE_4 10Hz,50ms亮，50ms灭
                    dest_cnt = 500;
                }
                else if (pwm_mode == PWM_MODE_PULSE_5)
                {
                    // PWM_MODE_PULSE_5 15Hz,33.3ms亮（单片机只能取整），33.3ms灭
                    dest_cnt = 333;
                }

                pwm_mode_pulse_cnt++;
                if (pwm_mode_pulse_cnt >= dest_cnt)
                {
                    pwm_mode_pulse_cnt = 0;
                    pwm_mode_pulse_dir = !pwm_mode_pulse_dir;
                }

                if (pwm_mode_pulse_dir == 0)
                {
                    if (!get_pwm_channel_0_status())
                    {
                        pwm_channel_0_enable();
                    }

                    if (!get_pwm_channel_1_status())
                    {
                        pwm_channel_1_enable();
                    }
                }
                else
                {
                    if (get_pwm_channel_0_status())
                    {
                        pwm_channel_0_disable();
                    }

                    if (get_pwm_channel_1_status())
                    {
                        pwm_channel_1_disable();
                    }
                }
            }
            else
            {
                // 如果不在脉冲模式
                pwm_mode_pulse_cnt = 0;
                pwm_mode_pulse_dir = 0;

                /*
                    目前只有脉冲模式（pwm_mode == PWM_MODE_PULSE）下
                    会把pwm关闭，如果退出了脉冲模式，要重新打开pwm
                */

                if (!get_pwm_channel_0_status())
                {
                    pwm_channel_0_enable();
                }

                if (!get_pwm_channel_1_status())
                {
                    pwm_channel_1_enable();
                }
            }
        }
#endif
    }

    // 退出中断设置IP，不可删除
    __IRQnIPnPop(TMR2_IRQn);
}