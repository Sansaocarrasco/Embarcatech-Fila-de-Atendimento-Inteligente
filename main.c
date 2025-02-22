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

int last_char = '\0';
int count_buzzer = 0; // Buzzer começa desativado

// Filas para números comuns e prioritários
char common_queue[500][5];  // Fila para números comuns
char priority_queue[500][5];  // Fila para números prioritários
int common_count = 0;  // Contador para números comuns
int priority_count = 0;  // Contador para números prioritários
int confirm_count = 0;

// Variáveis para armazenar o número atual e próximo
char current_number_str[5] = "Vazio";
char next_number_str[5] = "Vazio";

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

// Função para inicializar o display OLED
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

// Função para atualizar a fila de comum ou prioritário
void update_queue() {
    // Adicionar números na fila comum ou prioritária, dependendo do caractere pressionado
    if (last_char == 'A' && priority_count < 500) {
        // Gerar o número no formato A001, A002, etc., para a fila prioritária
        snprintf(priority_queue[priority_count], sizeof(priority_queue[priority_count]), "A%03d", priority_count);
        priority_count++;  // Incrementa o contador da fila prioritária
        last_char = '\0';
    }
    else if (last_char == 'B' && common_count < 500) {
        // Gerar o número no formato B001, B002, etc., para a fila comum
        snprintf(common_queue[common_count], sizeof(common_queue[common_count]), "B%03d", common_count);
        common_count++;  // Incrementa o contador da fila comum
        last_char = '\0';
    }
}

void update_display() {
    

    if (priority_count > 0 && (common_count % 2 == 0)) {
        // Exibe da fila prioritária
        strcpy(next_number_str, priority_queue[priority_count - 1]); // Último número na fila prioritária
    }
    else if (common_count > 0) {
        // Exibe da fila comum
        strcpy(next_number_str, common_queue[common_count - 1]);  // Último número na fila comum
    }

    ssd1306_fill(&ssd, false);  // Limpa o display
    char display_string[64];
    
    // Exibe o número atual
    snprintf(display_string, sizeof(display_string), "Atual: %s", current_number_str);
    ssd1306_draw_string(&ssd, display_string, 5, 10);  // Exibe o número atual no display

    // Exibe o próximo número
    snprintf(display_string, sizeof(display_string), "Proximo: %s", next_number_str);
    ssd1306_draw_string(&ssd, display_string, 5, 30);  // Exibe o próximo número no display
    
    ssd1306_send_data(&ssd);  // Envia os dados ao display
}


void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
  
    // Botão A
    if (gpio == BUTTON_A && current_time - last_press_time_A > TIME_DEBOUNCE) {
        // Trocar o número atual com o próximo
        strcpy(current_number_str, next_number_str);
        update_display();
        if (count_buzzer)
            gpio_put(BUZZER_PIN, 1);  // Aciona o buzzer
    }
  
    // Botão B
    if (gpio == BUTTON_B && current_time - last_press_time_B > TIME_DEBOUNCE) {
        last_press_time_B = current_time;  // Atualiza o tempo da última pressão
        count_buzzer = !count_buzzer;  // Alterna o estado do buzzer
        gpio_put(BUZZER_PIN, count_buzzer);  // Atualiza o estado do pino do buzzer
    }
}

void comunicacao_usb() {
    if (stdio_usb_connected()) {  // Verifica se a comunicação USB está conectada
        char c = getchar();  // Lê um caractere da entrada
        if (c != EOF) {  // Se for um caractere válido
            last_char = c;  // Armazena o caractere
            if (c == 'A' || c == 'B') {
                update_queue();  // Adiciona o número à fila quando o caractere é 'A' ou 'B'
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
    update_display();

    while (1) {
    
        comunicacao_usb();  // Atualiza a fila com entradas via USB

        sleep_ms(100);  // Atraso para evitar leitura muito rápida
    }

    return 0;
}