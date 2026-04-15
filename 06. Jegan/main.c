/**
 * 06. Jegan - HG Jegan Gundam Model LED Lighting Effect Controller
 *
 * Target MCU: STC8G series (SOP8 package)
 *
 * This program drives two LEDs on a Jegan Gundam model kit:
 *   - A jet/thruster LED (P3.0) with a multi-stage breathing/pulsating effect
 *   - A monitor/camera LED (P3.1) with a simple on/off state
 *
 * A push button on P3.3 triggers an external interrupt (INT1) to cycle
 * through three operating modes:
 *   Mode 0: All LEDs off
 *   Mode 1: Monitor LED on, jet LED runs breathing effect
 *   Mode 2: Both LEDs on (steady)
 *
 * Note: LEDs are active-low — setting a pin to 0 turns the LED on,
 * and setting it to 1 turns the LED off.
 */

#include <STC8.h>

/*----------------------------------------------------------------------------
 * Macro: CHECKMODE(mode, status)
 *
 * Atomically checks whether the current mode matches a given status value.
 * Uses the C comma operator: first disables external interrupt 1 (EX1=0)
 * to prevent the mode from being changed by the button ISR mid-check,
 * then evaluates and returns the boolean comparison (mode == status).
 *
 * EX1 is re-enabled inside the corresponding mode branch in main() after
 * the mode-specific actions are performed, ensuring consistent behavior.
 *---------------------------------------------------------------------------*/
#define CHECKMODE(mode,status) (EX1=0, (mode)==(status))

/*----------------------------------------------------------------------------
 * Macro: JET()
 *
 * Implements a multi-stage software-PWM breathing (pulsating) effect for the
 * jet/thruster LED. Each call to JET() executes one "tick" of the effect;
 * it is meant to be called repeatedly inside the main loop.
 *
 * How it works:
 *
 * 1. Time is divided into "stages". Each stage has its own period length
 *    defined in jet_cycle[] (e.g., 300, 200, 400 ticks). The current stage
 *    is tracked by stage_idx.
 *
 * 2. Within each stage, jet_idx counts from 0 up to (jet_cycle[stage_idx] - 1).
 *    When jet_idx reaches jet_cycle[stage_idx], it resets to 0 and jet_stat
 *    (the PWM duty-cycle value) increments by 1.
 *
 * 3. jet_stat represents the current duty-cycle width. For each full sweep
 *    of jet_idx (one PWM period), the LED is turned on for jet_stat ticks
 *    and off for the remaining ticks (or vice versa, depending on the
 *    brightening/dimming direction).
 *
 * 4. As jet_stat gradually increases from 0 to (jet_cycle[stage_idx] - 1),
 *    the duty cycle grows, making the LED appear brighter. When jet_stat
 *    wraps back to 0 (modulo jet_cycle[stage_idx]), one full brightness
 *    ramp is complete:
 *      - The 'lighter' flag toggles, reversing the PWM polarity so the
 *        LED dims instead of brightens (or vice versa).
 *      - stage_idx advances to the next stage, changing the ramp speed.
 *
 * 5. The 'lighter' flag controls the PWM output direction:
 *      - lighter == 1 (brightening): LED ON for jet_stat ticks, OFF for rest
 *      - lighter == 0 (dimming):     LED OFF for jet_stat ticks, ON for rest
 *
 * 6. The array of varying stage lengths (300,200,300,200,300,200,400,300)
 *    produces an irregular, organic-looking thruster flicker: shorter
 *    periods yield faster pulsing, and longer periods yield slower pulsing.
 *---------------------------------------------------------------------------*/
#define JET() \
{ \
    /* Check if the current PWM period has completed */ \
    if (jet_idx==jet_cycle[stage_idx]) { \
        jet_idx=0; /* Reset tick counter for new PWM period */ \
        /* Increment duty-cycle value, wrapping at the stage period length */ \
        jet_stat=(jet_stat+1)%jet_cycle[stage_idx]; \
        if (jet_stat == 0) { \
            /* Full brightness ramp complete: reverse direction */ \
            lighter=!lighter;  \
            /* Advance to next stage (wraps around all stages) */ \
            stage_idx = (stage_idx+1)%stage_cycle; \
        } \
    } \
    /* Apply PWM output to the jet LED pin based on current direction */ \
    if (lighter) { \
        /* Brightening phase: LED on for jet_stat ticks, off for the rest */ \
        if (jet_idx<jet_stat) jet = 0; /* LED on  (active-low) */ \
        else jet = 1;                  /* LED off (active-low) */ \
    } \
    else { \
        /* Dimming phase: LED off for jet_stat ticks, on for the rest */ \
        if (jet_idx<jet_stat) jet = 1; /* LED off (active-low) */ \
        else jet = 0;                  /* LED on  (active-low) */ \
    } \
    jet_idx++; /* Advance tick counter */ \
}

