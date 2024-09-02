#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"

#define COOLER_CHANNEL    LEDC_CHANNEL_0
#define COOLER_TIMER      LEDC_TIMER_0
#define COOLER_DUTY_RES   LEDC_TIMER_8_BIT  // Resolução de 8 bits
#define COOLER_FREQUENCY  10000              // Frequência de 10 kHz para PWM
#define COOLER_PIN        22                 // Pino GPIO conectado ao cooler
#define BUF_SIZE          1024               // Tamanho do buffer para leitura UART
#define DUTY_STR_SIZE     50                 // Tamanho do buffer para a string de duty cycle

void init_uart()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void send_serial_data(const char* data)
{
    uart_write_bytes(UART_NUM_0, data, strlen(data));
}

void increase_and_stabilize_duty_cycle(int stabilize, int delay_ms)
{
    // Aumenta o duty cycle de 0 até 255
    for (int i = 0; i <= 255; i++) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
        vTaskDelay(10 / portTICK_PERIOD_MS); // Aguarda 10 ms para efeito de visualização
    }

    // Espera antes de estabilizar
    vTaskDelay(delay_ms / portTICK_PERIOD_MS);

    // Estabiliza o duty cycle
    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, stabilize);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
}

void exception_for_duty_10()
{
    // Vai para 255 e espera 5 segundos
    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, 255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
   // send_serial_data("Duty cycle 255\n");

    // Cai para 155 e espera 5 segundos
    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, 155);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  //  send_serial_data("Duty cycle 155\n");

    // Cai para 130 e espera 5 segundos
    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, 130);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
   // send_serial_data("Duty cycle 130\n");

    // Cai para 110 e estabiliza
    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, 110);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
   // send_serial_data("Duty cycle 110\n");
}

void app_main(void)
{
    // Inicializa o UART
    init_uart();

    // Configurações do PWM
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = COOLER_DUTY_RES,
        .freq_hz = COOLER_FREQUENCY,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = COOLER_TIMER,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = COOLER_CHANNEL,
        .duty = 0,  // Duty cycle inicial em 0
        .gpio_num = COOLER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = COOLER_TIMER,
    };
    ledc_channel_config(&ledc_channel);

    int last_duty = 0;
    int now_duty = 0;
    char data[BUF_SIZE];
    char stored_data[BUF_SIZE] = ""; // Buffer para armazenar os dados recebidos

    while (1) {
        // Lê dados da serial
        int length = uart_read_bytes(UART_NUM_0, (uint8_t*)data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (length > 0) {
            data[length] = '\0'; // Adiciona o caractere de terminação
            printf("Valor recebido: %s\n", data); // Exibe o valor recebido

            strncat(stored_data, data, length);

            // Verifica se foi solicitado imprimir os dados armazenados
            if (strcmp(data, "print") == 0) {
                printf("Dados armazenados: %s\n", stored_data);
            }

            // Converte o valor recebido de string para inteiro
            now_duty = atoi(data);

            // Limita o duty cycle ao intervalo de 0 a 255 (8 bits)
            if (now_duty < 0) now_duty = 0;
            if (now_duty > 255) now_duty = 255;

            // Executa ações baseadas no valor recebido
            if (now_duty == 10) {
                exception_for_duty_10();
                last_duty = 110; // Mantém o último valor válido após a exceção
            } else if (now_duty == 45) {
                increase_and_stabilize_duty_cycle(140, 5000);
                last_duty = 140;
            } else if (now_duty == 60) {
                increase_and_stabilize_duty_cycle(160, 5000);
                last_duty = 160;
            } else if (now_duty == 80) {
                increase_and_stabilize_duty_cycle(255, 0); // Permanece em 255
                last_duty = 255;
            } else if(now_duty == 0){
                increase_and_stabilize_duty_cycle(0,5000);//botao off e não sei fazer o botão on uma vez que o pwm inicializa em duty =0?
            } else {
                send_serial_data("Valor não permitido\n");
                now_duty = last_duty; // Mantém o último valor válido
            }
        } else {
            // Se a serial está vazia, usa o último duty cycle válido
            if (last_duty == 110) {
                now_duty = 110;
            } else if (last_duty == 140) {
                now_duty = 140;
            } else if (last_duty == 160) {
                now_duty = 160;
            } else if (last_duty == 255) {
                now_duty = 255;
            }
        }

        // Define o duty cycle do PWM
        ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, now_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);

        // Envia a confirmação do duty cycle pela serial
        char duty_str[DUTY_STR_SIZE];
        snprintf(duty_str, DUTY_STR_SIZE, "Duty cycle ajustado para: %d\n", now_duty);
        send_serial_data(duty_str);

        vTaskDelay(50 / portTICK_PERIOD_MS); // Aguarda 50 ms antes de ler novamente
    }
}