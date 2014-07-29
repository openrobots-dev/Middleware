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
}

#endif /* _PID_HPP_ */
