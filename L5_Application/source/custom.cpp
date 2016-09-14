#include "custom.hpp"

#include "gpio.hpp"
#include "eint.h"
#include "FreeRTOS.h" // Dependency of semphr.h
#include "semphr.h"
#include "singleton_template.hpp"   // Dependency of LED.hpp
#include "LED.hpp"
#include "tasks.hpp" // For vTaskDelay() and scheduler_task

void button_isr();
void led_start_poll_isr();

static SemaphoreHandle_t button_semaphore = NULL;

struct GPIO_Task_Handler : public scheduler_task {
        // Note that the GPIO_Task_Handler object will NOT take ownership
        //  of the pointer to Semaphore Handler
    GPIO_Task_Handler (
        uint8_t priority,
        uint8_t led_sel, LPC1758_GPIO_Type gpio_sel,
        SemaphoreHandle_t sema
        )
        : scheduler_task("Button Task", 512*8, priority)
        , m_onboard_led(NULL)
        , k_led_sel(led_sel)
        , m_offboard_led(gpio_sel)
        , m_semaphore(sema)
    {
        m_onboard_led = &LED::getInstance();
        m_onboard_led->init();
        m_onboard_led->off(k_led_sel);

        m_offboard_led.setAsOutput();
        m_offboard_led.setLow();
    }

    bool run(void*){
        while(1){
            if(xSemaphoreTake(m_semaphore, portMAX_DELAY)){
                m_onboard_led->on(k_led_sel);
                m_offboard_led.setHigh();
                
                vTaskDelay(500);

                m_onboard_led->off(k_led_sel);
                m_offboard_led.setLow();
            }
        }
    }

    private:
        LED* m_onboard_led;
        const uint8_t k_led_sel;
        GPIO m_offboard_led;
        SemaphoreHandle_t m_semaphore;
};

struct LED_Poll_Task_Handler : public scheduler_task {
    LED_Poll_Task_Handler (
        uint8_t priority,
        LPC1758_GPIO_Type gpio_in_sel,
        uint8_t led_sel, LPC1758_GPIO_Type gpio_out_sel
        )
        : scheduler_task("Button Task", 512*8, priority)
        , m_onboard_led(NULL)
        , k_led_sel(led_sel)
        , m_offboard_led(gpio_out_sel), m_input(gpio_in_sel)
    {
        m_onboard_led = &LED::getInstance();
        m_onboard_led->init();
        m_onboard_led->off(k_led_sel);

        m_offboard_led.setAsOutput();
        m_offboard_led.setLow();

        m_input.setAsInput();
        m_input.enablePullDown();
    }

    bool run(void*){
        while(1){
            if(m_input.read()){
                m_onboard_led->on(k_led_sel);
                m_offboard_led.setHigh();
            } else {
                m_onboard_led->off(k_led_sel);
                m_offboard_led.setLow();
            }
            vTaskDelay(1);
        }
    }

    private:
        LED* m_onboard_led;
        const uint8_t k_led_sel;
        GPIO m_offboard_led, m_input;
};

void custom_main(){
        // Input for the button set up as an external interrupt
    static GPIO button_pin(P2_2);
        button_pin.setAsInput();
        button_pin.enablePullUp();
    eint3_enable_port2(2, eint_falling_edge, button_isr);

    button_semaphore = xSemaphoreCreateBinary();
    scheduler_add_task(new GPIO_Task_Handler(0, 2, P2_1, button_semaphore));    // Button task

    scheduler_add_task(new LED_Poll_Task_Handler(1, P2_3, 4, P2_0));  // LED polling task
}

void button_isr(){
        // Give the semaphore for the task to run.
    long yield = 0;
    xSemaphoreGiveFromISR(button_semaphore, &yield);
    portYIELD_FROM_ISR(yield);
}

