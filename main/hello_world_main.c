#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/adc.h"

#define COOLER_CHANNEL    LEDC_CHANNEL_0
#define COOLER_TIMER      LEDC_TIMER_0
#define COOLER_DUTY_RES   LEDC_TIMER_8_BIT
#define COOLER_FREQUENCY  10000
#define COOLER_PIN        22
#define BUF_SIZE          1024
#define DUTY_STR_SIZE     50
#define ADC1_CHANNEL      ADC1_CHANNEL_6

void init_uart() {
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

void send_serial_data(const char* data) {
    uart_write_bytes(UART_NUM_0, data, strlen(data));
}

void increase_and_stabilize_duty_cycle(int stabilize, int delay_ms) {
    for (int i = 0; i <= 255; i++) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, i);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelay(delay_ms / portTICK_PERIOD_MS);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, stabilize);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);
}

void app_main(void) {
    init_uart();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL, ADC_ATTEN_DB_0);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = COOLER_DUTY_RES,
        .freq_hz = COOLER_FREQUENCY,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = COOLER_TIMER,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = COOLER_CHANNEL,
        .duty = 0,
        .gpio_num = COOLER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = COOLER_TIMER,
    };
    ledc_channel_config(&ledc_channel);

    int last_duty = 0;
    int now_duty = 0;
    char data[BUF_SIZE];

    while (1) {
        int length = uart_read_bytes(UART_NUM_0, (uint8_t*)data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        if (length > 0) {
            data[length] = '\0';
            printf("Valor recebido: %s\n", data);

            now_duty = atoi(data);

            if (now_duty < 0) now_duty = 0;
            if (now_duty > 255) now_duty = 255;

            if (now_duty == 30) {
                increase_and_stabilize_duty_cycle(30, 1000);
                last_duty = 30;
            } else if (now_duty == 65) {
                increase_and_stabilize_duty_cycle(90, 1000);
                last_duty = 90;
            } else if (now_duty == 75) {
                increase_and_stabilize_duty_cycle(145, 1000);
                last_duty = 145;
            } else if (now_duty == 80) {
                increase_and_stabilize_duty_cycle(255, 0);
                last_duty = 255;
            } else if (now_duty == 0) {
                increase_and_stabilize_duty_cycle(0, 1000);
            } else {
                send_serial_data("Valor não permitido\n");
                now_duty = last_duty;
            }
        } else {
            now_duty = last_duty;
        }

        ledc_set_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL, now_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, COOLER_CHANNEL);

        char duty_str[DUTY_STR_SIZE];
        snprintf(duty_str, DUTY_STR_SIZE, "Duty cycle ajustado para: %d\n", now_duty);
        send_serial_data(duty_str);

        int adc_reading = adc1_get_raw(ADC1_CHANNEL);

        // Mapeia o valor do ADC para a temperatura
        int angle;
        if (adc_reading < 2700) {
            angle = 80; // 80 graus
        } else if (adc_reading >= 2700 && adc_reading < 3000) {
            angle = 75; // 75 graus
        } else if (adc_reading >= 3000 && adc_reading < 4010) {
            angle = 65; // 65 graus
        } else if (adc_reading >= 4010) {
            angle = 30; // 30 graus
        } else {
            angle = 0; // Valor padrão ou fora da faixa
        }

        // Envia o valor bruto do ADC pela UART
        char adc_str[DUTY_STR_SIZE];
        snprintf(adc_str, DUTY_STR_SIZE, "ADC: %d\n", adc_reading);
        send_serial_data(adc_str);

        // Imprime o valor bruto do ADC e o ângulo calculado no terminal
        printf("ADC Reading: %d, Deslocamento do Potenciômetro: %d graus\n", adc_reading, angle);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
