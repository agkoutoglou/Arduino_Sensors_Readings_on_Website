Arduino_Sensors_Readings_on_Website
===================================

A project with Arduino, reading values from different sensors and project them into a website.

Takes the values from:

1) A TEMT6000 Anbient light sensor

2) A MR003-007.1 Analog Reflectance Sensor

3) A BMP-180 Barometric Pressure Sensor

4) A SEN-10239 (HH10D) Humidity Sensor

The output is transmitted via Ethernet cable using a shield with the W5100 ethernet controller, at the 192.168.1.177 address.

Plans are ahead to include a DS18B20 Temperature Sensor (digital), as well to direct the output to a SD card, tracking time also, of the reading from the initialization of the board.