/*  
 *  Copyrights:
 *  Erik Komendera.  August 2010.
 *
 *  This file is part of tagmapper_cu.
 *
 *  tagmapper_cu is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  tagmapper_cu is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with client_server_cu. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  If you require a different license, contact Michael Otte at
 *  erik.komendera@colorado.edu
 *
 *
 *  This adds new Pseudolite tags to the Pseudolite XML map file.
 */
#include "ros/ros.h"
#include <std_msgs/Int8.h>
#include "stargazer_cu/Pose2DTagged.h"
#include "geometry_msgs/PoseStamped.h"
#include "irobot_create_rustic/Speeds.h"

#include <iostream>
#include <fstream>

#define SAMPLES 50 // Takes SAMPLES samples
#define DROP_SAMPLES 10 // And drops the first DROP_SAMPLES and averages the rest.
#define PSEUDO_FILE_LINE_WIDTH 256
#define PSEUDO_FILE_LINES 100

ros::Publisher kill_pub; // Tells Stargazer to end so it can restart and load the new XML file.
ros::Subscriber pose_sub; // Uses the pose to determine position of Pseudolite tag.
ros::Subscriber marker_sub; // Use the relative position to tag to modify the XML file.
ros::Subscriber speeds_sub; // Determines when the bot is stopped so that sampling can commence.

const char * pseudo_file = "../../stargazer_cu/pseudolites.xml"; // If the relative paths change, change this.
char pseudo_text[PSEUDO_FILE_LINES][PSEUDO_FILE_LINE_WIDTH]; // This acts as storage of the XML file.

int poseSampleCount; // Keeps track of how many samples have been stored.
int markerSampleCount;
int lastTagLine; // For use in placing the next tag line in the XML file.
unsigned int tagIDs[PSEUDO_FILE_LINES];
int tagCount; // Index to tagIDs
unsigned int currentTag;
bool isStopped;
bool restart;

struct Pose
{
    float x;
    float y;
    float z;
    
    float qw;
    float qx;
    float qy;
    float qz; 
    
    float alpha; // rotation in 2D plane
    
    float cos_alpha;
    float sin_alpha;
};

struct Marker
{
    float x;
    float y;
    float theta;
    int ID;
};

Pose poseSamples[SAMPLES];
Marker markerSamples[SAMPLES];

// Returns true if this is an old tag, false if this is a new one.
bool isTagInFile(unsigned int tag)
{
  for (int i = 0; i < tagCount; i++) {
    if (tag == tagIDs[i]) return true;
  }
  return false;
}

// Restarts the sampling process if something occurs.
void newTagInterrupt()
{
  currentTag = 0;
  poseSampleCount = 0;
  markerSampleCount = 0;
}

// Sample a new position.
void pose_callback(const geometry_msgs::PoseStamped::ConstPtr& msg)
{
  // Only sample a new position if the robot is not moving and there is room for more samples.
  if (isStopped && poseSampleCount < SAMPLES) {
    poseSamples[poseSampleCount].x = msg->pose.position.x;
    poseSamples[poseSampleCount].y = msg->pose.position.y;
    poseSamples[poseSampleCount].z = msg->pose.position.z;
    poseSamples[poseSampleCount].qw = msg->pose.orientation.w;
    poseSamples[poseSampleCount].qx = msg->pose.orientation.x;
    poseSamples[poseSampleCount].qy = msg->pose.orientation.y;
    poseSamples[poseSampleCount].qz = msg->pose.orientation.z;

    // normalize
    float magnitude = sqrt(poseSamples[poseSampleCount].qw*poseSamples[poseSampleCount].qw + poseSamples[poseSampleCount].qx*poseSamples[poseSampleCount].qx + poseSamples[poseSampleCount].qy*poseSamples[poseSampleCount].qy + poseSamples[poseSampleCount].qz*poseSamples[poseSampleCount].qz);
    if(poseSamples[poseSampleCount].qw < 0)
      magnitude *= -1; 
    poseSamples[poseSampleCount].qw /= magnitude;
    poseSamples[poseSampleCount].qx /= magnitude;
    poseSamples[poseSampleCount].qy /= magnitude;
    poseSamples[poseSampleCount].qz /= magnitude;
  
    if(poseSamples[poseSampleCount].qw > 1)
      poseSamples[poseSampleCount].qw = 1;
  
    float qw = poseSamples[poseSampleCount].qw;
    float qx = poseSamples[poseSampleCount].qx;
    float qy = poseSamples[poseSampleCount].qy;
    float qz = poseSamples[poseSampleCount].qz; 
  
    poseSamples[poseSampleCount].cos_alpha = qw*qw + qx*qx - qy*qy - qz*qz;
    poseSamples[poseSampleCount].sin_alpha = 2*qw*qz + 2*qx*qy; 
    poseSamples[poseSampleCount].alpha = atan2(poseSamples[poseSampleCount].sin_alpha, poseSamples[poseSampleCount].cos_alpha);

    poseSampleCount++;
    //ROS_INFO("Poses: %d\n", poseSampleCount);
  }
}

