/*
 * Copyright (c) 2018 Pilz GmbH & Co. KG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>
#include <math.h>
#include <boost/core/demangle.hpp>
#include <ros/console.h>
#include <moveit_msgs/Constraints.h>
#include <moveit/robot_trajectory/robot_trajectory.h>
#include <sensor_msgs/JointState.h>
#include <moveit/planning_interface/planning_interface.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/convert.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <moveit/kinematic_constraints/utils.h>
#include <moveit_msgs/MoveGroupAction.h>
#include "pilz_trajectory_generation/limits_container.h"
#include "pilz_trajectory_generation/trajectory_blend_response.h"
#include "pilz_trajectory_generation/trajectory_blend_request.h"
#include "pilz_trajectory_generation/trajectory_generator.h"
#include "pilz_msgs/MotionBlendRequestList.h"

namespace testutils
{

const std::string JOINT_NAME_PREFIX("prbt_joint_");

static constexpr int32_t DEFAULT_SERVICE_TIMEOUT(10);

/**
   * @brief Convert degree to rad.
   */
inline static constexpr double deg2Rad(double angle)
{
  return (angle/180.0)*M_PI;
}

inline std::string getJointName(size_t joint_number, std::string joint_prefix)
{
  return joint_prefix + std::to_string(joint_number);
}

/**
   * @brief Create limits for tests to avoid the need to get the from the parameter server
   * @param joint_number
   * @return
   */
inline pilz::JointLimitsContainer createFakeLimits(size_t joint_number,
                                                   std::string joint_prefix = testutils::JOINT_NAME_PREFIX)
{
  pilz::JointLimitsContainer container;

  for(size_t i = 0; i < joint_number; ++i)
  {
    pilz_extensions::JointLimit limit;
    limit.has_position_limits = true;
    limit.max_position = 2.967;
    limit.min_position = -2.967;
    limit.has_velocity_limits = true;
    limit.max_velocity = 1;
    limit.has_acceleration_limits = true;
    limit.max_acceleration = 0.5;
    limit.has_deceleration_limits = true;
    limit.max_deceleration = -1;

    container.addLimit(getJointName(i+1, joint_prefix), limit);
  }

  return container;
}


inline std::string demangel(char const * name)
{
  return boost::core::demangle(name);
}

//********************************************
// Motion plan requests
//********************************************

inline sensor_msgs::JointState generateJointState(std::vector<double> pos,
                                                  std::vector<double> vel,
                                                  std::string joint_prefix = testutils::JOINT_NAME_PREFIX)
{
  sensor_msgs::JointState state;
  auto posit = pos.begin();
  size_t i = 0;

  while(posit != pos.end())
  {
    state.name.push_back(testutils::getJointName(i+1, joint_prefix));
    state.position.push_back(*posit);

    i++;
    posit++;
  }
  for(auto vel_ : vel)
  {
    state.velocity.push_back(vel_);
  }
  return state;
}

inline sensor_msgs::JointState generateJointState(std::vector<double> pos,
                                                  std::string joint_prefix = testutils::JOINT_NAME_PREFIX)
{
  return generateJointState(pos, std::vector<double>(), joint_prefix);
}

inline moveit_msgs::Constraints generateJointConstraint(const std::vector<double>& pos_list,
                                                        std::string joint_prefix = testutils::JOINT_NAME_PREFIX)
{
  moveit_msgs::Constraints gc;

  auto pos_it = pos_list.begin();

  for(size_t i = 0; i < pos_list.size(); ++i)
  {
    moveit_msgs::JointConstraint jc;
    jc.joint_name = testutils::getJointName(i+1, joint_prefix);
    jc.position = *pos_it;
    gc.joint_constraints.push_back(jc);

    ++pos_it;
  }

  return gc;
}

/**
 * @brief Determines the goal position from the given request.
 * TRUE if successful, FALSE otherwise.
 */
