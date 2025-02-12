#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define SERVO_PIN 22       // 22 SERVOMOTOR - 12 LED
#define PWM_FREQ 50  // 50Hz -> Período de 20ms
#define PWM_WRAP 12500  // Calculado para 50Hz com clock padrão de 1.25MHz
#define DELAY_MS 5000  // Tempo de espera em cada posição (5 segundos)
#define STEP_DELAY 10  // Tempo de espera entre incrementos (10ms)
#define STEP_INCREMENT 5   // Passo de incremento do duty cycle (µs)

#define STEP_US 5      // Incremento do ciclo ativo (5µs)

volatile uint current_pulse = 500;  // Começa em 0°
volatile bool increasing = true;    // Direção do movimento
uint slice;

// Callback para temporizador periódico (movimenta suavemente entre 0° e 180°)
bool timer_callback_repeating(struct repeating_timer *t) {
    if (increasing) {
        current_pulse += STEP_INCREMENT;
        if (current_pulse >= 2400) increasing = false;
    } else {
        current_pulse -= STEP_INCREMENT;
        if (current_pulse <= 500) increasing = true;
    }

    pwm_set_gpio_level(SERVO_PIN, current_pulse);
    return true; // Mantém o temporizador repetindo
}

// Callback para temporizador one-shot (espera 5 segundos e passa para o próximo estado)
int64_t timer_callback_one_shot(alarm_id_t id, void *user_data) {
    static int step = 0;

    switch (step) {
        case 0:
            pwm_set_gpio_level(SERVO_PIN, 1500); // 90°
            add_alarm_in_ms(5000, timer_callback_one_shot, NULL, false);
            break;
        case 1:
            pwm_set_gpio_level(SERVO_PIN, 500); // 0°
            add_alarm_in_ms(5000, timer_callback_one_shot, NULL, false);
            break;
        case 2:
            add_repeating_timer_ms(STEP_DELAY, timer_callback_repeating, NULL, NULL); // Inicia movimento contínuo
            break;
    }

    step++;
    return 0;
}


int main() {
    stdio_init_all();

    // Configuração do GPIO para PWM
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(SERVO_PIN);
    printf("slice : %d\n", slice);
    pwm_set_clkdiv(slice, 125.0f); // clock de 1.25MHz
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_enabled(slice, true);
    
    
    // Define servo em 180° e inicia temporizadores
    pwm_set_gpio_level(SERVO_PIN, 2400);
    add_alarm_in_ms(5000, timer_callback_one_shot, NULL, false);

    // Loop infinito (mantém a aplicação rodando)
    while (1) {
        tight_loop_contents();
    }
    
}