// Sample a new marker.
void marker_callback(const stargazer_cu::Pose2DTagged::ConstPtr& msg)
{
  // Get marker data if we are continuing with a tag that's not in the file.  If the tag changes, we have to start all over again.
  // Also, only get data if stopped.
  if (isStopped && markerSampleCount < SAMPLES) {
    if (markerSampleCount == 0) {
      if (isTagInFile(msg->tag)) return;
      else currentTag = msg->tag;
    } else {
      if (msg->tag != currentTag) {
        newTagInterrupt();
        return;
      }
    }
    markerSamples[markerSampleCount].x = msg->x;
    markerSamples[markerSampleCount].y = msg->y;
    markerSamples[markerSampleCount].theta = msg->theta;
    markerSamples[markerSampleCount].ID = msg->tag;
    markerSampleCount++;
    //ROS_INFO("Markers: %d\n", markerSampleCount);
  }
}

// Determine if the robot is stopped.  When the robot stops, restart the sampling process.  If the robot is moving, don't sample at all.
void speeds_callback(const irobot_create_rustic::Speeds::ConstPtr& msg)
{
  //ROS_INFO("Forward = %f, Rotate = %f\n", msg->forward, msg->rotate);
  if ((fabs(msg->forward) > 0.001) || (fabs(msg->rotate) > 0.001)) {
    isStopped = false;
    //ROS_INFO("Is Stopped = %d\n", isStopped);
  } else {
    if (!isStopped) {
      newTagInterrupt();
    }
    isStopped = true;
    //ROS_INFO("Is Stopped = %d\n", isStopped);
  }
}

// When we have modified the XML file, we need to tell Stargazer to restart.
void publish_kill()
{
  ROS_INFO("Now kill");
  std_msgs::Int8 msg;
  msg.data = 1;
  kill_pub.publish(msg);
}

