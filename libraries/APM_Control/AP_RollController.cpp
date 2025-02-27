/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//	Code by Jon Challinger
//  Modified by Paul Riseborough
//

#include <AP_HAL/AP_HAL.h>
#include "AP_RollController.h"
#include <AP_AHRS/AP_AHRS.h>

extern const AP_HAL::HAL& hal;

const AP_Param::GroupInfo AP_RollController::var_info[] = {
    // index 0 reserved for old TCONST

    // index 1 to 3 reserved for old PID values

    // @Param: _AGL_RMAX
    // @DisplayName: Maximum Roll Rate
    // @Description: This sets the maximum roll rate that the attitude controller will demand (degrees/sec) in angle stabilized modes (all but MANUAL and ACRO). Setting it to zero disables this limit.
    // @Range: 0 180
    // @Units: deg/s
    // @Increment: 1
    // @User: Advanced
    AP_GROUPINFO("_AGL_RMAX",   4, AP_RollController, gains.rmax_pos,       0),

    // index 5, 6 reserved for old IMAX, FF

    // @Param: _RATE_P
    // @DisplayName: Roll axis rate controller P gain
    // @Description: Roll axis rate controller P gain. Converts the difference between desired roll rate and actual roll rate into a control surface angle
    // @Range: 0.08 0.35
    // @Increment: 0.005
    // @User: Standard

    // @Param: _RATE_I
    // @DisplayName: Roll axis rate controller I gain
    // @Description: Roll axis rate controller I gain. Corrects long-term difference in desired roll rate vs actual roll rate
    // @Range: 0.01 0.6
    // @Increment: 0.01
    // @User: Standard

    // @Param: _RATE_IMAX
    // @DisplayName: Roll axis rate controller I gain maximum
    // @Description: Roll axis rate controller I gain maximum. Constrains the maximum control surface angle that the I gain will output
    // @Range: 0 1
    // @Increment: 0.01
    // @User: Standard

    // @Param: _RATE_D
    // @DisplayName: Roll axis rate controller D gain
    // @Description: Roll axis rate controller D gain. Compensates for short-term change in desired roll rate vs actual roll rate
    // @Range: 0.001 0.03
    // @Increment: 0.001
    // @User: Standard

    // @Param: _RATE_FF
    // @DisplayName: Roll axis rate controller feed forward
    // @Description: Roll axis rate controller feed forward
    // @Range: 0 3.0
    // @Increment: 0.001
    // @User: Standard

    // @Param: _RATE_FLTT
    // @DisplayName: Roll axis rate controller target frequency in Hz
    // @Description: Roll axis rate controller target frequency in Hz
    // @Range: 2 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _RATE_FLTE
    // @DisplayName: Roll axis rate controller error frequency in Hz
    // @Description: Roll axis rate controller error frequency in Hz
    // @Range: 2 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _RATE_FLTD
    // @DisplayName: Roll axis rate controller derivative frequency in Hz
    // @Description: Roll axis rate controller derivative frequency in Hz
    // @Range: 0 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _RATE_SMAX
    // @DisplayName: Roll slew rate limit
    // @Description: Sets an upper limit on the slew rate produced by the combined P and D gains. If the amplitude of the control action produced by the rate feedback exceeds this value, then the D+P gain is reduced to respect the limit. This limits the amplitude of high frequency oscillations caused by an excessive gain. The limit should be set to no more than 25% of the actuators maximum slew rate to allow for load effects. Note: The gain will not be reduced to less than 10% of the nominal value. A value of zero will disable this feature.
    // @Range: 0 200
    // @Increment: 0.5
    // @User: Advanced

    AP_SUBGROUPINFO(rate_pid, "_RATE_", 9, AP_RollController, AC_PID),

    // @Param: _AGL_P
    // @DisplayName: Roll axis angle controller P gain
    // @Description: Roll axis angle controller P gain. Converts the difference between desired roll angle and actual roll angle into a roll rate
    // @Range: 0.08 0.35
    // @Increment: 0.005
    // @User: Standard

    // @Param: _AGL_I
    // @DisplayName: Roll axis angle controller I gain
    // @Description: Roll axis angle controller I gain. Corrects long-term difference in desired roll angle vs actual roll angle
    // @Range: 0.01 0.6
    // @Increment: 0.01
    // @User: Standard

    // @Param: _AGL_IMAX
    // @DisplayName: Roll axis angle controller I gain maximum
    // @Description: Roll axis angle controller I gain maximum. Constrains the maximum roll rate that the I gain will output
    // @Range: 0 1
    // @Increment: 0.01
    // @User: Standard

    // @Param: _AGL_D
    // @DisplayName: Roll axis angle controller D gain
    // @Description: Roll axis angle controller D gain. Compensates for short-term change in desired roll angle vs actual roll angle
    // @Range: 0.001 0.03
    // @Increment: 0.001
    // @User: Standard

    // @Param: _AGL_FF
    // @DisplayName: Roll axis angle controller feed forward (not used)
    // @Description: Roll axis angle controller feed forward (not used)
    // @Range: 0 3.0
    // @Increment: 0.001
    // @User: Standard

    // @Param: _AGL_FLTT
    // @DisplayName: Roll axis angle controller target frequency in Hz
    // @Description: Roll axis angle controller target frequency in Hz
    // @Range: 2 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _AGL_FLTE
    // @DisplayName: Roll axis angle controller error frequency in Hz
    // @Description: Roll axis angle controller error frequency in Hz
    // @Range: 2 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _AGL_FLTD
    // @DisplayName: Roll axis angle controller derivative frequency in Hz
    // @Description: Roll axis angle controller derivative frequency in Hz
    // @Range: 0 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: _AGL_SMAX
    // @DisplayName: Requested roll rate slew rate limit
    // @Description: Sets an upper limit on the slew rate produced by the combined P and D gains. If the amplitude of the control action produced by the rate feedback exceeds this value, then the D+P gain is reduced to respect the limit. This limits the amplitude of high frequency oscillations caused by an excessive gain. The limit should be set to no more than 25% of the actuators maximum slew rate to allow for load effects. Note: The gain will not be reduced to less than 10% of the nominal value. A value of zero will disable this feature.
    // @Range: 0 200
    // @Increment: 0.5
    // @User: Advanced

    AP_SUBGROUPINFO(angle_pid, "_AGL_", 10, AP_RollController, AC_PID),

    AP_GROUPEND
};

