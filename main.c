
/*
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Raspberry Pi Pico W Project (c)                          ┃
┃                                                          ┃
┃ A program to display Raspberry Pi Pico W                 ┃
┃ Final Project Embarcatech 2025                           ┃
┃                                                          ┃
┃ Copyright (c) 2025 João Pedro de Brito Matias            ┃
┃ GitHub: github.com/sansaocarrasco                        ┃
┃ License: MIT                                             ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "lib/ssd1306.h"
#include <string.h>

// Definição dos pinos
#define BUTTON_A 5  // Pino do botão A
#define BUTTON_B 6  // Pino do botão B
#define BUZZER_PIN 10  // Pino do Buzzer

// Definições para a comunicação I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C  // Endereço do display OLED

// Definições Adicionais
#define TIME_DEBOUNCE 500  // Tempo de debounce para os botões (em milissegundos)

ssd1306_t ssd;  // Instância do display OLED
uint sm;  // Estado da máquina de estados do PIO


// Variáveis de controle de botões e Display
volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_B = 0;
volatile int common_count = 0;
volatile int priority_count = 0;
volatile int confirm_count = 0;
volatile int usb_b_count = 0;

// Filas para números comuns e prioritários
char common_queue[500][5];
char priority_queue[500][5];

// Variáveis para armazenar o número atual e próximo
char current_number_str[5] = "Vazio";
char next_number_str[5] = "Vazio";
char last_char = '\0';

// Contadores para o buzzer
int count_buzzer = 0;

//Função pra inicializar os pinos
void init_gpio() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}
// Função para inicializar o display
void init_oled() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

// Função pra atualizar as filas
void update_queue() {
    if (last_char == 'A' && priority_count < 500) {
        snprintf(priority_queue[priority_count], sizeof(priority_queue[priority_count]), "A%03d", priority_count + 1);
        priority_count++;
        last_char = '\0';
    } else if (last_char == 'B' && common_count < 500) {
        snprintf(common_queue[common_count], sizeof(common_queue[common_count]), "B%03d", common_count + 1);
        common_count++;
        usb_b_count++;
        last_char = '\0';
    }
}

// Função para atualizar o display
void update_display() {
    if (confirm_count % 3 == 2 && priority_count > 0) {
        strcpy(next_number_str, priority_queue[0]);
    } else if (common_count > 0) {
        strcpy(next_number_str, common_queue[0]);
    }
    
    ssd1306_fill(&ssd, false);
    char display_string[64];
    snprintf(display_string, sizeof(display_string), "Atual: %s", current_number_str);
    ssd1306_draw_string(&ssd, display_string, 5, 10);
    snprintf(display_string, sizeof(display_string), "Proximo: %s", next_number_str);
    ssd1306_draw_string(&ssd, display_string, 5, 30);
    ssd1306_send_data(&ssd);
}

//Função de callback para os botões
void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (gpio == BUTTON_A && (current_time - last_press_time_A) > TIME_DEBOUNCE) {
        last_press_time_A = current_time;
        if (confirm_count % 3 == 2 && priority_count > 0) {
            strcpy(current_number_str, priority_queue[0]);
            for (int i = 0; i < priority_count - 1; i++) {
                strcpy(priority_queue[i], priority_queue[i + 1]);
            }
            priority_count--;
        } else if (common_count > 0) {
            strcpy(current_number_str, common_queue[0]);
            for (int i = 0; i < common_count - 1; i++) {
                strcpy(common_queue[i], common_queue[i + 1]);
            }
            common_count--;
        }
        confirm_count++;
        update_display();
        if (count_buzzer)
            gpio_put(BUZZER_PIN, 1);
    }
    
    if (gpio == BUTTON_B && (current_time - last_press_time_B) > TIME_DEBOUNCE) {
        last_press_time_B = current_time;
        count_buzzer = !count_buzzer;
        gpio_put(BUZZER_PIN, count_buzzer);
    }
}

// Função para comunicação USB
void comunicacao_usb() {
    if (stdio_usb_connected()) {
        char c = getchar();
        if (c != EOF) {
            last_char = c;
            if (c == 'A' || c == 'B') {
                update_queue();
            }
        }
    }
}
// Função principal
int main() {
    // Inicializa os pinos e o display
    stdio_init_all();
    init_oled();
    init_gpio();

    // Configura as interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    // Inicializa o display
    update_display();

    while (1) {
        // Atualiza as filas e o display a partir dos caracters recebidos
        comunicacao_usb();
        sleep_ms(100);
    }
    return 0;
}
