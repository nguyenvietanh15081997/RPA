#include "Led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static bool statusLedService = false;
static bool statusLedInternet = false;
static led_mode_t modeLedService = LED_OFF;
static led_mode_t modeLedInternet = LED_OFF;

// Mutex bảo vệ LED
static SemaphoreHandle_t led_mutex = NULL;

// esp_timer cho toggle BLINK/FLASH
static esp_timer_handle_t led_timer = NULL;

// Counter đơn giản cho toggle
static uint8_t blink_counter = 0;
static bool toogle_service_flash = false;
static bool toogle_service_blink = false;
static bool toogle_internet_flash = false;
static bool toogle_internet_blink = false;
// LEDC init
static void led_init_channels()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_GPIO_SERVICE) /*| (1ULL << LED_GPIO_INTERNET)*/;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(LED_GPIO_SERVICE, 0);
    // gpio_set_level(LED_GPIO_INTERNET, 0);
}

// Mutex lock/unlock
static void led_lock()
{
    if (led_mutex)
        xSemaphoreTake(led_mutex, portMAX_DELAY);
}
static void led_unlock()
{
    if (led_mutex)
        xSemaphoreGive(led_mutex);
}

// Timer callback
static void led_timer_callback(void *arg)
{
    blink_counter++;

    led_lock();
    // LED Service
    if (modeLedService == LED_BLINK)
    {
        if ((blink_counter % 5) == 0)
            toogle_service_blink = !toogle_service_blink;
        gpio_set_level(LED_GPIO_SERVICE, toogle_service_blink ? 1 : 0);
    }
    else if (modeLedService == LED_FLASH)
    {
        if ((blink_counter % 2) == 0)
            toogle_service_flash = !toogle_service_flash;
        gpio_set_level(LED_GPIO_SERVICE, toogle_service_flash ? 1 : 0);
    }
    else if (modeLedService == LED_ON)
        gpio_set_level(LED_GPIO_SERVICE, 1);
    else
        gpio_set_level(LED_GPIO_SERVICE, 0);

    // LED Internet
    if (modeLedInternet == LED_BLINK)
    {
        if ((blink_counter % 5) == 0)
            toogle_internet_blink = !toogle_internet_blink;
        gpio_set_level(LED_GPIO_INTERNET, toogle_internet_blink ? 1 : 0);
    }
    else if (modeLedInternet == LED_FLASH)
    {
        if ((blink_counter % 2) == 0)
            toogle_internet_flash = !toogle_internet_flash;
        gpio_set_level(LED_GPIO_INTERNET, toogle_internet_flash ? 1 : 0);
    }
    else if (modeLedInternet == LED_ON)
        gpio_set_level(LED_GPIO_INTERNET, 1);
    else
        gpio_set_level(LED_GPIO_INTERNET, 0);
    led_unlock();
}

// ----------------- Init LED module -----------------
void Led_init()
{
    if (!led_mutex)
        led_mutex = xSemaphoreCreateMutex();
    led_init_channels();

    esp_timer_create_args_t timer_args = {
        .callback = &led_timer_callback,
        .name = "led_timer"};
    esp_timer_create(&timer_args, &led_timer);
    esp_timer_start_periodic(led_timer, 100 * 1000); // 100ms tick → BLINK 1Hz
}

// ----------------- LED control -----------------
void SetLedService(bool on)
{
    modeLedService = !on ? LED_ON : LED_OFF;
    statusLedService = on;
}
void SetLedInternet(bool on)
{
    // modeLedInternet = !on ? LED_ON : LED_OFF;
    // statusLedInternet = on;
}
void BlinkLedService() { modeLedService = LED_BLINK; }
void FlashLedService() { modeLedService = LED_FLASH; }
void BlinkLedInternet() { modeLedInternet = LED_BLINK; }
void FlashLedInternet() { modeLedInternet = LED_FLASH; }

// ----------------- Lấy trạng thái -----------------
bool GetStatusLedService() { return statusLedService; }
bool GetStatusLedInternet() { return statusLedInternet; }
led_mode_t GetModeLedService() { return modeLedService; }
led_mode_t GetModeLedInternet() { return modeLedInternet; }
