/*
 * Stupid status bar.
 * Written by Runxi Yu.
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#define MAXLEN 4

time_t t;
char buf[MAXLEN + 1];
size_t len;
struct tm td;
int fdc, fds;
time_t t1;

int main()
{
	fdc = open("/sys/class/power_supply/macsmc-battery/capacity", O_RDONLY);
	fds = open("/sys/class/power_supply/macsmc-battery/status", O_RDONLY);
	for (;;) {
		t = time(NULL);
		td = *localtime(&t);
		len = pread(fdc, buf + 1, MAXLEN, 0);
		pread(fds, buf, 1, 0);
		buf[len] = '\0';
		dprintf(STDOUT_FILENO, "%s %d-%02d-%02d %02d:%02d:%02d\n", buf, td.tm_year + 1900, td.tm_mon + 1, td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec);
		t = (t + 1) * 1000000;
		for (t1 = time(NULL) * 1000000; t1 < t; t1 = time(NULL) * 1000000)
			usleep(t - t1);
	}
}
