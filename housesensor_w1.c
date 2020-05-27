/* housesensor - A simple home web server for measurements.
 *
 * Copyright 2019, Pascal Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *
 * housesensor_w1.c - The Linux 1-Wire interface.
 *
 * SYNOPSYS:
 *
 */

#include "housesensor.h"
#include "housesensor_w1.h"
#include "housesensor_db.h"


static const char *DS1820[] = {"10-", "28-", 0};
static int ScanPeriod = 10;

static int BelongsTo (const char *id, const char **list) {
    int i;
    for (i = 0; list[i]; ++i) {
        if (strncmp (id, list[i], strlen(list[i])) == 0) return 1;
    }
    return 0;
}

static void ReadDevice (const char *id) {

    char name [1024];
    char line[80];
    char *p;
    char *eol;
    FILE *f;

    snprintf (name, sizeof(name), "/sys/bus/w1/devices/%s/w1_slave", id);
    if (echttp_isdebug()) printf ("Scanning %s at %ld\n", name, time(0));

    f = fopen (name, "r");
    if (f) {
        fgets (line, sizeof(line), f);
        if (strstr (line, " YES")) {
            fgets (line, sizeof(line), f);
            if (BelongsTo (id, DS1820)) {
                p = strstr(line, " t=");
                if (p) {
                    for (eol = p; *eol > 0; ++eol) {
                        if (*eol < ' ') {
                            *eol = 0;
                            break;
                        }
                    }
                    housesensor_db_set ("w1", id, p+3, "Celsius");
                }
            }
        }
        fclose(f);
    }
    else if (echttp_isdebug()) printf ("    .. Not found\n");
}

void housesensor_w1_initialize (int argc, const char **argv) {
    const char *period = housesensor_db_option ("w1.scan.period");
    if (period) {
        ScanPeriod = atoi(period);
        if (ScanPeriod <= 5) ScanPeriod = 5;
    }
}

void housesensor_w1_background (time_t now) {

    static time_t LastScan = 0;
    char name[256];
    const char *device;

    if (now >= LastScan + ScanPeriod) {
        for (device = housesensor_db_device_first("w1");
             device; device = housesensor_db_device_next("w1")) {
            ReadDevice (device);
        }
        LastScan = now;
    }
}

