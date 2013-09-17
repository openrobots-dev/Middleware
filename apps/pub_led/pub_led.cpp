#include "common.hpp"

using namespace r2p;


struct AppConfig {
    char        led_topic_name[NamingTraits<Topic>::MAX_LENGTH + 1];
    char        pub_node_name[NamingTraits<Node>::MAX_LENGTH + 1];
    uint32_t    loop_delay_ms;
    unsigned    led_id;
};

const AppConfig app_config R2P_APP_CONFIG = {
    "LED",
    "LED_PUB",
    200,
    1
};


extern "C"
void app_main(void) {

    Node node(app_config.pub_node_name);
    r2p::Publisher<LedMsg> pub;

    node.advertise(pub, app_config.led_topic_name);
    
    bool was_on = false;
    while (r2p::ok()) {
        LedMsg *msgp;
        if (pub.alloc(msgp)) {
            msgp->id = app_config.led_id;
            msgp->on = !was_on;
            if (pub.publish(*msgp)) {
                was_on = !was_on;
            } else {
                palTogglePad(LED_GPIO, LED3);
            }
        } else {
            palTogglePad(LED_GPIO, LED3);
        }
        r2p::Thread::sleep(Time::ms(app_config.loop_delay_ms));
    }
}