/*----------------------------------------------------------------------------
 * Pin Assignments (directly addressable SFR bits via 'sbit')
 *
 * P3.0 — jet:     Jet/thruster LED output (active-low)
 * P3.1 — monitor: Monitor/camera LED output (active-low)
 * P3.3 — control: Push-button input for mode switching (active-low, INT1)
 *---------------------------------------------------------------------------*/
sbit jet=P3^0; // jet
sbit monitor=P3^1; // fire
// sbit gun=P3^1; // fire  (legacy alias, unused)
sbit control=P3^3;

/*----------------------------------------------------------------------------
 * Global Variables
 *---------------------------------------------------------------------------*/

/**
 * stb — Button press stability counter used for debouncing in the ISR.
 * It confirms that the button is held low across two sample windows
 * before accepting the press as valid.
 */
unsigned int stb = 0;

/**
 * lighter — Breathing direction flag for the JET macro.
 *   1 = brightening (increasing duty cycle → LED gets brighter)
 *   0 = dimming     (increasing duty cycle → LED gets dimmer)
 * Toggled each time a full brightness ramp completes.
 */
bit lighter = 1;

/**
 * stage_idx — Index into jet_cycle[] indicating the current breathing stage.
 * Each stage has a different period length, producing varied pulsing speeds.
 * Wraps around stage_cycle (8 stages total).
 */
unsigned int stage_idx = 0;

/**
 * jet_idx — Tick counter within the current PWM period.
 * Counts from 0 to (jet_cycle[stage_idx] - 1) on each call to JET().
 * When it reaches jet_cycle[stage_idx], a new PWM period begins.
 */
unsigned int jet_idx = 0;

/**
 * jet_stat — Current PWM duty-cycle width (in ticks) within a period.
 * Incremented each time jet_idx completes a full period.
 * Determines how many ticks the LED stays on vs. off.
 */
unsigned int jet_stat = 0;

/**
 * stage_cycle — Total number of stages in the breathing effect.
 * The JET macro cycles through stages 0..(stage_cycle-1).
 */
unsigned int stage_cycle = 8;

/**
 * jet_cycle[8] — Array defining the period length (in loop ticks) for each
 * breathing stage. Shorter values produce faster pulsing; longer values
 * produce slower, more drawn-out pulsing:
 *   Stage 0: 300  (medium)
 *   Stage 1: 200  (fast)
 *   Stage 2: 300  (medium)
 *   Stage 3: 200  (fast)
 *   Stage 4: 300  (medium)
 *   Stage 5: 200  (fast)
 *   Stage 6: 400  (slow)
 *   Stage 7: 300  (medium)
 */
unsigned int jet_cycle[8] = {300,200,300,200,300,200,400,300};

/**
 * mode — Current operating mode (0, 1, or 2). Declared volatile because
 * it is modified inside the INT1 interrupt service routine and read in main().
 */
volatile int mode = 0;

/**
 * num_mode — Total number of available operating modes.
 * Used to wrap the mode counter: mode = (mode + 1) % num_mode.
 */
int num_mode = 3;

/*----------------------------------------------------------------------------
 * Function: port_init
 *
 * Configures the I/O ports, pull-up resistors, and external interrupt
 * for the STC8G microcontroller.
 *---------------------------------------------------------------------------*/
