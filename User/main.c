/**
 ******************************************************************************
 * @file    main.c
 * @author  HUGE-IC Application Team
 * @version V1.0.0
 * @date    02-09-2022
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2021 HUGE-IC</center></h2>
 *
 * 版权说明后续补上
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "my_config.h"
#include "include.h"

#include <stdio.h>
#include "power_on.h"

#if USE_MY_DEBUG // 打印串口配置

#define UART0_BAUD 115200
#define USER_UART0_BAUD ((SYSCLK - UART0_BAUD) / (UART0_BAUD))
// 重写puchar()函数
char putchar(char c)
{
    while (!(UART0_STA & UART_TX_DONE(0x01)))
        ;
    UART0_DATA = c;
    return c;
}

void my_debug_config(void)
{
    // 作为发送引脚
    // P1_MD0 &= (GPIO_P13_MODE_SEL(0x3));
    // P1_MD0 |= GPIO_P13_MODE_SEL(0x1);            // 配置为输出模式
    // FOUT_S13 |= GPIO_FOUT_UART0_TX;              // 配置为UART0_TX
    // UART0_BAUD1 = (USER_UART0_BAUD >> 8) & 0xFF; // 配置波特率高八位
    // UART0_BAUD0 = USER_UART0_BAUD & 0xFF;        // 配置波特率低八位
    // UART0_CON0 = UART_STOP_BIT(0x0) |
    //              UART_EN(0x1); // 8bit数据，1bit停止位

    P0_MD0 &= ~(GPIO_P00_MODE_SEL(0x3));
    P0_MD0 |= GPIO_P00_MODE_SEL(0x1);            // 配置为输出模式
    FOUT_S00 |= GPIO_FOUT_UART0_TX;              // 配置为UART0_TX
    UART0_BAUD1 = (USER_UART0_BAUD >> 8) & 0xFF; // 配置波特率高八位
    UART0_BAUD0 = USER_UART0_BAUD & 0xFF;        // 配置波特率低八位
    UART0_CON0 = UART_STOP_BIT(0x0) |
                 UART_EN(0x1); // 8bit数据，1bit停止位
}
#endif // USE_MY_DEBUG // 打印串口配置

void main(void)
{
    // 看门狗默认打开, 复位时间2s
    WDT_KEY = WDT_KEY_VAL(0xDD); //  关闭看门狗 (如需配置看门狗请查看“WDT\WDT_Reset”示例)

    system_init();

    // 关闭HCK和HDA的调试功能
    WDT_KEY = 0x55;  // 解除写保护
    IO_MAP &= ~0x01; // 清除这个寄存器的值，实现关闭HCK和HDA引脚的调试功能（解除映射）
    WDT_KEY = 0xBB;  // 写一个无效的数据，触发写保护

#if USE_MY_DEBUG // 打印串口配置
    // 初始化打印

    my_debug_config();
    printf("sys reset\n");

    // // P06 配置为输出模式
    // P0_MD1 &= ~(0x03 << 4);
    // P0_MD1 |= 0x01 << 4;
    // FOUT_S06 = GPIO_FOUT_AF_FUNC;
    // P06 = 0;

    // // P21 配置为输出模式
    // P2_MD0 &= ~(0x03 << 2);
    // P2_MD0 |= 0x01 << 2; // 输出模式
    // FOUT_S21 = GPIO_FOUT_AF_FUNC;
    // P21 = 0;

    // 输出模式：
    // P1_MD0 &= (GPIO_P13_MODE_SEL(0x3));
    // P1_MD0 |= GPIO_P13_MODE_SEL(0x1); // 配置为输出模式
    // FOUT_S13 = GPIO_FOUT_AF_FUNC;     // 选择AF功能输出

    P2_MD1 &= ~(GPIO_P26_MODE_SEL(0x3));
    P2_MD1 |= GPIO_P26_MODE_SEL(0x01); // 输出模式
    FOUT_S26 = GPIO_FOUT_AF_FUNC;
#endif // 打印串口配置

#if 1
    adc_pin_config(); // 配置使用到adc的引脚
    adc_config();

    tmr0_config(); //
    pwm_init();    // 配置pwm输出的引脚
    tmr1_config();

    timer2_config();
    timer3_config(); // 要等adc完成初始化，再调用timer3的初始化

    rf_recv_init(); // rf功能初始化
    fan_ctl_config();

    P1_MD1 &= ~GPIO_P14_MODE_SEL(0x03); // P14 16脚
    P1_MD1 |= GPIO_P14_MODE_SEL(0x01);
    FOUT_S14 = GPIO_FOUT_AF_FUNC; // AF功能输出
    P14 = 0;                      //
#endif

    // limited_max_pwm_duty = MAX_PWM_DUTY;
    limited_pwm_duty_due_to_fan_err = MAX_PWM_DUTY;
    limited_pwm_duty_due_to_temp = MAX_PWM_DUTY;
    limited_pwm_duty_due_to_unstable_engine = MAX_PWM_DUTY;

    pwm_mode = PWM_MODE_COLOR_CYAN; // 开机缓启动之后，默认的模式
    power_on_handle();

    while (1)
    {
#if 1
        temperature_scan(); // 检测热敏电阻一端的电压值
        fan_scan();         // 检测风扇的状态是否异常，并根据结果来限制pwm占空比
        set_duty();         // 设定到要调节到的脉宽 (设置adjust_duty)

        key_driver_scan(&rf_key_para);
        rf_key_handle();

        // 如果 expect_adjust_pwm_channel_x_duty 有变化，可以在这里修改 adjust_pwm_channel_x_duty
        adjust_pwm_channel_0_duty = get_pwm_channel_x_adjust_duty(expect_adjust_pwm_channel_0_duty);
        adjust_pwm_channel_1_duty = get_pwm_channel_x_adjust_duty(expect_adjust_pwm_channel_1_duty);

        // 风扇控制：
        // fan_ctl(); // 没有了线控调光，这里目前用不上，没有关灯的功能
#endif
    }
}

/**
 * @}
 */

/*************************** (C) COPYRIGHT 2022 HUGE-IC ***** END OF FILE *****/
