/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "i2c_master.h"

#define I2C_MASTER_NUM      I2C_NUM_0           // I2C port number
#define I2C_MASTER_SDA_IO   GPIO_NUM_19      // GPIO for SDA
#define I2C_MASTER_SCL_IO   GPIO_NUM_18      // GPIO for SCL
#define I2C_MASTER_FREQ_HZ  200000          // I2C frequency (200 kHz)
#define I2C_MASTER_TX_BUF_DISABLE 0       
#define I2C_MASTER_RX_BUF_DISABLE 0   

/**
 * @brief hàm gửi dữ liệu
 * 
 * @param data dữ liệu gửi đi
 * @param len độ dài dữ liệu gửi đi
 * @return 0: gửi thành công
 *         1: gửi thất bại
 */
uint8_t i2c_send_bytes(uint8_t *data, size_t len, uint8_t slave_addr) {
    uint8_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, slave_addr, data, len, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK) {
        return 0;
    } else {
        ESP_LOGE("MASTER" ,"I2C write failed: %s\n", esp_err_to_name(ret));
        return 1;
    }

    return 0;
}

/**
 * @brief hàm đọc dữ liệu về
 * 
 * @param data con trỏ tới vùng dữ liệu nhận về
 * @param len độ dài dữ liệu nhận về
 * @return 0: đọc dữ liệu thành công
 *         1: đọc dữ liệu thất bại 
 */
uint8_t i2c_read_bytes(uint8_t *data, size_t len, uint8_t slave_addr) {
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, slave_addr, 
                                      data, len, 1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        ESP_LOGI("MASTER","I2C read successful: ");
        for (size_t i = 0; i < len; i++) {
            printf("0x%02X ", data[i]);
        }
        printf("\n");
        return 0;
    } else {
        ESP_LOGE("MASTER" ,"I2C read failed: %s\n", esp_err_to_name(ret));
        return 1;
    }
}

uint8_t i2c_read_register(uint8_t target_index, uint8_t slave_addr)
{
    size_t len_to_read = target_index+1; 
    uint8_t rx_buffer[len_to_read]; 

    // Gọi hàm đọc thông thường (nó luôn bắt đầu kéo từ index 0 của Telink)
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM,    slave_addr, 
                                                rx_buffer, len_to_read, 
                                                1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        // Trích xuất đúng byte bạn cần và trả về
        ESP_LOGE("MASTER", "I2C read successful, target index: %d, value: 0x%02X", target_index, rx_buffer[target_index]);
        return rx_buffer[target_index]; 
    } else {
        ESP_LOGE("MASTER", "I2C read failed: %s", esp_err_to_name(ret));
        return 0xFF; // Hoặc trả về một mã lỗi nào đó
    }
}

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE("MASTER" ,"I2C param config failed: %s\n", esp_err_to_name(ret));
        return;
    }
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                             I2C_MASTER_RX_BUF_DISABLE, 
                             I2C_MASTER_TX_BUF_DISABLE, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("MASTER" ,"I2C driver install failed: %s\n", esp_err_to_name(ret));
        return;
    }
}

// void app_main_test(void)
// {
//     // Configure I2C parameters
//     i2c_config_t i2c_config = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = I2C_MASTER_SDA_IO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_io_num = I2C_MASTER_SCL_IO,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = I2C_MASTER_FREQ_HZ,
//     };

//     // Apply configuration
//     i2c_param_config(I2C_MASTER_NUM, &i2c_config);

//     // Install I2C driver
//     i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 
//                        I2C_MASTER_RX_BUF_DISABLE, 
//                        I2C_MASTER_TX_BUF_DISABLE, 0);
//     uint8_t data_send[3] = {0x55,0,0};
    
//     while (1)
//     {
//         // i2c_send_bytes(&data_send[0],1, SLAVE_ADDR_1);
//         uint8_t data_rec[3] = {0};
//         i2c_read_bytes(data_rec, 3, SLAVE_ADDR_1);
//         ESP_LOGI("MASTER","Data received slave1: 0x%02X 0x%02X 0x%02X\n", data_rec[0], data_rec[1], data_rec[2]);

//         vTaskDelay(2000/portTICK_PERIOD_MS);
//         // i2c_send_bytes(&data_send[0],1, SLAVE_ADDR_2);
//         i2c_read_bytes(data_rec, 3, SLAVE_ADDR_2);
//         ESP_LOGI("MASTER","Data received slave2: 0x%02X 0x%02X 0x%02X\n", data_rec[0], data_rec[1], data_rec[2]);

//         vTaskDelay(2000/portTICK_PERIOD_MS);
//     }
    
// }
