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

A unit can be specified to accommodate sensors that have no intrinsic unit.

The program also records all measurements. The recording is accumulated each day in /dev/shm/housesensor.csv (i.e. in RAM) and moved at the end of the day to /var/lib/house/sensor as YYYY-MM-DD.csv, where YYYY, MM and DD represents the day of the recording.

If the HouseSensor service is stopped, /dev/shm/housesensor.csv is moved to /var/lib/house/sensor/housesensor.csv. The same file is also copied hourly to /var/lib/house/sensor/housesensor.csv. When the service is restarted, /var/lib/house/sensor/housesensor.csv is moved back to /dev/shm. This saves the recorded data when the OS reboots. However a system crash could cause up to one hour worth of recordings to be lost. This is a tradeoff to avoid wearing out a SD card or USB drive by rewriting the same block every minute or so.

The format of the recording is comma-separated variables, where each line represents one sensor measurement with fields in the following order:
* Timestamp (system time).
* Location of sensor (unquoted string).
* Name of sensor (unquoted string).
* Value (numeric or unquoted string).
* Unit (unquoted string).

