
//This function measures both temperature and humidity
void measure() {
  //Read Temperature and Humidity
  temp = dht.readTemperature();
  humid = dht.readHumidity();

  /*Serial.print("Humidity: ");
  Serial.print(humid);
  Serial.println(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println("*C");*/ 
}
