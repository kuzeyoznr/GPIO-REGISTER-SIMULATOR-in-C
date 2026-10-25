/* GPIO register simulator: build with a C11 compiler. No hardware required. */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stddef.h>

#define GPIO_PIN_COUNT 32U
#define BUTTON_PIN 3U
#define LED_PIN 5U

/* Simplified registers: one bit per pin, including the mode register. */
struct GPIO {
    uint32_t Moder; /* 0 = input, 1 = output */
    uint32_t IDR;   /* Simulated input levels */
    uint32_t ODR;   /* Output levels */
};

enum GPIO_Mode { GPIO_INPUT = 0, GPIO_OUTPUT = 1 };

/* Check before shifting: a shift by 32 or more would be invalid. */
static int gpio_valid(const struct GPIO *gpio, uint32_t pin) {
    return gpio != NULL && pin < GPIO_PIN_COUNT;
}

/* Mutating functions return 1 on success and 0 on invalid usage. */
int gpio_set_mode(struct GPIO *gpio, uint32_t pin, enum GPIO_Mode mode) {
    if (!gpio_valid(gpio, pin) ||
        (mode != GPIO_INPUT && mode != GPIO_OUTPUT)) {
        return 0;
    }
    uint32_t mask = UINT32_C(1) << pin;
    if (mode == GPIO_OUTPUT) {
        gpio->Moder |= mask;
    } else {
        gpio->Moder &= ~mask;
    }
    return 1;
}

int gpio_set(struct GPIO *gpio, uint32_t pin) {
    if (!gpio_valid(gpio, pin)) {
        return 0;
    }
    uint32_t mask = UINT32_C(1) << pin;
    if ((gpio->Moder & mask) == 0U) {
        return 0; /* Cannot drive a pin configured as an input. */
    }
    gpio->ODR |= mask;
    return 1;
}

int gpio_clear(struct GPIO *gpio, uint32_t pin) {
    if (!gpio_valid(gpio, pin)) {
        return 0;
    }
    uint32_t mask = UINT32_C(1) << pin;
    if ((gpio->Moder & mask) == 0U) {
        return 0;
    }
    gpio->ODR &= ~mask;
    return 1;
}

/* Read IDR for an input or ODR for an output. Invalid usage returns -1. */
int gpio_read(const struct GPIO *gpio, uint32_t pin) {
    if (!gpio_valid(gpio, pin)) {
        return -1;
    }
    uint32_t mask = UINT32_C(1) << pin;
    uint32_t value = (gpio->Moder & mask) != 0U ? gpio->ODR : gpio->IDR;
    return (value & mask) != 0U;
}

/* Emulate an external device driving an input pin to 0 or 1. */
int gpio_simulate_input(struct GPIO *gpio, uint32_t pin, int level) {
    if (!gpio_valid(gpio, pin) || (level != 0 && level != 1)) {
        return 0;
    }
    uint32_t mask = UINT32_C(1) << pin;
    if ((gpio->Moder & mask) != 0U) {
        return 0;
    }
    if (level == 1) {
        gpio->IDR |= mask;
    } else {
        gpio->IDR &= ~mask;
    }
    return 1;
}

/* Active-high button: pressed = 1, released = 0. */
int update_led(struct GPIO *gpio) {
    if (gpio == NULL ||
        (gpio->Moder & (UINT32_C(1) << BUTTON_PIN)) != 0U ||
        (gpio->Moder & (UINT32_C(1) << LED_PIN)) == 0U) {
        return 0;
    }
    return gpio_read(gpio, BUTTON_PIN) == 1
        ? gpio_set(gpio, LED_PIN) : gpio_clear(gpio, LED_PIN);
}

