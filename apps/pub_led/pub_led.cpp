#include "common.hpp"
#include <r2p/node/led.hpp>
#include <r2p/msg/led.hpp>


struct AppConfig {
    char        led_topic_name[r2p::NamingTraits<r2p::Topic>::MAX_LENGTH + 1];
    char        pub_node_name[r2p::NamingTraits<r2p::Node>::MAX_LENGTH + 1];
    uint32_t    loop_delay_ms;
    unsigned    led_id;
} R2P_PACKED;


const AppConfig app_config R2P_APP_CONFIG = {
    "leds",
    "LED_PUB",
    200,
    1
};


extern "C"
void app_main(void) {

    r2p::Node node(app_config.pub_node_name);
    r2p::Publisher<r2p::LedMsg> pub;

    node.advertise(pub, app_config.led_topic_name);
    
    bool was_on = false;
    while (r2p::ok()) {
        r2p::LedMsg *msgp;
        if (pub.alloc(msgp)) {
            msgp->led = app_config.led_id;
            msgp->value = !was_on;
            if (pub.publish(*msgp)) {
                was_on = !was_on;
            }
        }
        r2p::Thread::sleep(r2p::Time::ms(app_config.loop_delay_ms));
    }
}

