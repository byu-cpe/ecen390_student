#ifndef TRIGGER_H_
#define TRIGGER_H_

#include <stdbool.h>
#include <stdint.h>

// The trigger state machine debounces both the press and the release of
// a trigger. When transitioning from a released to a pressed state,
// a "pressed" function is called. It is specified as a callback function.
// When transitioning from a pressed to a released state, a "released"
// function is called. It also is specified as a callback function.

// Initialize the trigger state machine.
// period: Specify the period in milliseconds between calls to trigger_tick().
// gpio_num: GPIO pin number of the trigger.
// Return zero if successful, or non-zero otherwise.
int32_t trigger_init(uint32_t period, int32_t gpio_num);

// Standard tick function.
// This function is safe to call from an ISR context.
void trigger_tick(void);

// Register a trigger "pressed" callback function.
// If the trigger_tick() function is called from an ISR context,
// the "pressed" function also needs to be ISR safe.
// pressed: pointer to function called when the trigger is first pressed.
void trigger_register_pressed(void (*pressed)(void));

// Register a trigger "released" callback function.
// If the trigger_tick() function is called from an ISR context,
// the "released" function also needs to be ISR safe.
// released: pointer to function called when the trigger is first released.
void trigger_register_released(void (*released)(void));

// Enable or disable the trigger operation.
// If disabled, the trigger is ignored.
// enable: if true, enable the trigger, otherwise disable.
void trigger_operation(bool enable);

#endif // TRIGGER_H_
