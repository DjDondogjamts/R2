/* 
 * ==========================================================================
 *  ABU Robocon 2026 - R2: Simple Cylinder Cycle Test (No Buttons)
 *  Target: STM32 Nucleo-F446RE
 *  
 *  BEHAVIOR:
 *  1. Extends Cyl 1 for 2s, Retracts for 2s.
 *  2. Extends Cyl 2 for 2s, Retracts for 2s.
 *  3. Repeats forever.
 *  
 *  PINS:
 *  - Cyl 1: INA=PC_14, INB=PC_15, PWM=PB_1
 *  - Cyl 2: INA=PA_13, INB=PA_14, PWM=PB_2
 * ==========================================================================
 */

#include "mbed.h"
#include "stm32f4xx_hal.h"

// --- HARDWARE PINS: CYLINDER 1 ---
DigitalOut cyl1_ina(PA_9); 
DigitalOut cyl1_inb(PC_7);
PwmOut     cyl1_pwm(PA_9);

// --- HARDWARE PINS: CYLINDER 2 ---
DigitalOut cyl2_ina(PA_13);
DigitalOut cyl2_inb(PA_14);
PwmOut     cyl2_pwm(PB_2);

// LCD (Optional, for status)
#define I2C_SDA PB_9
#define I2C_SCL PB_8
I2C i2c_lcd(I2C_SDA, I2C_SCL);
uint8_t lcd_addr = 0x27; 
#define LCD_BACKLIGHT 0x08
#define LCD_EN 0x04
#define LCD_RS 0x01
DigitalOut led(LED1);

// --- LCD HELPERS (Minimal) ---
void wait_us(int us) { thread_sleep_for(us / 1000); }
void write_i2c(uint8_t data) { i2c_lcd.write(lcd_addr << 1, (char*)&data, 1); }
void pulse_enable(uint8_t data) {
    write_i2c(data | LCD_EN); wait_us(100);
    write_i2c(data & ~LCD_EN); wait_us(100);
}
void send_nibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = (nibble & 0xF0) | mode | LCD_BACKLIGHT;
    pulse_enable(data);
}
void send_command(uint8_t cmd) {
    send_nibble(cmd & 0xF0, 0);
    send_nibble((cmd << 4) & 0xF0, 0);
    thread_sleep_for(2);
}
void send_char(char c) {
    send_nibble(c & 0xF0, LCD_RS);
    send_nibble((c << 4) & 0xF0, LCD_RS);
}
void lcd_print(const char* str) { while (*str) send_char(*str++); }
void lcd_init() {
    thread_sleep_for(50);
    write_i2c(0x03 | LCD_BACKLIGHT); wait_us(4500);
    pulse_enable(0x03 | LCD_BACKLIGHT); wait_us(4500);
    pulse_enable(0x03 | LCD_BACKLIGHT); wait_us(4500);
    pulse_enable(0x03 | LCD_BACKLIGHT); wait_us(150);
    pulse_enable(0x02 | LCD_BACKLIGHT);
    send_command(0x28); send_command(0x0C); send_command(0x06); send_command(0x01);
    thread_sleep_for(2);
}
bool detect_lcd() {
    char dummy = 0;
    return (i2c_lcd.write(lcd_addr << 1, &dummy, 0) == 0);
}

// --- MAIN PROGRAM ---
int main() {
    // 1. CRITICAL: Disable LSE Oscillator to free PC_14/PC_15
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSE_CONFIG(RCC_LSE_OFF);
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) != RESET);
    
    // Force pins to Output
    cyl1_ina = 0; cyl1_inb = 0;

    // 2. Initialize PWM (50kHz)
    cyl1_pwm.period(0.00002f); 
    cyl2_pwm.period(0.00002f);
    cyl1_pwm = 0.0f;
    cyl2_pwm = 0.0f;

    // 3. Safe Start Delay
    thread_sleep_for(200);

    // 4. Init LCD
    if (detect_lcd()) {
        lcd_init();
        lcd_print("Simple Cycle Test");
    }

    while (true) {
        // --- CYCLE 1: CYLINDER 1 ---
        
        // Extend Cyl 1
        cyl1_ina = 1; cyl1_inb = 0; cyl1_pwm = 1.0f;
        cyl2_ina = 0; cyl2_inb = 0; cyl2_pwm = 0.0f; // Ensure Cyl 2 is off
        led = 1;
        if (detect_lcd()) { lcd_print("C1: EXTEND   "); }
        thread_sleep_for(2000); // Wait 2 seconds

        // Retract Cyl 1
        cyl1_ina = 0; cyl1_inb = 1; cyl1_pwm = 1.0f;
        led = 0;
        if (detect_lcd()) { lcd_print("C1: RETRACT  "); }
        thread_sleep_for(2000); // Wait 2 seconds
        
        // Stop Cyl 1 briefly
        cyl1_ina = 0; cyl1_inb = 0; cyl1_pwm = 0.0f;
        thread_sleep_for(500);

        // --- CYCLE 2: CYLINDER 2 ---

        // Extend Cyl 2
        cyl1_ina = 0; cyl1_inb = 0; cyl1_pwm = 0.0f; // Ensure Cyl 1 is off
        cyl2_ina = 1; cyl2_inb = 0; cyl2_pwm = 1.0f;
        led = 1;
        if (detect_lcd()) { lcd_print("C2: EXTEND   "); }
        thread_sleep_for(2000); // Wait 2 seconds

        // Retract Cyl 2
        cyl2_ina = 0; cyl2_inb = 1; cyl2_pwm = 1.0f;
        led = 0;
        if (detect_lcd()) { lcd_print("C2: RETRACT  "); }
        thread_sleep_for(2000); // Wait 2 seconds

        // Stop Cyl 2 briefly
        cyl2_ina = 0; cyl2_inb = 0; cyl2_pwm = 0.0f;
        thread_sleep_for(500);
    }
}