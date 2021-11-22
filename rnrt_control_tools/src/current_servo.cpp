#include "rnrt_control_tools/current_servo.h"

CurrentServo::CurrentServo()
    : m_current_last{0.0}
{
}

bool CurrentServo::init(const ros::NodeHandle &nh, std::string &joint_name)
{
    double u_max, gear_ratio, eff;

    const ros::NodeHandle nh_inv(nh, "inverter/" + joint_name);
    const ros::NodeHandle nh_gear(nh, "gear/" + joint_name);

    if (!nh_inv.getParam("umax", u_max))
    {
        ROS_ERROR("No U_max specified for motor.  Namespace: %s", nh_inv.getNamespace().c_str());
        return false;
    }

    if (!nh_gear.getParam("ratio", gear_ratio))
    {
        ROS_ERROR("No gear_ratio specified for motor.  Namespace: %s", nh_gear.getNamespace().c_str());
        return false;
    }

    if (!nh_gear.getParam("eff", eff))
    {
        ROS_ERROR("No efficiency specified for motor.  Namespace: %s", nh_gear.getNamespace().c_str());
        return false;
    }

    init(u_max, gear_ratio, nh, joint_name, eff);

    return true;
}

void CurrentServo::init(double &u_max,
                        double &gear_ratio,
                        const ros::NodeHandle &nh,
                        std::string &joint_name,
                        const double &efficiency)
{
    if (u_max <= 0.0)
    {
        ROS_WARN_STREAM("U_max is out of range");
        return;
    }

    if (gear_ratio <= 0.0)
    {
        ROS_WARN_STREAM("Gear_ratio is out of range");
        return;
    }

    m_u_max = u_max;
    m_gear_ratio = gear_ratio;
    m_efficiency = efficiency;

    if (efficiency > 1.0 || efficiency <= 0.0)
    {
        ROS_WARN_STREAM("Efficiency is out of range, setting to 1.0");
        m_efficiency = 1.0;
    }

    ros::NodeHandle motor_nh(nh, "motor_parameters/" + joint_name);
    ros::NodeHandle pid_nh(nh, "current_loop_gains/" + joint_name);

    initPid(pid_nh);
    initMotor(motor_nh);
}

void CurrentServo::initPid(const ros::NodeHandle &n)
{
    m_pid_current = std::make_shared<control_toolbox::Pid>();
    if (!m_pid_current->init(n))
    {
        ROS_WARN_STREAM("Failed to initialize PID gains from ROS parameter server.");
        return;
    }
}

void CurrentServo::initMotor(const ros::NodeHandle &n)
{
    m_motor = std::make_shared<PmMotor>();
    if (!m_motor->init(n))
    {
        ROS_WARN_STREAM("Failed to initialize motor from ROS parameter server.");
        return;
    }
}

double CurrentServo::getEffortResponse(const double &effort_command,
                                       const double &velocity,
                                       ros::Duration period)
{
    auto current_command{((effort_command / m_gear_ratio) / m_motor->getKm()) - m_current_last};

    auto voltage_command{m_pid_current->computeCommand(current_command, period)};

    voltage_command = std::clamp(voltage_command, -m_u_max, m_u_max);

    m_current_last = m_motor->getCurrentResponse(voltage_command,
                                                 velocity * m_gear_ratio,
                                                 period.toNSec(),
                                                 SolverType::RUNGEKUTTA);

    ROS_INFO_STREAM("current_command = " << current_command);
    ROS_INFO_STREAM("effort_command = " << effort_command);
    ROS_INFO_STREAM("voltage_command = " << voltage_command);
    ROS_INFO_STREAM("m_current_last = " << m_current_last);
    ROS_INFO_STREAM("velocity = " << velocity);
    return m_current_last * m_motor->getKm() * m_gear_ratio * m_efficiency;
}