bool getExpectedGoalPose(const moveit::core::RobotModelConstPtr &robot_model,
                         const planning_interface::MotionPlanRequest &req,
                         std::string& link_name,
                         Eigen::Affine3d &goal_pose_expect);

/**
 * @brief create a dummy motion plan request with zero start state
 * No goal constraint is given.
 * @param robot_model
 * @param planning_group
 * @param req
 */
void createDummyRequest(const moveit::core::RobotModelConstPtr &robot_model,
                        const std::string& planning_group,
                        planning_interface::MotionPlanRequest& req);

void createPTPRequest(const std::string& planning_group,
                      const moveit::core::RobotState &start_state,
                      const moveit::core::RobotState &goal_state,
                      planning_interface::MotionPlanRequest& req);

/**
 * @brief check if the goal given in joint space is reached
 * Only the last point in the trajectory is veryfied.
 * @param trajectory: generated trajectory
 * @param goal: goal in joint space
 * @param joint_position_tolerance
 * @param joint_velocity_tolerance
 * @return true if satisfied
 */
bool isGoalReached(const trajectory_msgs::JointTrajectory& trajectory,
                   const std::vector<moveit_msgs::JointConstraint>& goal,
                   const double joint_position_tolerance,
                   const double joint_velocity_tolerance = 1.0e-6);

/**
 * @brief check if the goal given in cartesian space is reached
 * Only the last point in the trajectory is veryfied.
 * @param robot_model
 * @param trajectory
 * @param req
 * @param matrix_norm_tolerance: used to compare the transformation matrix
 * @param joint_velocity_tolerance
 * @return
 */
bool isGoalReached(const robot_model::RobotModelConstPtr& robot_model,
                   const trajectory_msgs::JointTrajectory& trajectory,
                   const planning_interface::MotionPlanRequest &req,
                   const double matrix_norm_tolerance);

/**
 * @brief Check that given trajectory is straight line.
 */
bool checkCartesianLinearity(const robot_model::RobotModelConstPtr& robot_model,
                             const trajectory_msgs::JointTrajectory& trajectory,
                             const planning_interface::MotionPlanRequest &req,
                             const double translation_norm_tolerance,
                             const double rot_axis_norm_tolerance,
                             const double rot_angle_tolerance=10e-5);

/**
 * @brief check SLERP. The orientation should rotate around the same axis linearly.
 * @param start_pose
 * @param goal_pose
 * @param wp_pose
 * @param rot_axis_norm_tolerance
 * @return
 */
bool checkSLERP(const Eigen::Affine3d& start_pose,
                const Eigen::Affine3d& goal_pose,
                const Eigen::Affine3d& wp_pose,
                const double rot_axis_norm_tolerance,
                const double rot_angle_tolerance=10e-5);

/**
 * @brief get the waypoint index from time from start
 * @param trajectory
 * @param time_from_start
 * @return
 */
inline int getWayPointIndex(const robot_trajectory::RobotTrajectoryPtr& trajectory,
                            const double time_from_start)
{
  int index_before, index_after;
  double blend;
  trajectory->findWayPointIndicesForDurationAfterStart(time_from_start,index_before,index_after,blend);
  return blend > 0.5 ? index_after : index_before;
}

/**
 * @brief check joint trajectory of consistency, position, velocity and acceleration limits
 * @param trajectory
 * @param joint_limits
 * @return
 */
bool checkJointTrajectory(const trajectory_msgs::JointTrajectory& trajectory,
                          const pilz::JointLimitsContainer& joint_limits);

/**
 * @brief check if the sizes of the joint position/veloicty/acceleration are correct
 * @param trajectory
 * @return
 */
bool isTrajectoryConsistent(const trajectory_msgs::JointTrajectory& trajectory);

/**
 * @brief is Position Bounded
 * @param trajectory
 * @param joint_limits
 * @return
 */
bool isPositionBounded(const trajectory_msgs::JointTrajectory& trajectory,
                       const pilz::JointLimitsContainer& joint_limits);

