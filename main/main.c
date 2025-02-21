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
#include "hardware/pio.h"
#include "hardware/clocks.h"
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
volatile int current_number = 1;

int last_char;
int count_buzzer = 0; //Buzzer começa desativado
int count_common = 0;
int count_priority = 0;

char list_common_numbers[400]; // 100 chamadas de numeros comuns
char list_priority_numbers[400]; // 100 chamadas de numeros prioritarios

// Função para inicializar os botões e o buzzer
void init_gpio() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);  // Entrada para botão A
    gpio_pull_up(BUTTON_A);           // Habilitar pull-up

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);  // Entrada para botão B
    gpio_pull_up(BUTTON_B);           // Habilitar pull-up

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);  // Saída para o buzzer
}

// Função pra inicializar o display OLED
void init_oled(){
  // Configura a interface I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Botão A
    if (gpio == BUTTON_A && current_time - last_press_time_A > TIME_DEBOUNCE) {
        // Limpa o display
        ssd1306_fill(&ssd, false);

        // Formatar os números a serem exibidos
        char current_str[20];
        char next_str[20];

        snprintf(current_str, sizeof(current_str), "Atual: %03d", current_number);
        snprintf(next_str, sizeof(next_str), "Prox: %03d", current_number + 1);

        // Desenha as strings no display
        ssd1306_draw_string(&ssd, current_str, 5, 15);  // Número atual
        ssd1306_draw_string(&ssd, next_str, 5, 30);     // Próximo número

        ssd1306_send_data(&ssd);  // Envia os dados ao display

        // Aciona o buzzer se estiver ativado
        if (count_buzzer)
            gpio_put(BUZZER_PIN, 1);

        last_press_time_A = current_time; // Atualiza o tempo da última pressão
    }

    // Botão B (controle do buzzer)
    if (gpio == BUTTON_B && current_time - last_press_time_B > TIME_DEBOUNCE) {
        last_press_time_B = current_time; // Atualiza o tempo da última pressão
        count_buzzer = !count_buzzer;  // Alterna o estado do buzzer
        gpio_put(BUZZER_PIN, count_buzzer);  // Atualiza o estado do pino
    }
}

void comunicacao_usb() {
    if (stdio_usb_connected()) {  // Verifica se a comunicação USB está conectada
        char c = getchar();  // Lê um caractere da entrada
        if (c != EOF) {  // Se for um caractere válido
            last_char = c;  // Armazena o caractere
            char prox_number[5];  // Para armazenar o número formatado
            if (c >= 'A' && c <= 'B') {  // Se for 'A' ou 'B'
                if (c == 'A') {
                    snprintf(prox_number, sizeof(prox_number), "A%03d", current_number);
                    if (strlen(list_common_numbers) + strlen(prox_number) < sizeof(list_common_numbers) - 1) {
                        strcat(list_common_numbers, prox_number);  // Armazena em list_common_numbers
                    }
                } else if (c == 'B') {
                    snprintf(prox_number, sizeof(prox_number), "B%03d", current_number);
                    if (strlen(list_priority_numbers) + strlen(prox_number) < sizeof(list_priority_numbers) - 1) {
                        strcat(list_priority_numbers, prox_number);  // Armazena em list_priority_numbers
                    }
                }

                current_number += 1;  // Incrementa o número
            }
        }
    }
}


int main() {
    // Inicializar os componentes
    stdio_init_all();
    init_oled();
    init_gpio();

    // Configura as interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    while (1) {

      // Exibir uma mensagem inicial no OLED
      ssd1306_fill(&ssd, false);  // Limpa o display
      ssd1306_draw_string(&ssd, "Aguardando", 5, 15);  // Exibe a mensagem
      gpio_put(BUZZER_PIN, 0);  // Acionar o buzzer

      comunicacao_usb();
      sleep_ms(100);  // Atraso para evitar leitura muito rápida
    }

    return 0;
}
