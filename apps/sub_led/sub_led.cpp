#include "common.hpp"
#include <r2p/node/led.hpp>
#include <r2p/msg/led.hpp>


struct AppConfig {
    uint32_t    spin_delay_ms;
    char        led_topic_name[r2p::NamingTraits<r2p::Topic>::MAX_LENGTH + 1];
    char        sub_node_name[r2p::NamingTraits<r2p::Node>::MAX_LENGTH + 1];
} R2P_PACKED;


const AppConfig app_config R2P_APP_CONFIG = {
    500,
    "leds",
    "LED_SUB"
};


static bool led_callback(const r2p::LedMsg &msg) {

    unsigned led_pin = r2p::led2pin(msg.led);
    ioportid_t led_gpio = r2p::led2gpio(msg.led);
    
    if (msg.value) {
        palClearPad(led_gpio, led_pin);
    } else {
        palSetPad(led_gpio, led_pin);
    }
    return true;
}


extern "C"
void app_main(void) {
    
    enum { QUEUE_LENGTH = 4 };

    r2p::LedMsg sub_msgbuf[QUEUE_LENGTH], *sub_queue[QUEUE_LENGTH];
    r2p::Node node(app_config.sub_node_name);
    r2p::Subscriber<r2p::LedMsg> sub(sub_queue, QUEUE_LENGTH, led_callback);

    node.subscribe(sub, app_config.led_topic_name, sub_msgbuf);
    
    while (r2p::ok()) {
        node.spin(r2p::Time::ms(app_config.spin_delay_ms));
    }
}

