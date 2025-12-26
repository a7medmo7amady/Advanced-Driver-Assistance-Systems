/*
 * Obstacle Avoiding Robot - Pure Embedded C
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

// ==================== PARAMETERS ====================
#define OBSTACLE_CM     20
#define DRIVE_PWM       200
#define TURN_90_TIME    450
#define TURN_180_TIME   900

// ==================== UART ====================
void uart_init(uint32_t baud) {
    uint16_t ubrr = F_CPU / 16 / baud - 1;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send_char(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_print(const char* str) {
    while (*str) uart_send_char(*str++);
}

void uart_println(const char* str) {
    uart_print(str);
    uart_send_char('\r');
    uart_send_char('\n');
}

// ==================== PWM ====================
void pwm_init(void) {
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);  // Prescaler 64
    OCR1A = 0;
    OCR1B = 0;
}

// ==================== MOTORS ====================
void motors_init(void) {
    DDRB |= (1 << PB1) | (1 << PB2);
    DDRD |= (1 << PD3) | (1 << PD4) | (1 << PD5) | (1 << PD6);
    pwm_init();
}

void set_motors(int16_t left_speed, int16_t right_speed) {
    if (left_speed > 255) left_speed = 255;
    if (left_speed < -255) left_speed = -255;
    if (right_speed > 255) right_speed = 255;
    if (right_speed < -255) right_speed = -255;
    
    if (left_speed >= 0) {
        PORTD |= (1 << PD3);
        PORTD &= ~(1 << PD4);
        OCR1A = (uint8_t)left_speed;
    } else {
        PORTD &= ~(1 << PD3);
        PORTD |= (1 << PD4);
        OCR1A = (uint8_t)(-left_speed);
    }
    
    if (right_speed >= 0) {
        PORTD |= (1 << PD5);
        PORTD &= ~(1 << PD6);
        OCR1B = (uint8_t)right_speed;
    } else {
        PORTD &= ~(1 << PD5);
        PORTD |= (1 << PD6);
        OCR1B = (uint8_t)(-right_speed);
    }
}

void move_forward(void) { set_motors(DRIVE_PWM, DRIVE_PWM); }
void stop_motors(void) { set_motors(0, 0); }

void delay_ms(uint16_t ms) {
    while (ms--) _delay_ms(1);
}

void pivot_right_90(void) {
    set_motors(DRIVE_PWM, -DRIVE_PWM);
    delay_ms(TURN_90_TIME);
    stop_motors();
}

void pivot_left_90(void) {
    set_motors(-DRIVE_PWM, DRIVE_PWM);
    delay_ms(TURN_90_TIME);
    stop_motors();
}

void spin_180(void) {
    set_motors(DRIVE_PWM, -DRIVE_PWM);
    delay_ms(TURN_180_TIME);
    stop_motors();
}

// ==================== SENSOR ====================
void sensor_init(void) {
    DDRB |= (1 << PB4);
    DDRB &= ~(1 << PB3);
}

long read_distance_cm(void) {
    PORTB &= ~(1 << PB4);
    _delay_us(2);
    PORTB |= (1 << PB4);
    _delay_us(10);
    PORTB &= ~(1 << PB4);
    
    uint16_t timeout = 0;
    while (!(PINB & (1 << PB3))) {
        _delay_us(1);
        if (++timeout > 20000) return 999;
    }
    
    uint16_t duration = 0;
    while (PINB & (1 << PB3)) {
        _delay_us(1);
        duration++;
        if (duration > 20000) return 999;
    }
    
    long cm = duration / 58;
    return (cm >= 2 && cm <= 200) ? cm : 999;
}

long check_right(void) {
    pivot_right_90();
    delay_ms(100);
    long dist = read_distance_cm();
    pivot_left_90();
    delay_ms(50);
    return dist;
}

long check_left(void) {
    pivot_left_90();
    delay_ms(100);
    long dist = read_distance_cm();
    pivot_right_90();
    delay_ms(50);
    return dist;
}

// ==================== MAIN ====================
int main(void) {
    uart_init(9600);
    uart_println("=== Obstacle Robot ===");
    
    motors_init();
    sensor_init();
    
    stop_motors();
    delay_ms(1000);
    
    uart_println("Ready!");
    
    while (1) {
        long fwd = read_distance_cm();
        
        if (fwd > OBSTACLE_CM) {
            move_forward();
        } else {
            stop_motors();
            
            uart_println("FWD blocked");
            long right_d = check_right();
            
            if (right_d > OBSTACLE_CM) {
                uart_println("Turn RIGHT");
                pivot_right_90();
            } else {
                long left_d = check_left();
                
                if (left_d > OBSTACLE_CM) {
                    uart_println("Turn LEFT");
                    pivot_left_90();
                } else {
                    uart_println("SPIN 180");
                    spin_180();
                }
            }
        }
        
        delay_ms(100);
    }
    
    return 0;
}