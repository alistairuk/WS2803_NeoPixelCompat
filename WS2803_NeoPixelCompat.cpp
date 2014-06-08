#include <SPI.h>
#include <WS2803_NeoPixelCompat.h>

// Example to control WS2803-based RGB LED strips using an Adafruit NeoPixel interface
// Hacked togeather by Alistair MacDonald from some cool Adafruit code - MIT license
/*****************************************************************************/

// Constructor for use with hardware SPI (specific clock/data pins):
    WS2803_NeoPixelCompat::WS2803_NeoPixelCompat(uint16_t n) {
  updateLength(n);
  setPins();
}

// Constructor for use with arbitrary clock/data pins:
WS2803_NeoPixelCompat::WS2803_NeoPixelCompat(uint16_t n, uint8_t dpin, uint8_t cpin) {
  updateLength(n);
  setPins(dpin, cpin);
}


// via Michael Vogt/neophob: empty constructor is used when strand length
// isn't known at compile-time; situations where program config might be
// read from internal flash memory or an SD card, or arrive via serial
// command.  If using this constructor, MUST follow up with updateLength()
// and updatePins() to establish the strand length and output pins!
WS2803_NeoPixelCompat::WS2803_NeoPixelCompat(void) {
  begun   = false;
  numLEDs = 0;
  numBytes = 0;
  pixels  = NULL;
  setPins(); // Must assume hardware SPI until pins are set
}

// Activate hard/soft SPI as appropriate:
void WS2803_NeoPixelCompat::begin(void) {
  if(hardwareSPI == true) {
    startSPI();
  } else {
    pinMode(datapin, OUTPUT);
    pinMode(clkpin , OUTPUT);
  }
  begun = true;
}

// Change pin assignments post-constructor, switching to hardware SPI:
void WS2803_NeoPixelCompat::setPins(void) {
  hardwareSPI = true;
  datapin     = clkpin = 0;
  // If begin() was previously invoked, init the SPI hardware now:
  if(begun == true) startSPI();
  // Otherwise, SPI is NOT initted until begin() is explicitly called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!
}

// Change pin assignments post-constructor, using arbitrary pins:
void WS2803_NeoPixelCompat::setPins(uint8_t dpin, uint8_t cpin) {

  if(begun == true) { // If begin() was previously invoked...
    // If previously using hardware SPI, turn that off:
    if(hardwareSPI == true) SPI.end();
    // Regardless, now enable output on 'soft' SPI pins:
    pinMode(dpin, OUTPUT);
    pinMode(cpin, OUTPUT);
  } // Otherwise, pins are not set to outputs until begin() is called.

  // Note: any prior clock/data pin directions are left as-is and are
  // NOT restored as inputs!

  hardwareSPI = false;
  datapin     = dpin;
  clkpin      = cpin;
  clkport     = portOutputRegister(digitalPinToPort(cpin));
  clkpinmask  = digitalPinToBitMask(cpin);
  dataport    = portOutputRegister(digitalPinToPort(dpin));
  datapinmask = digitalPinToBitMask(dpin);
}

// Enable SPI hardware and set up protocol details:
void WS2803_NeoPixelCompat::startSPI(void) {
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV8);  // 2 MHz
    // WS2803 datasheet recommends max SPI clock of 2 MHz, and 50 Ohm
    // resistors on SPI lines for impedance matching.  In practice and
    // at short distances, 2 MHz seemed to work reliably enough without
    // resistors, and 4 MHz was possible with a 220 Ohm resistor on the
    // SPI clock line only.  Your mileage may vary.  Experiment!
    // SPI.setClockDivider(SPI_CLOCK_DIV4);  // 4 MHz
}

// Returns pointer to pixels[] array.  Pixel data is stored in device-
// native format and is not translated here.  Application will need to be
// aware whether pixels are RGB vs. GRB and handle colors appropriately.
uint8_t *WS2803_NeoPixelCompat::getPixels(void) const {
  return pixels;
}

