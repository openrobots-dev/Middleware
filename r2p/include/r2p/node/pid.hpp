#ifndef _PID_HPP_
#define _PID_HPP_

#include <float.h>

class PID {
private:
	float _i;
	float _d;
	float _setpoint;
	float _k;
	float _ki;
	float _kd;
	float _min;
	float _max;

public:
	PID(void);
	void config(float k, float ti, float td, float ts, float min, float max);
	void set(float setpoint);
	void reset(void);
	float get_setpoint(void);
	float update(float measure);
};


PID::PID(void) {
	_i = 0;
	_d = 0;
	_setpoint = 0;
	_k = 0;
	_ki = 0;
	_kd = 0;
	_min = -FLT_MAX;
	_max = FLT_MAX;

}


void PID::config(float k, float ti, float td, float ts, float min = -FLT_MAX, float max = FLT_MAX) {

	_k = k;
	_ki = (ti == 0) ? 0 : k * (ts / ti);
	_kd = k * (td / ts);
	_min = min;
	_max = max;
}


void PID::set(float setpoint) {

	_setpoint = setpoint;
}

void PID::reset(void) {

	_i = 0;
	_d = 0;
	_setpoint = 0;
}

float PID::get_setpoint(void) {

	return _setpoint;
}

float PID::update(float measure) {
	float error;
	float output;

	/* calculate error */
	error = _setpoint - measure;

	/* proportional term */
	output = (_k * error);

	/* integral term */
	_i += error;
	output += _ki * _i;

	/* derivative term */
	output += _kd * (error - _d);
	_d = error;

	/* saturation filter */
	if (output > _max) {
		output = _max;
		/* anti windup: cancel error integration */
		_i -= error;
	} else if (output < _min) {
		output = _min;
		/* anti windup: cancel error integration */
		_i -= error;
	}

	return output;
//
//	/* saturation filter */
//	if (output > max_) {
//		/* conditional anti-windup: integrate only if error and setpoint have different sign */
//		if ((error >= 0) && (setpoint_ <= 0)) {
//			i_ += error * dt;
//			output += (ki_ * i_);
//		}
//		output = max_;
//	} else if (output < min_) {
//		/* conditional anti-windup: integrate only if error and setpoint have different sign */
//		if ((error >= 0) && (setpoint_ <= 0)) {
//			i_ += error * dt;
//			output += (ki_ * i_);
//		}
//		output = min_;
//	} else {
//		i_ += error * dt;
//		output += (ki_ * i_);
//	}
//
//	/* derivative term */
//	output += (kd_ * (error - d_));
//	d_ = error;
//
//	return output;
}

#endif /* _PID_HPP_ */
