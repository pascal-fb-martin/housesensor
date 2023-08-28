# housesensor - A simple home web server for sensor measurement.
#
# Copyright 2023, Pascal Martin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

HAPP=housesensor
HROOT=/usr/local
SHARE=$(HROOT)/share/house

# Application build. --------------------------------------------

OBJS= housesensor.o housesensor_w1.o housesensor_db.o
LIBOJS=

all: housesensor

main: housesensor.o
db: housesensor_db.o

clean:
	rm -f *.o *.a housesensor

rebuild: clean all

%.o: %.c
	gcc -c -Os -o $@ $<

housesensor: $(OBJS)
	gcc -Os -o housesensor $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lrt

# Distribution agnostic file installation -----------------------

install-app:
	mkdir -p $(HROOT)/bin
	mkdir -p /var/lib/house/sensor
	rm -f $(HROOT)/bin/housesensor
	cp housesensor $(HROOT)/bin
	chown root:root $(HROOT)/bin/housesensor
	chmod 755 $(HROOT)/bin/housesensor
	mkdir -p $(SHARE)/public/sensor
	chmod 755 $(SHARE) $(SHARE)/public $(SHARE)/public/sensor
	cp public/* $(SHARE)/public/sensor
	chown root:root $(SHARE)/public/sensor/*
	chmod 644 $(SHARE)/public/sensor/*
	mkdir -p /etc/house
	touch /etc/house/sensor.config
	touch /etc/default/housesensor

uninstall-app:
	rm -rf $(SHARE)/public/sensor
	rm -f $(HROOT)/bin/housesensor

purge-app:

purge-config:
	rm -rf /etc/house/sensor.config /etc/default/housesensor /var/lib/house/sensor

# System installation. ------------------------------------------

include $(SHARE)/install.mak

