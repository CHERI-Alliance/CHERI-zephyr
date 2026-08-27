#include <controller/motors.h>
#include <controller/imu.h>
#include <controller/input.h>
#include <controller/pid.h>

bool motor_set(enum motor motor, uint32_t throttle) { (void)motor; (void)throttle; return true; }
bool motor_configure(enum motor motor, struct motor_config *cfg) { (void)motor; (void)cfg; return true; }
enum motor motor_from_string(char *str) { (void)str; return MOTOR_LAST; }

int imu_get_data(struct imu_data *data) { (void)data; return 0; }

float input_get_channel_value(enum input_channel ch) { (void)ch; return 0.0f; }
uint32_t input_get_channel_raw_value(enum input_channel ch) { (void)ch; return 0; }
bool input_configure_channel(enum input_channel ch, struct input_channel_config *cfg) { (void)ch; (void)cfg; return true; }
bool input_set_calibration(enum input_channel ch, struct input_channel_pwm_calib *calib) { (void)ch; (void)calib; return true; }
enum input_channel input_channel_from_string(char *str) { (void)str; return CHANNEL_FUNC_LAST; }
bool input_init(void) { return true; }
bool input_start(void) { return true; }
bool input_stop(void) { return true; }

void pid_init(struct pid *pid, bool reset_state) { (void)pid; (void)reset_state; }
void pid_reset(struct pid *pid) { (void)pid; }
float pid_process(struct pid *pid, float in) { (void)pid; (void)in; return 0.0f; }