// constructor
AP_RollController::AP_RollController(const AP_Vehicle::FixedWing &parms)
    : aparm(parms)
{
    AP_Param::setup_object_defaults(this, var_info);
    rate_pid.set_slew_limit_scale(45);
}


/*
  AC_PID based rate controller
*/
float AP_RollController::_get_rate_out(float desired_rate, float scaler, bool disable_integrator, bool ground_mode, float aspeed, float eas2tas, float rate_x)
{
    const float dt = AP::scheduler().get_loop_period_s();

    bool limit_I = fabsf(_last_out) >= 45;
    float old_I = rate_pid.get_i();

    rate_pid.set_dt(dt);

    bool underspeed = aspeed <= float(aparm.airspeed_min);
    if (underspeed) {
        limit_I = true;
    }

    // the P and I elements are scaled by sq(scaler). To use an
    // unmodified AC_PID object we scale the inputs and calculate FF separately
    //
    // note that we run AC_PID in radians so that the normal scaling
    // range for IMAX in AC_PID applies (usually an IMAX value less than 1.0)
    rate_pid.update_all(radians(desired_rate) * scaler * scaler, rate_x * scaler * scaler, limit_I);

    if (underspeed) {
        // when underspeed we lock the integrator
        rate_pid.set_integrator(old_I);
    }

    // FF should be scaled by scaler/eas2tas, but since we have scaled
    // the AC_PID target above by scaler*scaler we need to instead
    // divide by scaler*eas2tas to get the right scaling
    const float ff = degrees(rate_pid.get_ff() / (scaler * eas2tas));

    if (disable_integrator) {
        rate_pid.reset_I();
    }

    // convert AC_PID info object to same scale as old controller
    _pid_info = rate_pid.get_pid_info();
    auto &pinfo = _pid_info;

    const float deg_scale = degrees(1);
    pinfo.FF = ff;
    pinfo.P *= deg_scale;
    pinfo.I *= deg_scale;
    pinfo.D *= deg_scale;

    // fix the logged target and actual values to not have the scalers applied
    pinfo.target = desired_rate;
    pinfo.actual = degrees(rate_x);

    // sum components
    float out = pinfo.FF + pinfo.P + pinfo.I + pinfo.D;
    if (ground_mode) {
        // when on ground suppress D term to prevent oscillations
        out -= pinfo.D + 0.5*pinfo.P;
    }

    // remember the last output to trigger the I limit
    _last_out = out;

    if (autotune != nullptr && autotune->running && aspeed > aparm.airspeed_min) {
        // let autotune have a go at the values
        autotune->update(pinfo, scaler, angle_err_deg);
    }

    // output is scaled to notional centidegrees of deflection
    return constrain_float(out * 100, -4500, 4500);
}

