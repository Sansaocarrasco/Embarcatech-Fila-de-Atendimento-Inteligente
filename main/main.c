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

// Fila única para armazenar números comuns e prioritários
char queue[800]; // Considerando que teremos até 400 números no total (200 comuns + 200 prioritários)

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

void update_display(const char* current, const char* next) {
    ssd1306_fill(&ssd, false);  // Limpa o display
    char display_string[64];
    snprintf(display_string, sizeof(display_string), "Atual: %s", current);
    ssd1306_draw_string(&ssd, display_string, 5, 10);  // Exibe o número atual
    snprintf(display_string, sizeof(display_string), "Proximo: %s", next);
    ssd1306_draw_string(&ssd, display_string, 5, 30);  // Exibe o próximo número
    ssd1306_send_data(&ssd);  // Envia os dados ao display
}

// Função para atualizar a fila
void update_queue(char type) {
    char number_str[6];  // "A001", "B001" tem 5 caracteres mais o terminador nulo
    if (type == 'A') {
        snprintf(number_str, sizeof(number_str), "A%03d", current_number++);
    } else if (type == 'B') {
        snprintf(number_str, sizeof(number_str), "B%03d", current_number++);
    }

    // Adiciona o número à fila
    strcat(queue, number_str);

    // Se dois números comuns foram inseridos, insira um prioritário
    if (type == 'A' && count_common == 1) {
        char priority_str[6];
        snprintf(priority_str, sizeof(priority_str), "P%03d", current_number++);
        strcat(queue, priority_str);  // Adiciona número prioritário à fila
    }

    count_common = (type == 'A') ? (count_common + 1) % 2 : count_common; // Reseta após 2 números comuns
}

void button_callback(uint gpio, uint32_t events){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
  
    //Botão A
    if (gpio == BUTTON_A && current_time - last_press_time_A > TIME_DEBOUNCE) {

      if (count_buzzer)
      gpio_put(BUZZER_PIN, 1);  // Acionar o buzzer
  
      /*
      Adicionar interação com as filas (prioridade e comum) para aparecer no 
      display o numero atual a ser atendido e o próximo numero
      */
    }
  
    // Botão B
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
            if (c == 'A' || c == 'B') {
                update_queue(c);  // Adiciona o número à fila
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
      // Exibir continuamente o número atual e o próximo número no OLED
      // Atualiza os números atuais e próximos
      char current_number_str[6], next_number_str[6];
      if (strlen(queue) > 0) {
          // Pega o primeiro número da fila
          snprintf(current_number_str, sizeof(current_number_str), "%s", queue); // Exibe o primeiro número
          // Pega o próximo número da fila
          snprintf(next_number_str, sizeof(next_number_str), "%s", queue + strlen(current_number_str)); // Pega o próximo número
      } else {
          snprintf(current_number_str, sizeof(current_number_str), "Vazio");
          snprintf(next_number_str, sizeof(next_number_str), "Vazio");
      }

      update_display(current_number_str, next_number_str);

      comunicacao_usb();  // Atualiza a fila com entradas via USB

      sleep_ms(100);  // Atraso para evitar leitura muito rápida
    }

    return 0;
}

