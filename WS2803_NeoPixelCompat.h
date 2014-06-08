#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

class WS2803_NeoPixelCompat {

 public:

  WS2803_NeoPixelCompat(uint16_t n, uint8_t dpin, uint8_t cpin); // Configurable pins
  WS2803_NeoPixelCompat(uint16_t n); // Use SPI hardware; specific pins only
  WS2803_NeoPixelCompat(void); // Empty constructor; init pins/strand length later
  void
    begin(void),
    show(void),
    setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
    setPixelColor(uint16_t n, uint32_t c),
    setBrightness(uint8_t),
    setPins(uint8_t dpin, uint8_t cpin), // Change pins, configurable
    setPins(void), // Change pins, hardware SPI
    updateLength(uint16_t n); // Change strand length
  uint8_t
   *getPixels(void) const;
  uint16_t
    numPixels(void) const;
  static uint32_t
    Color(uint8_t r, uint8_t g, uint8_t b);
  uint32_t
    getPixelColor(uint16_t n) const;

 private:
  uint16_t
    numLEDs,       // Number of RGB LEDs in strip
    numBytes;      // Size of 'pixels' buffer below
  uint8_t
    *pixels, // Holds color values for each LED (3 bytes each)
    brightness,
    clkpin    , datapin,     // Clock & data pin numbers
    clkpinmask, datapinmask; // Clock & data PORT bitmasks
  volatile uint8_t
    *clkport  , *dataport;   // Clock & data PORT registers
  void
    startSPI(void);
  boolean
    hardwareSPI, // If 'true', using hardware SPI
    begun;       // If 'true', begin() method was previously invoked
};
