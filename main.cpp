#include <cstdio>
#include <cstring>
#include <string>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "secrets.h"
#include "AsyncHttpClient.h"

static  char url[256];

AsyncHttpClient http;


void core1_entry() {
    while (true) {
        // Wait for pointer from Core 0
        uint32_t raw_ptr = multicore_fifo_pop_blocking();
        char *received_str = (char *)raw_ptr;
        
        printf("URL: %s\n", received_str);
         http.get(HTTP_SERVER, url);
        // Signal Core 0 that we are done reading the buffer
        multicore_fifo_push_blocking(1);
    }
}

int main() {



    float data1 = 1.1f;
    float data2 = 2.2f;
    float data3 = 3.3f;
  
    snprintf(url, sizeof(url), "/log/async_test/%.1f/%.1f/%.1f/1", data1, data2, data3);
    stdio_init_all();
    multicore_launch_core1(core1_entry);
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return 1;
    }

    
    int in = getchar_timeout_us(0); // Clear any pending input
    while(in != 'c')
    {
        printf("Press 'c' to start HTTP client...\n");
        in = getchar_timeout_us(0);
        sleep_ms(1000);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,!cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN)); // Toggle LED
    }


    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) 
    {
        printf("Wi-Fi connection failed\n");
        return 1;
    }
    printf("Connected!\n");

  
    snprintf(url, sizeof(url), "/log/async_test/%.2f/%.2f/%.2f/1", data1, data2, data3);
    http.get(HTTP_SERVER, url);

    // Main non-blocking loop
    uint32_t last_blink = 0;
    bool led_state = false;

    while (true) {
        // 1. Do other work (e.g., blink onboard LED every 250ms without stalling)
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_blink >= 250) {
            last_blink = now;
            led_state = !led_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
        }

        // 2. Check HTTP request state
        if (http.state == AsyncHttpClient::State::DONE) {
            printf("HTTP Response Received (%zu bytes):\n%s\n", http.response.size(), http.response.c_str());
            http.state = AsyncHttpClient::State::IDLE; // Reset state
             for (int i = 0; i < 5; i++)     
            {
                printf("Waiting 5 seconds between http requests... (%d/5)\n", i + 1);
                sleep_ms(1000);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,!cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN)); // Toggle LED
            }

            data1 += rand() % 10 / 10.0f; // Simulate some changing data
            data2 += rand() % 10 / 10.0f; // Simulate some changing data
            data3 += rand() % 10 / 10.0f; // Simulate some changing data
            snprintf(url, sizeof(url), "/log/async_test/%.1f/%.1f/%.1f/1", data1, data2, data3);
            multicore_fifo_push_blocking((uint32_t)url);
            // Wait for Core 1 acknowledgment before overwriting message_buffer
            multicore_fifo_pop_blocking();
            // http.get(HTTP_SERVER, url);
        } else if (http.state == AsyncHttpClient::State::ERROR) {
            printf("HTTP Request Failed!\n");
            http.state = AsyncHttpClient::State::IDLE;
        }

        sleep_ms(10);
    }
}
