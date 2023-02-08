# Zenoh-Pico ROS2 Example
This example shows on how to interface a zenoh-pico subscriber/publisher with the ROS2 DDS domain. It utilizes the CycloneDDS CDR library to do DDS IDL serialization/deserialization.
# Demo
Requirements

 - A ROS2 Distribution for example [ROS2 galactic](https://docs.ros.org/en/galactic/Installation.html). 
 - The Zenoh router
 - [Zenoh DDS plugin](https://github.com/eclipse-zenoh/zenoh-plugin-dds#how-to-install-it) to bridge Zenoh and DDS

Compilation

    $ cd /path/to/zenoh-pico
    $ mkdir build
    $ cmake .. -DROS_DISTRO=galactic # For other ROS Distro's use a different name
    $ make
  Demo setup
  

    $ zenohd & # Starts Zenoh router
    $ zenoh-bridge-dds -m client & # Starts Zenoh DDS bridge
    $ source /opt/ros/galactic/setup.bash # For other ROS Distro's use a different name
    $ ros2 topic echo /zenoh_log_test rcl_interfaces/msg/Log
On a second terminal start the Zenoh-pico publisher

    $ cd /path/to/zenoh-pico
    $ ./build/examples/z_pub_ros2
    Opening session...
    Declaring publisher for 'rt/zenoh_log_test'...
    Putting Data ('rt/zenoh_log_test')...
    Putting Data ('rt/zenoh_log_test')...
The first terminal should show the ROS2 output

    ---
    stamp:
      sec: 1675777460
      nanosec: 832124177
    level: 20
    name: zenoh_log_test
    msg: Hello from Zenoh to ROS2 encoded with CycloneDDS dds_cdrstream serializer
    file: z_pub_ros2.c
    function: z_publisher_put
    line: 138
On a third terminal you can start the Zenoh-pico subscriber

    $ cd /path/to/zenoh-pico
    $ ./build/examples/z_sub_ros2
    Opening session...
    Declaring Subscriber on 'rt/zenoh_log_test'...
    Enter 'q' to quit...
    >> [Subscriber] Received ('rt/zenoh_log_test' size '160')
    >> Time(sec=1675777461, nanosec=832501494)
    >> Log(level=20, name='zenoh_log_test', msg='Hello from Zenoh to ROS2 encoded with CycloneDDS dds_cdrstream serializer', file='z_pub_ros2.c', function='z_publisher_put', line=138)
