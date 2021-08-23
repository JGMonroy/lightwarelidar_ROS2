//----------------------------------------------------------------------------------------------------------------------------------
// LightWare SF45B ROS driver.
//----------------------------------------------------------------------------------------------------------------------------------
#include "common.h"
#include "lwNx.h"

#include <math.h>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

struct lwSf45Params {
	int32_t updateRate;
	int32_t cycleDelay;
	float lowAngleLimit;
	float highAngleLimit;
};

std::shared_ptr<rclcpp::Node> node;

void validateParams(lwSf45Params* Params) {
	if (Params->updateRate < 1) Params->updateRate = 1;
	else if (Params->updateRate > 12) Params->updateRate = 12;

	if (Params->cycleDelay < 5) Params->cycleDelay = 5;
	else if (Params->cycleDelay > 2000) Params->cycleDelay = 2000;

	if (Params->lowAngleLimit < -160) Params->lowAngleLimit = -160;
	else if (Params->lowAngleLimit > -10) Params->lowAngleLimit = -10;

	if (Params->highAngleLimit < 10) Params->highAngleLimit = 10;
	else if (Params->highAngleLimit > 160) Params->highAngleLimit = 160;
}

int driverStart(lwSerialPort** Serial, const char* PortName, int32_t BaudRate) {
	platformInit();

	lwSerialPort* serial = platformCreateSerialPort();
	*Serial = serial;
	if (!serial->connect(PortName, BaudRate)) {
		RCLCPP_ERROR(node->get_logger(), "Could not establish serial connection on %s", PortName);
		return 1;
	};

	// Disable streaming of point data. (Command 30: Stream)
	if (!lwnxCmdWriteUInt32(serial, 30, 0)) { return 1; }

	// Read the product name. (Command 0: Product name)
	char modelName[16];
	if (!lwnxCmdReadString(serial, 0, modelName)) { return 1; }

	// Read the hardware version. (Command 1: Hardware version)
	uint32_t hardwareVersion;
	if (!lwnxCmdReadUInt32(serial, 1, &hardwareVersion)) { return 1; }

	// Read the firmware version. (Command 2: Firmware version)
	uint32_t firmwareVersion;	
	if (!lwnxCmdReadUInt32(serial, 2, &firmwareVersion)) { return 1; }
	char firmwareVersionStr[16];
	lwnxConvertFirmwareVersionToStr(firmwareVersion, firmwareVersionStr);

	// Read the serial number. (Command 3: Serial number)
	char serialNumber[16];
	if (!lwnxCmdReadString(serial, 3, serialNumber)) { return 1; }

	RCLCPP_INFO(node->get_logger(), "Model: %.16s", modelName);
	RCLCPP_INFO(node->get_logger(), "Hardware: %d", hardwareVersion);
	RCLCPP_INFO(node->get_logger(), "Firmware: %.16s (%d)", firmwareVersionStr, firmwareVersion);
	RCLCPP_INFO(node->get_logger(), "Serial: %.16s", serialNumber);

	return 0;
}

int driverScanStart(lwSerialPort* Serial, lwSf45Params* Params) {
	// Configre distance output for first return and angle. (Command 27: Distance output)
	if (!lwnxCmdWriteUInt32(Serial, 27, 0x101)) { return 1; }

	// (Command 66: Update rate)
	if (!lwnxCmdWriteUInt8(Serial, 66, Params->updateRate)) { return 1; }

	// (Command 85: Scan speed)
	if (!lwnxCmdWriteUInt16(Serial, 85, Params->cycleDelay)) { return 1; }

	// (Command 98: Scan low angle)
	if (!lwnxCmdWriteFloat(Serial, 98, Params->lowAngleLimit)) { return 1; }

	// (Command 99: Scan high angle)
	if (!lwnxCmdWriteFloat(Serial, 99, Params->highAngleLimit)) { return 1; }

	// Enable streaming of point data. (Command 30: Stream)
	if (!lwnxCmdWriteUInt32(Serial, 30, 5)) { return 1; }

	return 0;
}

struct lwDistanceResult {
	float x;
	float y;
	float z;
};

