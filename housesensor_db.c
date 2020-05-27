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
 * housesensor_db.c - The database of sensors.
 *
 * SYNOPSYS:
 *
 * void housesensor_db_initialize (int argc, const char **argv);
 *
 *    Load the sensor database. Must be called once.
 *
 * void housesensor_db_set (const char *driver, const char *device,
 *                          const char *value, const char *unit);
 *
 *    Set the value for a specific sensor.
 *
 * const char *housesensor_db_device_first (const char *driver);
 * const char *housesensor_db_device_next (const char *driver);
 *
 *    Search all devices for a specific driver.
 *
 * const char *housesensor_db_option (const char *name);
 *
 *    Get the value for the specified option.
 *
 * const char *housesensor_db_json (char *buffer, int size);
 *
 *    Get a complete list of latest measurement in JSON format.
 */

#include "housesensor.h"
#include "housesensor_db.h"


typedef struct {
    char *location;
    int   first;
} SensorLocation;

typedef struct {
    char *driver;
    char *device;
    char *location;
    char *name;
    char unit[32];
    char value[128]; // ASCII representation.
    time_t timestamp;
    int   next;
} SensorContext;

typedef struct {
    char *name;
    char *value;
} SensorOption;

#define SENSOR_DATABASE_SIZE 1024
static SensorContext SensorDatabase[SENSOR_DATABASE_SIZE];
static int SensorCount = 0;
static int SensorDriverCursor = 0;

static SensorLocation SensorLocationDatabase[SENSOR_DATABASE_SIZE];
static int SensorLocationCount = 0;

static SensorOption SensorOptionDatabase[128];
static int SensorOptionCount = 0;

static void DecodeLine (char *buffer) {

    int i, start, count;

    char *token[16];

    // Split the line
    for (i = start = count = 0; buffer[i] >= ' '; ++i) {
        if (buffer[i] == ' ') {
            if (count >= 16) {
                fprintf (stderr, "too many tokens at %s\n", buffer+i);
                exit(1);
            }
            token[count++] = buffer + start;
            do {
               buffer[i] = 0;
            } while (buffer[++i] == ' ');
            start = i;
        }
    }
    buffer[i] = 0;
    token[count++] = buffer + start;

    if (count >= 3 && strcmp(token[0],"OPTION") == 0) {
        SensorOption *o = SensorOptionDatabase + SensorOptionCount++;
        o->name = strdup(token[1]);
        o->value = strdup(token[2]);
        return;
    }

    if (count >= 4 && SensorCount < SENSOR_DATABASE_SIZE) {
        SensorContext *s = SensorDatabase + SensorCount++;
        s->driver = strdup(token[0]);
        s->device = strdup(token[1]);
        s->location = strdup(token[2]);
        s->name = strdup(token[3]);
        if (count >= 5)
            strncpy (s->unit, token[4], sizeof(s->unit));
        else
            s->unit[0] = 0;
        s->value[0] = 0;
        s->timestamp = 0;
        for (i = 0; i < SensorLocationCount; ++i) {
            if (strcmp(SensorLocationDatabase[i].location, s->location) == 0)
                break;
        }
        if (i >= SensorLocationCount) {
            SensorLocation *l = SensorLocationDatabase + SensorLocationCount++;
            l->location = s->location;
            l->first = SensorCount - 1;
            s->next = -1;
        } else {
            SensorLocation *l = SensorLocationDatabase + i;
            s->next = l->first;
            l->first = SensorCount - 1;
        }
    }
}

static void LoadConfig (const char *name) {

    char buffer[1024];
    FILE *f = fopen (name, "r");

    if (f == 0) {
        fprintf (stderr, "cannot access configuration file %s\n", name);
        exit(1);
    }

    while (!feof(f)) {
        buffer[0] = 0;
        fgets (buffer, sizeof(buffer), f);
        if (buffer[0] != '#' && buffer[0] > ' ') {
            DecodeLine (buffer);
        }
    }
    fclose(f);
}

const char *housesensor_db_device_first (const char *driver) {

    SensorDriverCursor = 0;
    return housesensor_db_device_next (driver);
}

const char *housesensor_db_device_next (const char *driver) {

    int i;

    for (i = SensorDriverCursor; i < SensorCount; ++i) {
        SensorContext *s = SensorDatabase + i;
        if (strcmp (s->driver, driver) == 0) {
            SensorDriverCursor = i + 1;
            return s->device;
        }
    }
    return 0;
}

void housesensor_db_set (const char *driver, const char *device,
                         const char *value, const char *unit) {
    int i;
    for (i = 0; i < SensorCount; ++i) {
        SensorContext *s = SensorDatabase + i;
        if (strcmp (s->device, device)) continue;
        if (strcmp (s->driver, driver)) continue;

        strncpy (s->value, value, sizeof(s->value));
        s->value[sizeof(s->value)-1] = 0;

        if (unit && s->unit[0] == 0) {
            strncpy (s->unit, unit, sizeof(s->unit));
            s->unit[sizeof(s->unit)-1] = 0;
        }
        s->timestamp = time(0);;
        break;
    }
}

const char *housesensor_db_option (const char *name) {

    int i;

    for (i = 0; i < SensorOptionCount; ++i) {
        SensorOption *s = SensorOptionDatabase + i;
        if (strcmp (s->name, name) == 0) {
            return s->value;
        }
    }
    return 0;
}

void housesensor_db_json (char *buffer, int size) {

    char host[256];
    int length;
    int i, j;

    gethostname (host, sizeof(host));
    snprintf (buffer, size,
             "{\"sensor\":{\"timestamp\":%ld, \"host\":\"%s\"",
             (long)time(0), host);
    length = strlen(buffer);

    for (j = 0; j < SensorLocationCount; ++j) {

        const char *prefix = "";

        snprintf (buffer+length, size-length,
                 ",\"%s\":[", SensorLocationDatabase[j].location);
        length += strlen(buffer+length);

        for (i = SensorLocationDatabase[j].first;
             i >= 0; i = SensorDatabase[i].next) {

            SensorContext *s = SensorDatabase + i;

            snprintf (buffer+length, size-length,
                      "%s{\"name\":\"%s\"", prefix, s->name);
            length += strlen(buffer+length);
            prefix = ",";

            if (s->timestamp) {
                snprintf (buffer+length, size-length,
                          ",\"timestamp\":%ld", (long) s->timestamp);
                length += strlen(buffer+length);
            }

            if (s->value[0] == 0) {
                snprintf (buffer+length, size-length, ",\"value\":null}");
            } else if (s->unit[0]) {
                snprintf (buffer+length, size-length,
                          ",\"value\":%s,\"unit\":\"%s\"}", s->value, s->unit);
            } else {
                snprintf (buffer+length, size-length, 
                          ",\"value\":\"%s\"}", s->value);
            }
            length += strlen(buffer+length);
        }
        snprintf (buffer+length, size-length, "]");
        length += strlen(buffer+length);
    }
    snprintf (buffer+length, size-length, "}}");
}

void housesensor_db_initialize (int argc, const char **argv) {

    const char *config = "/etc/house/sensor.config";

    int i;
    for (i = 1; i < argc; ++i) {
        echttp_option_match ("-config=", argv[i], &config);
    }

    LoadConfig (config);
}

