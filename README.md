# LightWare Optoelectronics ROS2 driver package

## sf30c Node
This node interfaces with an SF30/C and passes laser scan data to the laserpointer topic. The SF30 laser rangefinder contains a laser and should never be aimed at a person or an animal. Do not look at the beam directly with optical instruments.

Note: The SF30 needs to be configured with the "LightWare Terminal software" or the "LightWare Studio Setup" before being used by this code. 
- The 'Output type' or 'Active data port' must be set to one of the USB options.
- The 'USB port output rate' can be configured as required.
- If you have just configured the SF30 with a terminal program, please disconnect and reconnect the USB port before running this code.

## sf40c Node

This node interfaces with an SF40/C and passes laser scan data to the laserscan topic.

Note: The SF40/C should be configured to run at a baud rate of 921600. This is the default factory setting.

### Published topics
/laserscan (sensor_msgs/LaserScan)

A full 360 degrees of scan data is published to this topic at around 5Hz.

### Parameters
~port (string, default: /dev/ttyUSB0)

The communications port used to interface with the SF40/C.

~frame_id (string, default: laser)

The transformation frame.

## sf45b Node

This node interfaces with an SF45/B and passes point cloud data to the pointcloud topic.

### Published topics
/pointcloud (sensor_msgs/PointCloud2)

A configurable number of points are published to this topic. See the maxPoints parameter to control how often this topic is updated.

Note: Only a portion of the pointcloud is updated each time.

### Parameters
~port (string, default: /dev/ttyUSB0)

The communications port used to interface with the SF45/B.

~baudrate (int, default: 115200)

The baudrate used when interfacing with the SF45/B.

~frame_id (string, default: laser)

The transformation frame.

~maxPoints (number, default: 100)

The number of distance readings taken before a result is published to the /pointcloud topic.

~updateRate (int, default: 6)

The number of distance readings taken per second.

| Value | Readings per second |
|-------|---------------------|
| 1     | 50                  |
| 2     | 100                 |
| 3     | 200                 |
| 4     | 400                 |
| 5     | 500                 |
| 6     | 625                 |
| 7     | 1000                |
| 8     | 1250                |
| 9     | 1538                |
| 10    | 2000                |
| 11    | 2500                |
| 12    | 5000                |

~cycleDelay (int, default: 5)

Controls the speed of the scan, a higher cycle delay results in a slower scan (5 to 2000).

~lowAngleLimit (float, default -45)

Lower angle limit of the scan in degrees (-160 to -10).

~highAngleLimit (float, default 45)

Higher angle limit of the scan in degrees (10 to 160).

~publishLaserScan (bool, default true)

Whether to publish a `LaserScan` message on the topic `/scan`