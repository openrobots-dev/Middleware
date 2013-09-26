#include "common.hpp"

using namespace r2p;


struct AppConfig {
    uint32_t    spin_delay_ms;
    char        led_topic_name[NamingTraits<Topic>::MAX_LENGTH + 1];
    char        sub_node_name[NamingTraits<Node>::MAX_LENGTH + 1];
};


const AppConfig app_config R2P_APP_CONFIG = {
    500,
    "LED",
    "LED_SUB"
};


static bool led_callback(const LedMsg &msg) {

    unsigned led_pin_id = led2pin(msg.id);
    
    if (msg.on) {
        palClearPad(LED_GPIO, led_pin_id);
    } else {
        palSetPad(LED_GPIO, led_pin_id);
    }
    return true;
}


extern "C"
void app_main(void) {
    
    enum { QUEUE_LENGTH = 4 };

    LedMsg sub_msgbuf[QUEUE_LENGTH], *sub_queue[QUEUE_LENGTH];
    r2p::Node node(app_config.sub_node_name);
    r2p::Subscriber<LedMsg> sub(sub_queue, QUEUE_LENGTH, led_callback);

    node.subscribe(sub, app_config.led_topic_name, sub_msgbuf);
    
    while (r2p::ok()) {
        if (!node.spin(Time::ms(app_config.spin_delay_ms))) {
            palTogglePad(LED_GPIO, LED4);
        }
    }
}