// When all the samples have been retrieved, compute the average and add to the file.
void saveTagInXml()
{
  float pth, px, py;

  Pose avPose;
  Marker avMarker;
  memset(&avPose, 0, sizeof(Pose));
  memset(&avMarker, 0, sizeof(Marker));

  // Only average the final SAMPLES-DROP_SAMPLES samples.  This is done to let data readings settle.
  for (int i = DROP_SAMPLES; i < SAMPLES; i++) {
    avPose.x += poseSamples[i].x;
    avPose.y += poseSamples[i].y;
    avPose.z += poseSamples[i].z;
    avPose.alpha += poseSamples[i].alpha;
    avMarker.x += markerSamples[i].x;
    avMarker.y += markerSamples[i].y;
    avMarker.theta += markerSamples[i].theta;
    avMarker.ID = markerSamples[i].ID;
  }
  avPose.x /= (SAMPLES-DROP_SAMPLES);
  avPose.y /= (SAMPLES-DROP_SAMPLES);
  avPose.z /= (SAMPLES-DROP_SAMPLES);
  avPose.alpha /= (SAMPLES-DROP_SAMPLES);
  avMarker.x /= (SAMPLES-DROP_SAMPLES);
  avMarker.y /= (SAMPLES-DROP_SAMPLES);
  avMarker.theta /= (SAMPLES-DROP_SAMPLES);

  // -2PI < Angle < 2PI
  pth = -avPose.alpha - avMarker.theta;
  while ( pth < 0 ) pth += 2*M_PI;
  while ( pth >= 2*M_PI ) pth -= 2*M_PI;

  px = avPose.x - avMarker.x*cos(pth) - avMarker.y*sin(pth);
  py = avPose.y - avMarker.y*cos(pth) + avMarker.x*sin(pth);
  ROS_INFO("TagMapper is at: X = %f, Y = %f, TH = %f, ID = %d", px, py, pth, avMarker.ID);

  // Insert the new tag line into the character array, and write it to the XML file.
  std::ofstream psout;
  psout.open(pseudo_file, std::ofstream::out);
  for (int i = 0; i < lastTagLine; i++) {
    psout << pseudo_text[i] << std::endl;
  }
  psout << "    <PseudoLite id=\"" << avMarker.ID << "\" x=\"" << px << "\" y=\"" << py << "\" angle=\"" << pth << "\"></PseudoLite>\n";
  psout << "</PseudoLiteMap>\n";
  psout.close();
}

// Main loop.
int main(int argc, char **argv)
{
  ros::init(argc, argv, "new_tagmapper_cu");
  ros::NodeHandle n;
  kill_pub = n.advertise<std_msgs::Int8>("/cu/killsg", 1000);
  pose_sub = n.subscribe("/cu/pose_cu", 1, pose_callback);
  marker_sub = n.subscribe("/cu/stargazer_marker_cu", 1, marker_callback);
  speeds_sub = n.subscribe("/speeds_bus", 1, speeds_callback);
  ros::Rate loop_rate(1000);

  isStopped = false;
  restart = false;
  poseSampleCount = 0;
  markerSampleCount = 0;
  lastTagLine = 0;
  memset(tagIDs, 0, PSEUDO_FILE_LINES*sizeof(int));
  tagCount = 0;
  currentTag = 0;

  // Open the file, read it in, extract the tag IDs, and determine the place to insert the new tag.
  int ifline = 0;
  std::ifstream psin;
  psin.open(pseudo_file, std::ifstream::in);
  if (!psin.good()) {
    return 1;
  }
  while (!psin.eof()) {
    psin.getline(pseudo_text[ifline], PSEUDO_FILE_LINE_WIDTH);
    ifline++;
  }
  psin.close();
  for (int i = 0; i < PSEUDO_FILE_LINES; i++) {
    if (strstr(pseudo_text[i], "/PseudoLiteMap")) {
      lastTagLine = i;
      break;
    }
  }
  for (int i = 0; i < lastTagLine; i++) {
    if (strstr(pseudo_text[i], "PseudoLite id")) {
      sscanf(pseudo_text[i], "    <PseudoLite id=\"%d\"", &tagIDs[tagCount]);
      tagCount++;
    }
  }

  // Run the main loop.  If the new tag samples have all been retrieved and the robot is stopped, save the tag, kill Stargazer, and then shut down.
  int count = 0;
  while (ros::ok())
  {
    if (poseSampleCount == SAMPLES && markerSampleCount == SAMPLES && isStopped) {
      saveTagInXml();
      publish_kill();
      break;
    }
    ros::spinOnce();
    loop_rate.sleep();
    ++count;
  }

  kill_pub.shutdown();
  pose_sub.shutdown();
  marker_sub.shutdown();
  speeds_sub.shutdown();

  return 0;
}

