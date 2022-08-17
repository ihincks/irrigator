#include "Arduino.h"

/// Stores the state of a single button.
class Button {
  private:
    /// Possible button states.
    enum STATES { IDLE, DEBOUNCE, PRESSED, HELD };

    /// Debounce time.
    static const unsigned int MS_DEBOUNCE = 50;

    /// How long the button must be held from press time to enter `HELD` state.
    static const unsigned int MS_HOLD = 1000;

    /// Duration between repeat events when the button is in the `HELD` state.
    static const unsigned int MS_HOLD_REPEAT = 200;

    /// Digital pin number of the button.
    const unsigned int _pin;

    /// Counter used for changing states.
    unsigned int _msCount;

    /// Current state of the button.
    unsigned int _state;

    /// Whether the current single press has been used and is therefore blocked.
    bool _singleBlocked;

    /// Whether we are in a blocking period while the button is being `HELD`.
    bool _holdBlocked;

  public:
    Button() = delete;

    Button(unsigned int pin) :
        _pin(pin), _msCount(0), _state(IDLE), _singleBlocked(false), _holdBlocked(false) {
    }

    void begin() {
        pinMode(_pin, INPUT_PULLUP);
    }

    void callback() {
        int read = digitalRead(_pin);
        if (read == 1) {
            _state = IDLE;
        } else {
            switch (_state) {
            case IDLE:
                _state = DEBOUNCE;
                _msCount = 0;
                break;
            case DEBOUNCE:
                if (++_msCount > MS_DEBOUNCE) {
                    _singleBlocked = false;
                    _holdBlocked = false;
                    _state = PRESSED;
                }
                break;
            case PRESSED:
                if (++_msCount > MS_HOLD) {
                    _state = HELD;
                    _msCount = 0;
                }
                break;
            case HELD:
                if (++_msCount > MS_HOLD_REPEAT) {
                    _holdBlocked = false;
                    _msCount = 0;
                }
                break;
            }
        }
    }

    const bool isHeld() const {
        return _state == HELD;
    }

    const bool isHeldRepeat() {
        if (!_holdBlocked && isHeld()) {
            _holdBlocked = true;
            return true;
        }
        return false;
    }

    const bool isPressed() const {
        return ((_state == PRESSED) || (_state == HELD));
    }

    const bool isSinglePressed() {
        if (!_singleBlocked && isPressed()) {
            _singleBlocked = true;
            return true;
        }
        return false;
    }
};