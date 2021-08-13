#include "udp_bridge/udpbridge.h"
#include <ros/console.h>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "udp_bridge_node");

    
    if( ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug) ) {
        ros::console::notifyLoggerLevelsChanged();
    }
    
    udp_bridge::UDPBridge bridge;
    
    bridge.spin();
    return 0;
}