/*
 Function returns an equivalent elevator deflection in centi-degrees in the range from -4500 to 4500
 A positive demand is up
 Inputs are:
 1) desired roll rate in degrees/sec
 2) control gain scaler = scaling_speed / aspeed
*/
float AP_RollController::get_rate_out(float desired_rate, float scaler)
{
    GSO_AHRS_Data ahrs_data;
    _get_gso_ahrs_data(ahrs_data);

    return _get_rate_out(desired_rate, scaler, false, false, ahrs_data.aspeed, ahrs_data.eas2tas, ahrs_data.rate_x);
}

void AP_RollController::_get_gso_ahrs_data(GSO_AHRS_Data &ahrs_data)
{

    bool have_aspeed;
    {
        AP_AHRS &_ahrs = AP::ahrs();
        WITH_SEMAPHORE(_ahrs.get_semaphore());
        ahrs_data.eas2tas = _ahrs.get_EAS2TAS();
        ahrs_data.rate_x = _ahrs.get_gyro().x;
        have_aspeed = AP::ahrs().airspeed_estimate(ahrs_data.aspeed);
        ahrs_data.roll_sensor = _ahrs.roll_sensor;
    }

    if (!have_aspeed) {
        ahrs_data.aspeed = 0;
    }

}

/*
 Function returns an equivalent aileron deflection in centi-degrees in the range from -4500 to 4500
 A positive demand is up
 Inputs are:
 1) demanded bank angle in centi-degrees
 2) control gain scaler = scaling_speed / aspeed
 3) boolean which is true when stabilise mode is active
 4) minimum FBW airspeed (metres/sec)
*/
float AP_RollController::get_servo_out_using_angle_target(int32_t target_angle, float scaler, bool disable_integrator, bool ground_mode)
{
    const float dt = AP::scheduler().get_loop_period_s();
    angle_pid.set_dt(dt);

    GSO_AHRS_Data ahrs_data;
    _get_gso_ahrs_data(ahrs_data);

    const float target_angle_deg = target_angle * 0.01f;
    const float measured_angle_deg = ahrs_data.roll_sensor * 0.01f;
    angle_err_deg = target_angle_deg - measured_angle_deg;

    if (angle_err_deg > 2.0f) {
        angle_pid.relax_integrator(0, 0.1f);
    }

    angle_pid.update_all(target_angle_deg, measured_angle_deg, false);

    if (disable_integrator) {
        angle_pid.reset_I();
    }

    _angle_pid_info = angle_pid.get_pid_info();
    auto &pinfo = _angle_pid_info;

    float desired_rate = pinfo.P + pinfo.I + pinfo.D;

    return _get_servo_out(desired_rate, scaler, disable_integrator, ground_mode, ahrs_data);
}

