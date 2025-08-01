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
#
# WARNING
#
# This Makefile depends on echttp and houseportal (dev) being installed.

prefix=/usr/local
SHARE=$(prefix)/share/house

INSTALL=/usr/bin/install

HAPP=housesensor

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
	gcc -Os -o housesensor $(OBJS) -lhouseportal -lechttp -lssl -lcrypto -lmagic -lrt

# Distribution agnostic file installation -----------------------

install-ui: install-preamble
	$(INSTALL) -m 0755 -d $(DESTDIR)$(SHARE)/public/sensor
	$(INSTALL) -m 0644 public/* $(DESTDIR)$(SHARE)/public/sensor

install-app: install-ui
	$(INSTALL) -m 0755 -d $(DESTDIR)/var/lib/house/sensor
	$(INSTALL) -m 0755 -s housesensor $(DESTDIR)$(prefix)/bin
	touch $(DESTDIR)/etc/default/housesensor

uninstall-app:
	rm -rf $(DESTDIR)$(SHARE)/public/sensor
	rm -f $(DESTDIR)$(prefix)/bin/housesensor

purge-app:

purge-config:
	rm -f $(DESTDIR)/etc/default/housesensor
	rm -f $(DESTDIR)/etc/house/sensor.config
	rm -f $(DESTDIR)/etc/default/housesensor
	rm -rf $(DESTDIR)/var/lib/house/sensor

# System installation. ------------------------------------------

include $(SHARE)/install.mak

