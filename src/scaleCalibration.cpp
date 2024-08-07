#include <Arduino.h>

#include "HX711.h"


void ScaleCal(HX711 scale){
  if (scale.is_ready()) {
    scale.set_scale();    
    Serial.println("Tare... remove any weights from the scale.");
    delay(5000);
    scale.tare();

    Serial.println("Tare done...");

    Serial.println("Place a known weight on the scale...");
    
    delay(5000);
 
    long reading = scale.get_units(10);
    Serial.println("Result: ");
    Serial.println(reading);


  }
      else {
    Serial.println("HX711 not found.");

  }
  delay(1000);
}