/**
 * @brief is Velocity Bounded
 * @param trajectory
 * @param joint_limits
 * @return
 */
bool isVelocityBounded(const trajectory_msgs::JointTrajectory& trajectory,
                       const pilz::JointLimitsContainer& joint_limits);

/**
 * @brief is Acceleration Bounded
 * @param trajectory
 * @param joint_limits
 * @return
 */
bool isAccelerationBounded(const trajectory_msgs::JointTrajectory& trajectory,
                           const pilz::JointLimitsContainer& joint_limits);

/**
 * @brief compute the tcp pose from joint values
 * @param robot_model
 * @param link_name
 * @param joint_values
 * @param pose
 * @param joint_prefix Prefix of the joint names
 * @return false if forward kinematics failed
 */
bool toTCPPose(const moveit::core::RobotModelConstPtr &robot_model,
               const std::string &link_name,
               const std::vector<double> &joint_values,
               geometry_msgs::Pose &pose,
               std::string joint_prefix = testutils::JOINT_NAME_PREFIX);

/**
 * @brief computeLinkFK
 * @param robot_model
 * @param link_name
 * @param joint_state
 * @param pose
 * @return
 */
bool computeLinkFK(const robot_model::RobotModelConstPtr& robot_model,
                   const std::string &link_name,
                   const std::map<std::string, double> &joint_state,
                   Eigen::Affine3d &pose);



/**
 * @brief checkOriginalTrajectoryAfterBlending
 * @param blend_res
 * @param traj_1_res
 * @param traj_2_res
 * @param time_tolerance
 * @return
 */
bool checkOriginalTrajectoryAfterBlending(const pilz::TrajectoryBlendRequest& req,
                                          const pilz::TrajectoryBlendResponse& res,
                                          const double time_tolerance);

/**
 * @brief check the blending result, if the joint space continuity is fulfilled
 * @param res: trajectory blending response, contains three trajectories.
 * Between these three trajectories should be continuous.
 * @return true if joint position/velocity is continuous. joint acceleration can have jumps.
 */
bool checkBlendingJointSpaceContinuity(const pilz::TrajectoryBlendResponse& res,
                                       double joint_velocity_tolerance,
                                       double joint_accleration_tolerance);

bool checkBlendingCartSpaceContinuity(const pilz::TrajectoryBlendRequest& req,
                                      const pilz::TrajectoryBlendResponse& res,
                                      const pilz::LimitsContainer& planner_limits);

/**
 * @brief Checks if all points of the blending trajectory lie within the blending radius.
 */
bool checkThatPointsInRadius(const std::string &link_name,
                             const double& r,
                             Eigen::Affine3d &circ_pose,
                             const pilz::TrajectoryBlendResponse& res);

/**
 * @brief compute Cartesian velocity
 * @param pose_1
 * @param pose_2
 * @param duration
 * @param v
 * @param w
 */
void computeCartVelocity(const Eigen::Affine3d pose_1,
                         const Eigen::Affine3d pose_2,
                         double duration,
                         Eigen::Vector3d& v,
                         Eigen::Vector3d& w);

void createFakeCartTraj(const robot_trajectory::RobotTrajectoryPtr& traj,
                        const std::string &link_name,
                        moveit_msgs::RobotTrajectory& fake_traj);

/**
 * @brief Returns an initial joint state and two poses which
 * can be used to perform a Lin-Lin movement.
 *
 * @param frame_id
 * @param initialJointState As cartesian position: (0.3, 0, 0.65, 0, 0, 0)
 * @param p1 (0.05, 0, 0.65, 0, 0, 0)
 * @param p2 (0.05, 0.4, 0.65, 0, 0, 0)
 */
void getLinLinPosesWithoutOriChange(const std::string& frame_id,
                                    sensor_msgs::JointState& initialJointState,
                                    geometry_msgs::PoseStamped& p1,
                                    geometry_msgs::PoseStamped& p2);

