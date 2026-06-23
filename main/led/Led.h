/**
 * @file Led.h
 * @brief Khai báo các hàm và cấu hình điều khiển LED trạng thái và GPIO reset BLE.
 *
 * File này cung cấp các định nghĩa và hàm để điều khiển LED hiển thị trạng thái dịch vụ,
 * trạng thái kết nối Internet, và reset module BLE thông qua GPIO.
 */

#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdbool.h>

/**
 * @enum LED_Mode LED chế độ hoạt động
 */
typedef enum
{
    LED_OFF = 0, /**< LED tắt  */
    LED_ON,      /**< LED bật */
    LED_BLINK,   /**< LED nhấp nháy chậm */
    LED_FLASH    /**< LED nhấp nháy nhanh */
} led_mode_t;

/**
 * @enum khai báo gpio num
 */
#define LED_GPIO_SERVICE GPIO_NUM_12            /**< GPIO điều khiển LED Service. */
#define LED_GPIO_INTERNET GPIO_NUM_18           /**< GPIO điều khiển LED Internet. */

/**
 * @brief Khởi tạo hệ thống điều khiển LED.
 *
 * Thiết lập bộ LED, cấu hình các chân GPIO và trạng thái ban đầu của LED.
 */
void Led_init();

/**
 * @brief Bật hoặc tắt LED Internet.
 *
 * @param status true để bật, false để tắt.
 */
void SetLedInternet(bool on);

/**
 * @brief Bật hoặc tắt LED Service.
 *
 * @param status true để bật, false để tắt.
 */
void SetLedService(bool on);

/**
 * @brief Lấy trạng thái hiện tại của LED Internet.
 *
 * @return true nếu LED đang bật.
 */
bool GetStatusLedInternet();

/**
 * @brief Lấy trạng thái hiện tại của LED Service.
 *
 * @return true nếu LED đang bật.
 */
bool GetStatusLedService();

/**
 * @brief Bật chế độ nhấp nháy chậm cho LED Service.
 */
void BlinkLedService();

/**
 * @brief Bật chế độ nhấp nháy nhanh cho LED Service.
 */
void FlashLedService();

/**
 * @brief Bật chế độ nhấp nháy chậm cho LED Internet.
 */
void BlinkLedInternet();

/**
 * @brief Bật chế độ nhấp nháy nhanh cho LED Internet.
 */
void FlashLedInternet();

/**
 * @brief Lấy chế độ hoạt động hiện tại của LED Service.
 *
 * @return Một trong các giá trị LED_ON_OFF, LED_BLINK, LED_FLASH.
 */
led_mode_t GetModeLedService();

/**
 * @brief Lấy chế độ hoạt động hiện tại của LED Internet.
 *
 * @return Một trong các giá trị LED_ON_OFF, LED_BLINK, LED_FLASH.
 */
led_mode_t GetModeLedInternet();
