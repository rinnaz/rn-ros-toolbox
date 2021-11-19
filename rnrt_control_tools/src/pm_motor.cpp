#include "rnrt_control_tools/pm_motor.h"

PmMotor::PmMotor()
    : m_l{1.0}, m_r{1.0}, m_te{m_l / m_r}, m_km{1.0}, m_ke{1.0},
      m_pole_pairs{1}
{
    initStateSpaceModel();
}

PmMotor::PmMotor(const double &ind,
                 const double &res,
                 const double &km,
                 const uint64_t &pole_pairs)
{
    init(ind, res, km, pole_pairs);
}

void PmMotor::init(const double &ind,
                   const double &res,
                   const double &km,
                   const uint64_t &pole_pairs)
{
    if (ind <= 0.0 || res <= 0.0 || km <= 0.0 || pole_pairs <= 0)
    {
        throw std::range_error("Negative input is not allowed");
    }
    m_l = ind;
    m_r = res;
    m_km = km;
    m_ke = km;
    m_te = m_l / m_r;
    m_pole_pairs = pole_pairs;

    initStateSpaceModel();
}

void PmMotor::initStateSpaceModel()
{
    TransferFcn tfcn{{1.0 / m_r}, {m_te, 1.0}};
    m_state_space_model_ptr = std::make_shared<StateSpaceModel>(tfcn);
}

double PmMotor::getCurrentResponse(const double &input_voltage,
                                   const double &current_velocity,
                                   const uint64_t &dt,
                                   const SolverType solver)
{
    return m_state_space_model_ptr->getResponse(input_voltage - current_velocity * m_ke,
                                                dt,
                                                solver);
}

double PmMotor::getTorqueResponse(const double &input_voltage,
                                  const double &current_velocity,
                                  const uint64_t &dt,
                                  const SolverType solver)
{
    return m_km * getCurrentResponse(input_voltage, current_velocity, dt, solver);
}