void port_init(void)
{
    /*
     * P3 I/O Mode Configuration (P3M1, P3M0):
     *   For each pin, the mode is determined by the (M1, M0) bit pair:
     *     (0, 0) = Quasi-bidirectional (weak pull-up, default 8051 mode)
     *     (0, 1) = Push-pull output
     *     (1, 0) = Input-only (high impedance)
     *     (1, 1) = Open-drain
     *
     *   P3M1 = 0xF0 = 1111_0000  →  P3.7-P3.4: M1=1
     *   P3M0 = 0x00 = 0000_0000  →  all pins:   M0=0
     *
     *   Result:
     *     P3.0-P3.3: (0,0) = Quasi-bidirectional (LED outputs + button input)
     *     P3.4-P3.7: (1,0) = High-impedance input (unused pins)
     */
    P3M1=0xf0;
    P3M0=0x00;

    /*
     * P3 Internal Pull-Up Resistor Configuration (P3PU):
     *   P3PU = 0x0C = 0000_1100
     *   Enables internal pull-up resistors on P3.2 and P3.3.
     *   P3.3 (control button) benefits from the pull-up so the pin reads
     *   high when the button is not pressed.
     */
    P3PU=0x0c;

    /*
     * Initialize all P3 pins to HIGH (0xFF).
     * Since LEDs are active-low, this turns all LEDs off at startup.
     */
    P3 = 0xFF;

    /*
     * External Interrupt 1 (INT1) Configuration:
     *   IT1 = 1: INT1 is triggered on a falling edge (button press pulls
     *            the pin from high to low).
     *   EX1 = 1: Enable external interrupt 1.
     *   EA  = 1: Enable the global interrupt flag, allowing all enabled
     *            interrupts to be serviced.
     */
    IT1 = 1;
    EX1 = 1;
    EA = 1;
}

/*----------------------------------------------------------------------------
 * Function: delay
 *
 * Software busy-wait delay. The total delay duration is proportional to 'c',
 * with each unit of 'c' executing 100 × 100 = 10,000 inner loop iterations.
 * Used for button debouncing in the ISR.
 *
 * Parameters:
 *   c — Number of outer loop iterations (delay units).
 *---------------------------------------------------------------------------*/
void delay(unsigned int c) 
{
    for (c; c>0; c--)
    {
        unsigned char a,b;
        for(b=100; b>0; b--)
        {
            for(a=100; a>0; a--) {}
        }
    }
}

/*----------------------------------------------------------------------------
 * ISR: INT1_Routine  (External Interrupt 1, vector 2)
 *
 * Triggered on the falling edge of P3.3 (control button press).
 * Implements a double-check debouncing strategy:
 *   1. Wait a short delay to let switch bouncing settle.
 *   2. If the button is still pressed (pin low), increment the stability
 *      counter 'stb'.
 *   3. Wait another short delay.
 *   4. If the button is STILL pressed AND stb > 0 (confirming a stable
 *      press across both sample windows), advance the mode.
 *   5. Reset stb for the next interrupt.
 *
 * This prevents electrical noise or switch bounce from causing
 * unintended mode changes.
 *---------------------------------------------------------------------------*/
void INT1_Routine (void) interrupt 2 {
    // delay a period to avoid jitter
    delay(10);
    if (control==0) stb += 1;
    delay(10);
    if (control==0 && stb) mode = (mode+1)%num_mode;
    stb = 0;
}

/*----------------------------------------------------------------------------
 * Function: main
 *
 * Entry point. Initializes the hardware, then enters an infinite loop that
 * drives the LEDs according to the current operating mode.
 *
 * Operating Modes:
 *   Mode 0 — OFF:
 *     Both LEDs are turned off (pins set high, active-low).
 *
 *   Mode 1 — JET BREATHING:
 *     The monitor LED is turned on (steady).
 *     The jet LED runs the multi-stage breathing/pulsating effect via JET().
 *
 *   Mode 2 — ALL ON (steady):
 *     Both the monitor and jet LEDs are turned on at full brightness.
 *
 * In each mode branch, EX1 is re-enabled (set to 1) after CHECKMODE
 * disabled it, so the button interrupt remains active for mode switching.
 *---------------------------------------------------------------------------*/
void main() {
    port_init();

    while(1)
    {
        if (CHECKMODE(mode, 0)) {
            /* Mode 0 — OFF: disable both LEDs */
            EX1 = 1;         /* Re-enable button interrupt */
            monitor = 1;     /* Monitor LED off (active-low) */
            jet = 1;         /* Jet LED off (active-low) */
        } else if (CHECKMODE(mode, 1)) {
            /* Mode 1 — JET BREATHING: monitor on, jet runs breathing effect */
            EX1 = 1;         /* Re-enable button interrupt */
            monitor = 0;     /* Monitor LED on (active-low) */
            JET()            /* Execute one tick of the jet breathing effect */
        } else if (CHECKMODE(mode, 2)) {
            /* Mode 2 — ALL ON: both LEDs on at full brightness */
            EX1 = 1;         /* Re-enable button interrupt */
            monitor = 0;     /* Monitor LED on (active-low) */
            jet = 0;         /* Jet LED on (active-low) */
        }
        
        
    }
}
