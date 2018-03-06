#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include "hesaiLidarSDK.h"
using namespace std;

class HesaiLidarClient
{
public:
  HesaiLidarClient(ros::NodeHandle node, ros::NodeHandle nh)
  {
    lidarPublisher = node.advertise<sensor_msgs::PointCloud2>("pandar_points", 10);

    image_transport::ImageTransport it(nh);
    imgPublishers[0] = it.advertise("pandora_camera0", 1);
    imgPublishers[1] = it.advertise("pandora_camera1", 1);
    imgPublishers[2] = it.advertise("pandora_camera2", 1);
    imgPublishers[3] = it.advertise("pandora_camera3", 1);
    imgPublishers[4] = it.advertise("pandora_camera4", 1);

    string serverIp;
    int serverPort;
    string calibrationFile;
    int lidarRecvPort;
    int gpsPort;
    double startAngle;
    string lidarCorrectionFile;
    int laserReturnType;
		int laserCount;
		int pclDataType;
    string pcapFile;
    
    nh.getParam("pcap_file", pcapFile);
    nh.getParam("server_ip", serverIp);
    nh.getParam("server_port", serverPort);
    nh.getParam("calbration_file", calibrationFile);
    nh.getParam("lidra_recv_port", lidarRecvPort);
    nh.getParam("gps_port", gpsPort);
    nh.getParam("start_angle", startAngle);
    nh.getParam("lidar_correction_file", lidarCorrectionFile);
    nh.getParam("laser_return_type", laserReturnType);
    nh.getParam("laser_count", laserCount);
    nh.getParam("pcldata_type", pclDataType);
    if(!pcapFile.empty())
    {
      hsdk = new HesaiLidarSDK(pcapFile, lidarCorrectionFile, laserReturnType, laserCount, pclDataType,
                      boost::bind(&HesaiLidarClient::lidarCallback, this, _1, _2));
    }
    else if(serverIp.empty())
    {
      hsdk = new HesaiLidarSDK(lidarRecvPort, gpsPort, lidarCorrectionFile,
                      boost::bind(&HesaiLidarClient::lidarCallback, this, _1, _2),
                      NULL, laserReturnType, laserCount, pclDataType);
    }

    else
    {
      hsdk = new HesaiLidarSDK(serverIp, serverPort, lidarRecvPort, gpsPort, startAngle,
                      calibrationFile,
                      lidarCorrectionFile,
                      boost::bind(&HesaiLidarClient::cameraCallback, this, _1, _2, _3),
                      boost::bind(&HesaiLidarClient::lidarCallback, this, _1, _2),
                      NULL, laserReturnType, laserCount, pclDataType);
    }
    hsdk->start();
  }

  void cameraCallback(boost::shared_ptr<cv::Mat> matp, double timestamp, int pic_id)
  {

    sensor_msgs::ImagePtr imgMsg;

    switch (pic_id)
    {
    case 0:
      imgMsg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", *matp).toImageMsg();
      break;
    case 1:
    case 2:
    case 3:
    case 4:
      imgMsg = cv_bridge::CvImage(std_msgs::Header(), "rgb8", *matp).toImageMsg();
      break;
    default:
      ROS_INFO("picid wrong in getImageToPub");
      return;
    }
    imgMsg->header.stamp = ros::Time(timestamp);
    imgPublishers[pic_id].publish(imgMsg);
  }

  void lidarCallback(boost::shared_ptr<PPointCloud> cld, double timestamp)
  {
    pcl_conversions::toPCL(ros::Time(timestamp), cld->header.stamp);
    sensor_msgs::PointCloud2 output;
    pcl::toROSMsg(*cld, output);
    lidarPublisher.publish(output);
  }


private:
  ros::Publisher lidarPublisher;
  image_transport::Publisher imgPublishers[5];
  HesaiLidarSDK* hsdk;
};


int main(int argc, char **argv)
{
  ros::init(argc, argv, "pandora_ros");
  ros::NodeHandle nh("~");
  ros::NodeHandle node;
  HesaiLidarClient pandoraClient(node, nh);

  ros::spin();
  return 0;
}