int driverScan(lwSerialPort* Serial, lwDistanceResult* DistanceResult) {
	// The incoming point data packet is Command 44: Distance data in cm.
	lwResponsePacket response;

	if (lwnxRecvPacket(Serial, 44, &response, 1000)) {
		int16_t distanceCm = (response.data[5] << 8) | response.data[4];
		int16_t angleHundredths = (response.data[7] << 8) | response.data[6];

		float distance = distanceCm / 100.0f;
		float angle = angleHundredths / 100.0f;
		float faceAngle = (angle - 90) * M_PI / 180.0;

		DistanceResult->x = distance * -cos(faceAngle);
		DistanceResult->y = distance * sin(faceAngle);
		DistanceResult->z = 0;

		return 1;
	}

	return 0;
}

int main(int argc, char** argv) {
	rclcpp::init(argc, argv);

	node = rclcpp::Node::make_shared("sf45b");

	RCLCPP_INFO(node->get_logger(), "Starting SF45B node");
	
	auto pointCloudPub = node->create_publisher<sensor_msgs::msg::PointCloud2>("pointcloud", 4);
		
	lwSerialPort* serial = 0;

	int32_t baudRate = node->declare_parameter<int32_t>("baudrate", 115200);
	std::string portName = node->declare_parameter<std::string>("port", "/dev/ttyUSB0");
	std::string frameId = node->declare_parameter<std::string>("frameId", "laser");

	lwSf45Params params;
	params.updateRate = node->declare_parameter<int32_t>("updateRate", 6); // 1 to 12
	params.cycleDelay = node->declare_parameter<int32_t>("cycleDelay", 5); // 5 to 2000
	params.lowAngleLimit = node->declare_parameter<int32_t>("lowAngleLimit", -45.0f); // -160 to -10
	params.highAngleLimit = node->declare_parameter<int32_t>("highAngleLimit", 45.0f); // 10 to 160
	validateParams(&params);
	
	int32_t maxPointsPerMsg = node->declare_parameter<int32_t>("maxPoints", 100); // 1 to ...
	if (maxPointsPerMsg < 1) maxPointsPerMsg = 1;
	
	if (driverStart(&serial, portName.c_str(), baudRate) != 0) {
		RCLCPP_ERROR(node->get_logger(), "Failed to start driver");
		return 1;
	}

	if (driverScanStart(serial, &params) != 0) {
		RCLCPP_ERROR(node->get_logger(), "Failed to start scan");
		return 1;
	}

	sensor_msgs::msg::PointCloud2 pointCloudMsg;
	pointCloudMsg.header.frame_id = frameId;
	pointCloudMsg.height = 1;
	pointCloudMsg.width = maxPointsPerMsg;
	
	pointCloudMsg.fields.resize(3);
	pointCloudMsg.fields[0].name = "x";
	pointCloudMsg.fields[0].offset = 0;	
	pointCloudMsg.fields[0].datatype = 7;
	pointCloudMsg.fields[0].count = 1;

	pointCloudMsg.fields[1].name = "y";
	pointCloudMsg.fields[1].offset = 4;
	pointCloudMsg.fields[1].datatype = 7;
	pointCloudMsg.fields[1].count = 1;
	
	pointCloudMsg.fields[2].name = "z";
	pointCloudMsg.fields[2].offset = 8;
	pointCloudMsg.fields[2].datatype = 7;
	pointCloudMsg.fields[2].count = 1;

	pointCloudMsg.is_bigendian = false;
	pointCloudMsg.point_step = 12;
	pointCloudMsg.row_step = 12 * maxPointsPerMsg;
	pointCloudMsg.is_dense = true;

	pointCloudMsg.data = std::vector<uint8_t>(maxPointsPerMsg * 12);

	int currentPoint = 0;
	std::vector<lwDistanceResult> distanceResults(maxPointsPerMsg);

	while (rclcpp::ok()) {

		while (true) {
			lwDistanceResult distanceResult;
			int status = driverScan(serial, &distanceResult);

			if (status == 0) {
				break;
			} else {
				distanceResults[currentPoint] = distanceResult;
				++currentPoint;
			}

			if (currentPoint == maxPointsPerMsg) {
				memcpy(&pointCloudMsg.data[0], &distanceResults[0], maxPointsPerMsg * 12);

				pointCloudMsg.header.stamp = node->now();
				pointCloudPub->publish(pointCloudMsg);
				
				currentPoint = 0;
			}
		}

		rclcpp::spin_some(node);
	}

	rclcpp::shutdown();

	return 0;
}
