#include <LiquidCrystal.h>
#include "Button.h"
#include "LowPower.h"

// LCD
const int lcd_rs = 9;
const int lcd_en = 8;
const int lcd_d4 = 7;
const int lcd_d5 = 6;
const int lcd_d6 = 5;
const int lcd_d7 = 4;
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

// OUTPUT
const int pump = 2;
bool pumpState = false;

int i = 0;

/// Stores the state of the LCD backlight.
class Backlight {
  private:
    /// ms to keep the LCD backlight on past the last action.
    static const unsigned int MS_ON = 30000;

    /// Pin of the backlight.
    const unsigned int _pin;

    /// Current state of the LCD backlight.
    bool _on;

    /// How long the LCD backlight has been on since last action.
    unsigned int _msCount;

  public:
    Backlight() = delete;

    Backlight(unsigned int pin) : _msCount(0), _on(false), _pin(pin) {
    }

    void begin() {
        pinMode(_pin, OUTPUT);
        turnOn();
    }

    void callback() {
        if (_on && (++_msCount > MS_ON)) {
            _on = false;
            digitalWrite(_pin, LOW);
        }
    }

    const decltype(_on) isOn() const {
        return _on;
    }

    void turnOn() {
        if (!_on) {
            digitalWrite(_pin, HIGH);
            _on = true;
        }
        _msCount = 0;
    }
};

class Display {
  protected:
    unsigned int _screen = 0;

  public:
    Display* nextDisplay = this;

    virtual void actionDown(){};
    virtual void actionUp(){};

    void clear() const {
        lcd.clear();
    }

    virtual void display(){};

    Display& next() {
        ++_screen;
        clear();
        if (_screen >= numScreens()) {
            _screen = 0;
            return *nextDisplay;
        }
        return *this;
    };

    virtual unsigned int numScreens() {
        return 1;
    };
};

class Display1 : public Display {
  public:
    void display() {
        lcd.setCursor(0, 0);
        lcd.write("Display1");
        lcd.setCursor(0, 1);
        lcd.print(_screen);
    };

    unsigned int numScreens() {
        return 3;
    };
};

class Display2 : public Display {
  public:
    void display() {
        lcd.setCursor(0, 0);
        lcd.write("Display2");
        lcd.setCursor(0, 1);
        lcd.print(_screen);
    };

    unsigned int numScreens() {
        return 2;
    };
};

class Time : public Display {
  private:
    enum STATES { IDLE, SET_MINUTES, SET_HOURS };

    static const unsigned long MS_DAY = 86400000ul;
    static const unsigned long MS_HOUR = 86400000ul / 24ul;
    static const unsigned long MS_MINUTE = 86400000ul / (24ul * 60ul);
    static const unsigned long MS_SECOND = 86400000ul / (24ul * 60ul * 60ul);
    unsigned long _msCount = 0;

    void dec(const unsigned long amount) {
        if (_msCount < amount) {
            _msCount += MS_DAY;
        }
        _msCount -= amount;
    }

    void inc(const unsigned long amount) {
        _msCount += amount;
        if (_msCount > MS_DAY) {
            _msCount -= MS_DAY;
        }
    }

  public:
    void actionUp() {
        if (_screen == SET_MINUTES) {
            inc(MS_MINUTE);
        } else if (_screen == SET_HOURS) {
            inc(MS_HOUR);
        }
    }

    void actionDown() {
        if (_screen == SET_MINUTES) {
            dec(MS_MINUTE);
        } else if (_screen == SET_HOURS) {
            dec(MS_HOUR);
        }
    }

    void callback() {
        ++_msCount;
        _msCount *= (_msCount < MS_DAY);
    }

    const unsigned int seconds() const {
        return (_msCount / MS_SECOND) % 60;
    }

    const unsigned int minutes() const {
        return (_msCount / MS_MINUTE) % 60;
    }

    const unsigned int hours() const {
        return (_msCount / MS_HOUR) % 24;
    }

    void show(const int row, const int col, const int blank = -1) const {
        char buff[8];
        sprintf(buff, "%02d:%02d:%02d", hours(), minutes(), seconds());
        if (blank >= 0) {
            buff[blank] = ' ';
            buff[blank + 1] = ' ';
        }
        lcd.setCursor(col, row);
        lcd.write(buff);
    }

    void display() {
        auto blank = -1;
        if (2 * (_msCount % MS_SECOND) < MS_SECOND) {
            if (_screen == SET_MINUTES) {
                blank = 3;
            } else if (_screen == SET_HOURS) {
                blank = 0;
            }
        }
        show(0, 0, blank);
    };

    unsigned int numScreens() {
        return 3;
    };
};

// class Irrigator {
//   private:
//   unsigned int
//   public:

// };

// BUTTONS
Button btnBlack(10);
Button btnGreen(11);
Button btnOrange(12);

Backlight backlight(3);

Display1 d1;
Display2 d2;
Time time;
Display* currentDisplay;

// Timer 0 Interrupt is called once a millisecond
SIGNAL(TIMER0_COMPA_vect) {
    btnBlack.callback();
    btnGreen.callback();
    btnOrange.callback();
    backlight.callback();
    time.callback();
}

void setup() {
    lcd.begin(16, 2);
    backlight.begin();
    btnBlack.begin();
    btnGreen.begin();
    btnOrange.begin();

    // IO
    pinMode(pump, OUTPUT);

    // INTERUPTS
    // Timer0 is already used for millis()
    // set up  "TIMER0_COMPA_vect" interrupt function
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);

    // OTHER
    digitalWrite(pump, pumpState);
    currentDisplay = &time;
    d1.nextDisplay = &d2;
    d2.nextDisplay = &time;
    time.nextDisplay = &d1;
}

void loop() {
    if (!backlight.isOn() && (btnBlack.isSinglePressed() || btnGreen.isSinglePressed() ||
                              btnOrange.isSinglePressed())) {
        backlight.turnOn();
    }

    if (btnBlack.isSinglePressed()) {
        currentDisplay = &(currentDisplay->next());
    }

    if (btnOrange.isHeldRepeat() || btnOrange.isSinglePressed()) {
        currentDisplay->actionUp();
    }

    if (btnGreen.isHeldRepeat() || btnGreen.isSinglePressed()) {
        currentDisplay->actionDown();
    }

    currentDisplay->display();

    delay(50);
}