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
 * SYNOPSIS:
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
 * const char *housesensor_db_latest (void);
 *
 *    Get a complete list of latest measurements in JSON format.
 *
 * const char *housesensor_db_recent (void);
 *
 *    Get a list of the N most recent measurements in JSON format.
 *
 * const char *housesensor_db_history (void);
 *
 *    Get a list of the days for which history is available.
 *    (We do not return the whole history: that could be huge.)
 *
 * void housesensor_db_background (time_t now);
 *
 *    This background function performs some cleanup and must be
 *    called periodically.
 */

#include <sys/types.h>
#include <dirent.h>

#include "houseportalclient.h"

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

typedef struct {
    SensorContext *sensor;
    time_t timestamp;
    char *value;
} SensorEvent;

#define SENSOR_DATABASE_BLOCK 64
static SensorContext *SensorDatabase = 0;
static int SensorDatabaseSize = 0;
static int SensorCount = 0;
static int SensorDriverCursor = 0;

static SensorLocation SensorLocationDatabase[SENSOR_DATABASE_BLOCK];
static int SensorLocationCount = 0;

static SensorOption SensorOptionDatabase[128];
static int SensorOptionCount = 0;

static const char SensorLogName[] = "/dev/shm/housesensor.csv";
static FILE *SensorLog = 0;
static time_t SensorLogLastWrite = 0;
static time_t SensorLogLastMove = 0;
static const char SensorArchiveDir[] = "/var/lib/house/sensor";
static const char SensorArchiveFormat[] = "%04d-%02d-%02d.csv";

#define SENSOR_EVENT_DEPTH (SENSOR_DATABASE_BLOCK*128)

SensorEvent SensorEventLog[SENSOR_EVENT_DEPTH];
int SensorEventCursor = 0;


static void SensorEventAdd (SensorContext *sensor) {

    SensorEvent *evt = SensorEventLog + SensorEventCursor;
    evt->sensor = sensor;
    evt->timestamp = sensor->timestamp;
    if (evt->value) free (evt->value);
    evt->value = strdup(sensor->value);

    SensorEventCursor += 1;
    if (SensorEventCursor >= SENSOR_EVENT_DEPTH) SensorEventCursor = 0;
    SensorEventLog[SensorEventCursor].sensor = 0;
}


static int LineSplit (char *buffer, char **token, int max) {

    int i, start, count;

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
    return count;
}

static void AddOption (char **token, int count) {
    SensorOption *o = SensorOptionDatabase + SensorOptionCount++;
    o->name = strdup(token[1]);
    o->value = strdup(token[2]);
}

static void AddSensor (char **token, int count) {

    int i;
    SensorContext *s = SensorDatabase + SensorCount++;

    if (SensorCount > SensorDatabaseSize) {
        SensorDatabaseSize += SENSOR_DATABASE_BLOCK;
        SensorDatabase =
            realloc (SensorDatabase, sizeof(SensorContext)*SensorDatabaseSize);
        if (!SensorDatabase) {
            fprintf (stderr,
                     "No enough memory for %d sensors\n", SensorDatabaseSize);
            exit (1);
        }
        s = SensorDatabase + SensorCount - 1;
    }

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
        if (SensorLocationCount > SENSOR_DATABASE_BLOCK) {
            fprintf (stderr, "Too many locations\n");
            exit (1);
        }
        l->location = s->location;
        l->first = SensorCount - 1;
        s->next = -1;
    } else {
        SensorLocation *l = SensorLocationDatabase + i;
        s->next = l->first;
        l->first = SensorCount - 1;
    }
}