float AP_RollController::get_servo_out_using_angle_error(int32_t angle_err, int32_t target_angle, float scaler, bool disable_integrator, bool ground_mode)
{
    const float dt = AP::scheduler().get_loop_period_s();
    angle_pid.set_dt(dt);

    angle_err_deg = angle_err * 0.01f;

    if (angle_err_deg > 2.0f) {
        angle_pid.relax_integrator(0, 0.1f);
    }

    angle_pid.update_error(angle_err_deg, false);

    if (disable_integrator) {
        angle_pid.reset_I();
    }

    _angle_pid_info = angle_pid.get_pid_info();
    auto &pinfo = _angle_pid_info;

    GSO_AHRS_Data ahrs_data;
    _get_gso_ahrs_data(ahrs_data);

    if (target_angle == 0) {
        pinfo.target = 0;
        pinfo.actual = 0;
    } else {
        const float actual_angle = ahrs_data.roll_sensor;
        pinfo.target = target_angle * 0.01f;
        pinfo.actual = actual_angle * 0.01f;
    }

    float desired_rate = pinfo.P + pinfo.I + pinfo.D;

    return _get_servo_out(desired_rate, scaler, disable_integrator, ground_mode, ahrs_data);
}

float AP_RollController::_get_servo_out(float desired_rate, float scaler, bool disable_integrator, bool ground_mode, const GSO_AHRS_Data &ahrs_data)
{
    // Limit the demanded roll rate
    if (gains.rmax_pos && desired_rate < -gains.rmax_pos) {
        desired_rate = - gains.rmax_pos;
    } else if (gains.rmax_pos && desired_rate > gains.rmax_pos) {
        desired_rate = gains.rmax_pos;
    }

    return _get_rate_out(desired_rate, scaler, disable_integrator, ground_mode, ahrs_data.aspeed, ahrs_data.eas2tas, ahrs_data.rate_x);
}

void AP_RollController::reset_I()
{
    _pid_info.I = 0;
    rate_pid.reset_I();
    _angle_pid_info.I = 0;
    angle_pid.reset_I();
}

/*
  convert from old to new PIDs
  this is a temporary conversion function during development
 */
void AP_RollController::convert_pid()
{
    AP_Float &angle_kP = angle_pid.kP();
    if (angle_kP.configured()) {
        return;
    }

    float old_tconst;
    bool have_old = AP_Param::get_param_by_index(this, 0, AP_PARAM_FLOAT, &old_tconst);
    if (!have_old) {
        // tconst wasn't set
        return;
    }

    const float angle_kp = 1 / old_tconst;
    angle_pid.kP().set_and_save_ifchanged(angle_kp);
}

/*
  start an autotune
 */
void AP_RollController::autotune_start(void)
{
    if (autotune == nullptr) {
        angle_i_backup = angle_pid.kI();
        angle_fltt_backup = angle_pid.filt_T_hz();
        gains.tau = tau();
        autotune = new AP_AutoTune(gains, &kP(), AP_AutoTune::AUTOTUNE_ROLL, aparm, rate_pid);
        if (autotune == nullptr) {
            if (!failed_autotune_alloc) {
                GCS_SEND_TEXT(MAV_SEVERITY_ERROR, "AutoTune: failed roll allocation");
            }
            failed_autotune_alloc = true;
        }
        angle_pid.kI(0);
        angle_pid.kD().set_and_save_ifchanged(0);
        angle_pid.filt_T_hz(0);
    }
    if (autotune != nullptr) {
        autotune->start();
    }
}

/*
  restore autotune gains
 */
void AP_RollController::autotune_restore(void)
{
    if (autotune != nullptr) {
        autotune->stop();
        angle_pid.kI(angle_i_backup);
        angle_pid.filt_T_hz(angle_fltt_backup);
    }
}