uint16_t WS2803_NeoPixelCompat::numPixels(void) const {
  return numLEDs;
}

// Adjust output brightness; 0=darkest (off), 255=brightest.  This does
// NOT immediately affect what's currently displayed on the LEDs.  The
// next call to show() will refresh the LEDs at this level.  However,
// this process is potentially "lossy," especially when increasing
// brightness.  The tight timing in the WS2811/WS2812 code means there
// aren't enough free cycles to perform this scaling on the fly as data
// is issued.  So we make a pass through the existing color data in RAM
// and scale it (subsequent graphics commands also work at this
// brightness level).  If there's a significant step up in brightness,
// the limited number of steps (quantization) in the old data will be
// quite visible in the re-scaled version.  For a non-destructive
// change, you'll need to re-render the full strip data.  C'est la vie.
void WS2803_NeoPixelCompat::setBrightness(uint8_t b) {
  // Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
  uint8_t newBrightness = b + 1;
  if(newBrightness != brightness) { // Compare against prior value
    // Brightness has changed -- re-scale existing data in RAM
    uint8_t  c,
            *ptr           = pixels,
             oldBrightness = brightness - 1; // De-wrap old brightness value
    uint16_t scale;
    if(oldBrightness == 0) scale = 0; // Avoid /0
    else if(b == 255) scale = 65535 / oldBrightness;
    else scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
    for(uint16_t i=0; i<numBytes; i++) {
      c      = *ptr;
      *ptr++ = (c * scale) >> 8;
    }
    brightness = newBrightness;
  }
}

// Change strand length (see notes with empty constructor, above):
void WS2803_NeoPixelCompat::updateLength(uint16_t n) {
  if(pixels != NULL) free(pixels); // Free existing data (if any)
  // Allocate new data -- note: ALL PIXELS ARE CLEARED
  numLEDs = n;
  numBytes = n*3;
  pixels = (uint8_t *)calloc(numBytes, 1);
  // 'begun' state does not change -- pins retain prior modes
}

void WS2803_NeoPixelCompat::show(void) {
  uint16_t i; // 3 bytes per LED
  uint8_t  bit;

     if(hardwareSPI) {	
       for(i=0; i<numBytes; i++) {
         SPDR = pixels[i];
         while(!(SPSR & (1<<SPIF)));
       }
     } else {
       for(i=0; i<numBytes; i++ ) {
         for(bit=0x80; bit; bit >>= 1) {
           if(pixels[(i)] & bit) *dataport |=  datapinmask;
           else                *dataport &= ~datapinmask;
           *clkport |=  clkpinmask;
           *clkport &= ~clkpinmask;
         }
       }
  }

  delay(1); // Data is latched by holding clock pin low for 1 millisecond
}

// Set pixel color from separate R,G,B components:
void WS2803_NeoPixelCompat::setPixelColor(
 uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if(n < numLEDs) {
    if(brightness) { // See notes in setBrightness()
      r = (r * brightness) >> 8;
      g = (g * brightness) >> 8;
      b = (b * brightness) >> 8;
    }
    uint8_t *p = &pixels[n * 3];
    p[0] = r;
    p[1] = g;
    p[2] = b;
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
void WS2803_NeoPixelCompat::setPixelColor(uint16_t n, uint32_t c) {
    uint8_t
      r = (uint8_t)(c >> 16),
      g = (uint8_t)(c >>  8),
      b = (uint8_t)c;
  setPixelColor(n, r, g, b);
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t WS2803_NeoPixelCompat::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}


// Query color from previously-set pixel (returns packed 8-bit mono color value)
uint32_t WS2803_NeoPixelCompat::getPixelColor(uint16_t n) const {

  if(n < numLEDs) {
    uint8_t *p = &pixels[n * 3];
    return ((uint32_t)p[0] << 16) |
           ((uint32_t)p[1] <<  8) |
            (uint32_t)p[2];
  }

  return 0; // Pixel # is out of bounds

}