static void DecodeLine (char *buffer) {

    int count;
    char *token[16];

    count = LineSplit (buffer, token, 16);
    if (count < 1) return;

    if (strcmp(token[0],"OPTION") == 0) {
        if (count < 3) {
            fprintf (stderr, "invalid option line (too few items)\n");
            return;
        }
        AddOption (token, count);
        return;
    }

    if (count < 4) {
        fprintf (stderr, "invalid sensor line (too few item)\n");
        return;
    }
    AddSensor (token, count);
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
    time_t now = time(0);
    SensorContext *s;

    // Search for that exact sensor.
    //
    for (i = 0; i < SensorCount; ++i) {
        s = SensorDatabase + i;
        if (strcmp (s->device, device)) continue;
        if (strcmp (s->driver, driver)) continue;
        break;
    }

    if (i < SensorCount) {
        strncpy (s->value, value, sizeof(s->value));
        s->value[sizeof(s->value)-1] = 0;

        if (unit && s->unit[0] == 0) {
            strncpy (s->unit, unit, sizeof(s->unit));
            s->unit[sizeof(s->unit)-1] = 0;
        }
        s->timestamp = now;
        if (echttp_isdebug()) printf ("Set %s.%s to %s %s\n",
                                      driver, device, s->value, s->unit);

        if (SensorLog == 0) {
            SensorLog = fopen (SensorLogName, "a");
        }
        if (SensorLog) {
            fprintf (SensorLog, "%lld,%s,%s,%s,%s\n",
                     (long long)now, s->location, s->name, value, s->unit);
            SensorLogLastWrite = now;
        }
        SensorEventAdd (s);
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

const char *housesensor_db_latest (void) {

    static char buffer[65537];

    char *prefix0 = "";
    char host[256];
    int length;
    int i, j;

    gethostname (host, sizeof(host));
    snprintf (buffer, sizeof(buffer),
             "{\"host\":\"%s\",\"proxy\":\"%s\",\"timestamp\":%ld,\"sensor\":{",
             host, houseportal_server(), (long)time(0));
    length = strlen(buffer);

    for (j = 0; j < SensorLocationCount; ++j) {

        const char *prefix = "";

        snprintf (buffer+length, sizeof(buffer)-length, "%s\"%s\":[",
                  prefix0, SensorLocationDatabase[j].location);
        length += strlen(buffer+length);
        prefix0 = ",";

        for (i = SensorLocationDatabase[j].first;
             i >= 0; i = SensorDatabase[i].next) {

            SensorContext *s = SensorDatabase + i;

            snprintf (buffer+length, sizeof(buffer)-length,
                      "%s{\"name\":\"%s\"", prefix, s->name);
            length += strlen(buffer+length);
            prefix = ",";

            if (s->timestamp) {
                snprintf (buffer+length, sizeof(buffer)-length,
                          ",\"timestamp\":%ld", (long) s->timestamp);
                length += strlen(buffer+length);
            }

            if (s->value[0] == 0) {
                snprintf (buffer+length, sizeof(buffer)-length,
                          ",\"value\":null}");
            } else if (s->unit[0]) {
                snprintf (buffer+length, sizeof(buffer)-length,
                          ",\"value\":%s,\"unit\":\"%s\"}", s->value, s->unit);
            } else {
                snprintf (buffer+length, sizeof(buffer)-length, 
                          ",\"value\":\"%s\"}", s->value);
            }
            length += strlen(buffer+length);
        }
        snprintf (buffer+length, sizeof(buffer)-length, "]");
        length += strlen(buffer+length);
    }
    snprintf (buffer+length, sizeof(buffer)-length, "}}");
    return buffer;
}

const char *housesensor_db_recent (void) {

    static char buffer[SENSOR_EVENT_DEPTH*64];
    const char *prefix = "";
    char host[256];
    int length;
    int i;

    gethostname (host, sizeof(host));

    snprintf (buffer, sizeof(buffer),
              "{\"sensor\":{\"timestamp\":%d,\"host\":\"%s\",\"recent\":[",
              time(0), host);
    length = strlen(buffer);

    for (i = SensorEventCursor + 1; i != SensorEventCursor; ++i) {
        if (i >= SENSOR_EVENT_DEPTH) {
            i = 0;
            if (!SensorEventCursor) break;
        }
        SensorContext *s = SensorEventLog[i].sensor;
        if (s) {

            const char *value = SensorEventLog[i].value;
            time_t timestamp = SensorEventLog[i].timestamp;

            snprintf (buffer+length, sizeof(buffer)-length,
                      "%s{\"location\":\"%s\",\"name\":\"%s\",\"time\":%ld",
                      prefix, s->location, s->name, timestamp);
            length += strlen(buffer+length);
            prefix = ",";

            if (s->unit[0]) {
                snprintf (buffer+length, sizeof(buffer)-length,
                          ",\"value\":%s,\"unit\":\"%s\"}", value, s->unit);
            } else {
                snprintf (buffer+length, sizeof(buffer)-length, 
                          ",\"value\":\"%s\"}", value);
            }
            length += strlen(buffer+length);
        }
    }
    snprintf (buffer+length, sizeof(buffer)-length, "]}}");
    return buffer;
}

const char *housesensor_db_history (void) {

    static char buffer[65537];

    DIR *d = opendir (SensorArchiveDir);
    char host[256];

    gethostname (host, sizeof(host));

    if (d) {
        int length;
        struct dirent *de;
        const char *prefix = "";

        snprintf (buffer, sizeof(buffer),
                  "{\"sensor\":{\"timestamp\":%d,\"host\":\"%s\",\"history\":[",
                  time(0), host);
        length = strlen(buffer);

        while (de = readdir(d)) {
            if (de->d_name[0] == '.') continue;
            snprintf (buffer+length, sizeof(buffer)-length,
                      "%s\"%10.10s\"", prefix, de->d_name);
            length += strlen(buffer+length);
            prefix = ",";
        }
        snprintf (buffer+length, sizeof(buffer)-length, "]}}");
        closedir(d);
        return buffer;
    }

    snprintf (buffer, sizeof(buffer),
              "{\"sensor\":{\"timestamp\":%d,\"host\":\"%s\",\"history\":[]}}",
              time(0), host);
    return buffer;
}

static void housesensor_db_backup (const struct tm *t, const char *method) {

    char archivename[256];
    char command[1024];

    snprintf (archivename, sizeof(archivename), SensorArchiveFormat,
              t->tm_year+1900, t->tm_mon+1, t->tm_mday);

    snprintf (command, sizeof(command), "%s %s %s/%s",
              method, SensorLogName, SensorArchiveDir, archivename);

    system (command);
}

void housesensor_db_background (time_t now) {

    static time_t LastHourlyBackup = 0;

    struct tm *t;
    time_t onehourbefore = now - 3600;

    if (SensorLog && now > SensorLogLastWrite + 10) {

        fclose (SensorLog);
        SensorLog = 0;

        t = localtime (&onehourbefore);

        if (t->tm_hour == 23 && now > SensorLogLastMove + 3601) {

            housesensor_db_backup (t, "mv");
            SensorLogLastMove = LastHourlyBackup = now;

        } else if (onehourbefore > LastHourlyBackup) {

            housesensor_db_backup (localtime (&now), "cp");
            LastHourlyBackup = now;
        }
    }
}

void housesensor_db_initialize (int argc, const char **argv) {

    struct tm *t;
    time_t yesterday = time(0) - 3600;
    const char *config = "/etc/house/sensor.config";

    int i;
    for (i = 1; i < argc; ++i) {
        echttp_option_match ("-config=", argv[i], &config);
    }

    LoadConfig (config);

    // If we start at midnight, and yesterday's log already exists,
    // do not re-archive today's data as if it was yesterday's.
    //
    t = localtime (&yesterday);
    if (t->tm_hour == 23) {
        FILE *f;
        char name[1024];
        snprintf (name, sizeof(name), SensorArchiveFormat,
                  t->tm_year+1900, t->tm_mon+1, t->tm_mday);
        f = fopen(name, "r");
        if (f) {
            SensorLogLastMove = yesterday + 3600;
            fclose(f);
        }
    }
}

