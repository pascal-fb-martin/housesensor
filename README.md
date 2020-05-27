# HouseSensor - A Small Web Server to Access Physical Sensors

# Overview

This project provides a small web server that reports the latest values from local sensors. This project depends on [echttp](https://github.com/pascal-fb-martin/echttp) and [HousePortal](https://github.com/pascal-fb-martin/houseportal).

The original goal was to read temperature sensors from 1-wire DS18x20 devices.

# Installation

* Clone this GitHub repository.
* make
* sudo make install

# Configuration

The list of sensors to scan is defined in file /etc/house/sensor.config.

There are two types of entry: options and sensors.

An option entry start with the keyword OPTION:
```
'OPTIONS' name value
```
A sensor line start with a driver name and is made of 4 to 5 items:
```
driver device location name [unit]
```
The only driver supported at this time is 'w1' (the Linux interface for the 1-Wire network).

For 1-Wire devices, the device is the 1-Wire ID of the sensor, e.g. 28-01162bdbf5ee or 10-000800c49886.

The location is an arbitrary user name, which is used to organize the sensors in groups. The name is the name of the sensor as reported to the outside.

A unit can be specified to accomodate sensors that have no intrinsic unit.

The program also records all measurements. The recording is accumulated each day in /dev/shm/housesensor.log (i.e. in RAM) and copied each day to /var/lib/house as sensor-YYYY-MM-DD.log, where YYYY, MM and DD represents the current day.

