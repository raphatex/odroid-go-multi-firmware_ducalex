#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "input.h"

#define ODROID_GAMEPAD_IO_UP GPIO_NUM_4
#define ODROID_GAMEPAD_IO_DOWN GPIO_NUM_32
#define ODROID_GAMEPAD_IO_LEFT GPIO_NUM_33
#define ODROID_GAMEPAD_IO_RIGHT GPIO_NUM_34

#define ODROID_GAMEPAD_IO_SELECT GPIO_NUM_13
#define ODROID_GAMEPAD_IO_START GPIO_NUM_35
#define ODROID_GAMEPAD_IO_A GPIO_NUM_14
#define ODROID_GAMEPAD_IO_B GPIO_NUM_27
#define ODROID_GAMEPAD_IO_MENU GPIO_NUM_25
#define ODROID_GAMEPAD_IO_VOLUME GPIO_NUM_21

static uint32_t gamepad_state = 0;


uint32_t input_read_raw(void)
{
    uint32_t state = 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_UP)) ? (1 << ODROID_INPUT_UP) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_RIGHT)) ? (1 << ODROID_INPUT_RIGHT) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_DOWN)) ? (1 << ODROID_INPUT_DOWN) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_LEFT)) ? (1 << ODROID_INPUT_LEFT) : 0;

    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_SELECT)) ? (1 << ODROID_INPUT_SELECT) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_START)) ? (1 << ODROID_INPUT_START) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_A)) ? (1 << ODROID_INPUT_A) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_B)) ? (1 << ODROID_INPUT_B) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_MENU)) ? (1 << ODROID_INPUT_MENU) : 0;
    state |= (!gpio_get_level(ODROID_GAMEPAD_IO_VOLUME)) ? (1 << ODROID_INPUT_VOLUME) : 0;

    return state;
}

int input_wait_for_button_press(int ticks)
{
    uint32_t previousState = gamepad_state;
    uint32_t timeout = xTaskGetTickCount() + ticks;

    while (true)
    {
        uint32_t state = gamepad_state;

        for (int i = 0; i < ODROID_INPUT_MAX; i++)
        {
            if (!(previousState & (1 << i)) && (state & (1 << i))) {
                return i;
            }
        }

        if (ticks > 0 && timeout < xTaskGetTickCount()) {
            break;
        }

        previousState = state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return -1;
}

static void input_task(void *arg)
{
    uint8_t debounce[ODROID_INPUT_MAX];

    // Initialize state
    for (int i = 0; i < ODROID_INPUT_MAX; ++i)
    {
        debounce[i] = 0xff;
    }

    while (1)
    {
        // Read hardware
        uint32_t state = input_read_raw();

        // Debounce
        for (int i = 0; i < ODROID_INPUT_MAX; ++i)
        {
            debounce[i] <<= 1;
            debounce[i] |= (state >> i) & 1;
            switch (debounce[i] & 0x03)
            {
                case 0x00:
                    gamepad_state &= ~(1 << i);
                    break;

                case 0x03:
                    gamepad_state |= (1 << i);
                    break;

                default:
                    // ignore
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

void input_init(void)
{
    gpio_set_direction(ODROID_GAMEPAD_IO_UP, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_UP, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_DOWN, GPIO_MODE_INPUT);

    gpio_set_direction(ODROID_GAMEPAD_IO_LEFT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_LEFT, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_RIGHT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_RIGHT, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_SELECT, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_SELECT, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_START, GPIO_MODE_INPUT);

	gpio_set_direction(ODROID_GAMEPAD_IO_A, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_A, GPIO_PULLUP_ONLY);

    gpio_set_direction(ODROID_GAMEPAD_IO_B, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_B, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_MENU, GPIO_MODE_INPUT);
	gpio_set_pull_mode(ODROID_GAMEPAD_IO_MENU, GPIO_PULLUP_ONLY);

	gpio_set_direction(ODROID_GAMEPAD_IO_VOLUME, GPIO_MODE_INPUT);
    gpio_set_direction(ODROID_GAMEPAD_IO_VOLUME, GPIO_PULLUP_ONLY);

    // Start background polling
    xTaskCreatePinnedToCore(&input_task, "input_task", 1024 * 2, NULL, 5, NULL, 1);

}
