WS2803 NeoPixel compatable library
==================================

Arduino library for controlling WS2803 based RGB LED strips using SPI. It appears to also work with the WS2801.

After downloading, rename folder to 'WS2803_NeoPixelCompat' and install in Arduino Libraries folder. Restart Arduino IDE, then open File->Sketchbook->Library->WS2803_NeoPixelCompat->strandtest sketch.

A bit yay to Adafruit for the great work they have done of the two libraries that were combined to make this happen. The originals and more are available at https://github.com/adafruit/ .

Updates to this library are available at https://github.com/alistairuk .

To convert your sketch from using NeoPixel just replace...

#include <Adafruit_NeoPixel.h>

...with...

#include <WS2803_NeoPixelCompat.h>
#include <SPI.h>

...and replace...

Adafruit_NeoPixel strip = Adafruit_NeoPixel({LED_Count},{Data_Pin}, {Params});

...with...

WS2803_NeoPixelCompat strip = WS2803_NeoPixelCompat({LED_Count},{Data_Pin},{Clock_Pin});