/* Tests remain active even if the compiler defines NDEBUG. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #condition); \
        return 0; \
    } \
} while (0)

static int run_tests(void) {
    struct GPIO gpio = {0};
    CHECK(gpio_read(&gpio, 0) == 0);
    CHECK(!gpio_set(&gpio, LED_PIN));
    CHECK(!gpio_clear(&gpio, LED_PIN));
    CHECK(gpio_set_mode(&gpio, LED_PIN, GPIO_OUTPUT));
    CHECK(gpio_set_mode(&gpio, 0, GPIO_OUTPUT));
    CHECK(gpio_set_mode(&gpio, 31, GPIO_OUTPUT));
    CHECK(gpio_set(&gpio, 0));
    CHECK(gpio_set(&gpio, 31));
    CHECK(gpio_set(&gpio, LED_PIN));
    CHECK(gpio_set(&gpio, LED_PIN)); /* Setting twice is harmless. */
    CHECK(gpio.ODR == UINT32_C(0x80000021));
    CHECK(gpio_read(&gpio, 31) == 1);
    CHECK(gpio_clear(&gpio, LED_PIN));
    CHECK(gpio_clear(&gpio, LED_PIN));
    CHECK(gpio.ODR == UINT32_C(0x80000001)); /* Other bits preserved. */
    CHECK(!gpio_simulate_input(&gpio, LED_PIN, 1));
    CHECK(!gpio_simulate_input(&gpio, BUTTON_PIN, 2));
    CHECK(gpio_simulate_input(&gpio, BUTTON_PIN, 1));
    CHECK(gpio_read(&gpio, BUTTON_PIN) == 1);
    CHECK(update_led(&gpio));
    CHECK(gpio_read(&gpio, LED_PIN) == 1);
    CHECK(gpio_simulate_input(&gpio, BUTTON_PIN, 0));
    CHECK(update_led(&gpio));
    CHECK(gpio_read(&gpio, LED_PIN) == 0);
    CHECK(gpio.ODR == UINT32_C(0x80000001));

    /* Switching mode preserves stored levels; reading follows the mode. */
    CHECK(gpio_set_mode(&gpio, 31, GPIO_INPUT));
    CHECK(gpio_read(&gpio, 31) == 0);
    CHECK(gpio_set_mode(&gpio, 31, GPIO_OUTPUT));
    CHECK(gpio_read(&gpio, 31) == 1);

    struct GPIO before = gpio;
    CHECK(!gpio_set(&gpio, 32));
    CHECK(!gpio_clear(&gpio, UINT32_MAX));
    CHECK(gpio_read(&gpio, 32) == -1);
    CHECK(!gpio_set_mode(&gpio, 32, GPIO_OUTPUT));
    CHECK(!gpio_set_mode(&gpio, 0, (enum GPIO_Mode)2));
    CHECK(!gpio_simulate_input(&gpio, 32, 1));
    CHECK(gpio.Moder == before.Moder && gpio.IDR == before.IDR &&
          gpio.ODR == before.ODR);
    CHECK(!gpio_set(NULL, 0));
    CHECK(!gpio_clear(NULL, 0));
    CHECK(gpio_read(NULL, 0) == -1);
    CHECK(!gpio_set_mode(NULL, 0, GPIO_INPUT));
    CHECK(!gpio_simulate_input(NULL, 0, 1));
    CHECK(!update_led(NULL));
    CHECK(gpio_set_mode(&gpio, BUTTON_PIN, GPIO_OUTPUT));
    CHECK(!update_led(&gpio));
    CHECK(gpio_set_mode(&gpio, BUTTON_PIN, GPIO_INPUT));
    CHECK(gpio_set_mode(&gpio, LED_PIN, GPIO_INPUT));
    CHECK(!update_led(&gpio));
    puts("All GPIO tests passed.");
    return 1;
}

static int run_demo(void) {
    struct GPIO gpio = {0};
    const int button_levels[] = {0, 1, 0, 1, 0};
    if (!gpio_set_mode(&gpio, BUTTON_PIN, GPIO_INPUT) ||
        !gpio_set_mode(&gpio, LED_PIN, GPIO_OUTPUT)) {
        return 0;
    }
    puts("\nButton on input pin 3 controls LED on output pin 5:");
    for (size_t i = 0; i < sizeof button_levels / sizeof button_levels[0]; ++i) {
        if (!gpio_simulate_input(&gpio, BUTTON_PIN, button_levels[i]) ||
            !update_led(&gpio)) {
            return 0;
        }
        printf("Button %-8s | LED %-3s | IDR=0x%08" PRIX32
               " | ODR=0x%08" PRIX32 "\n",
               button_levels[i] ? "PRESSED" : "RELEASED",
               gpio_read(&gpio, LED_PIN) == 1 ? "ON" : "OFF",
               gpio.IDR, gpio.ODR);
    }
    return 1;
}

int main(void) {
    if (!run_tests() || !run_demo()) {
        fputs("GPIO simulator failed.\n", stderr);
        return 1;
    }
    return 0;
}