void getOriChange(Eigen::Matrix3d& ori1, Eigen::Matrix3d& ori2);

inline geometry_msgs::Quaternion fromEuler(double a, double b, double c)
{
  tf2::Vector3 qvz(0.,0.,1.);
  tf2::Vector3 qvy(0.,1.,0.);
  tf2::Quaternion q1 ( qvz, a);

  q1 = q1 * tf2::Quaternion( qvy, b );
  q1 = q1 * tf2::Quaternion( qvz, c );


  geometry_msgs::Quaternion msg;
  tf2::convert(q1, msg);
  return msg;
}

/**
 * @brief Returns an initial joint state and two poses which
 * can be used to perform a Lin-Lin movement.
 *
 * @param frame_id
 * @param initialJointState As cartesian position: (0.3, 0, 0.65, 0, 0, 0)
 * @param p1 (0.05, 0, 0.65, 0, 0, 0)
 * @param p2 (0.05, 0.4, 0.65, 0, 0, 0)
 */
void getLinLinPosesWithoutOriChange(const std::string& frame_id,
                                    sensor_msgs::JointState& initialJointState,
                                    geometry_msgs::PoseStamped& p1,
                                    geometry_msgs::PoseStamped& p2);


/**
 * @brief Test data for blending, which contains three joint position vectors of three robot state.
 */
struct blend_test_data
{
  std::vector<double> start_position;
  std::vector<double> mid_position;
  std::vector<double> end_position;
};

/**
 * @brief fetch test datasets from parameter server
 * @param nh
 * @return
 */
bool getBlendTestData(const ros::NodeHandle &nh,
                      const size_t& dataset_num,
                      const std::string& name_prefix,
                      std::vector<blend_test_data>& datasets);


/**
 * @brief check the blending result of lin-lin
 * @param lin_res_1
 * @param lin_res_2
 * @param blend_req
 * @param blend_res
 * @param checkAcceleration
 */
bool checkBlendResult(const pilz::TrajectoryBlendRequest& blend_req,
                      const pilz::TrajectoryBlendResponse& blend_res,
                      const pilz::LimitsContainer& limits,
                      double joint_velocity_tolerance,
                      double joint_acceleration_tolerance,
                      double cartesian_velocity_tolerance,
                      double cartesian_angular_velocity_tolerance);

/**
 * @brief generate two LIN trajectories from test data set
 * @param data: contains joint poisitons of start/mid/end states
 * @param sampling_time_1: sampling time for first LIN
 * @param sampling_time_2: sampling time for second LIN
 * @param[out] res_lin_1: result of the first LIN motion planning
 * @param[out] res_lin_2: result of the second LIN motion planning
 * @param[out] dis_lin_1: translational distance of the first LIN
 * @param[out] dis_lin_2: translational distance of the second LIN
 * @return true if succeed
 */
bool generateTrajFromBlendTestData(const moveit::core::RobotModelConstPtr &robot_model,
                                   const std::shared_ptr<pilz::TrajectoryGenerator> tg,
                                   const std::string &group_name,
                                   const std::string &link_name,
                                   const blend_test_data& data,
                                   const double &sampling_time_1,
                                   const double &sampling_time_2,
                                   planning_interface::MotionPlanResponse& res_lin_1,
                                   planning_interface::MotionPlanResponse& res_lin_2,
                                   double &dis_lin_1, double &dis_lin_2);

void generateRequestMsgFromBlendTestData(const moveit::core::RobotModelConstPtr &robot_model,
                                         const blend_test_data& data,
                                         const std::string &planner_id,
                                         const std::string &group_name,
                                         const std::string &link_name,
                                         pilz_msgs::MotionBlendRequestList& req_list);

void checkRobotModel(const moveit::core::RobotModelConstPtr &robot_model,
                     const std::string& group_name,
                     const std::string& link_name);
}

#endif // TEST_UTILS_H
