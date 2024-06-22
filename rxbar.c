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

#define LEN 10

time_t t;
char buf[LEN];
struct tm td;
int fd;
int i;
time_t t1;
time_t t2;

int main()
{
	fd = open("/sys/class/power_supply/macsmc-battery/capacity", O_RDONLY);
	for (;;) {
		t = time(NULL);
		td = *localtime(&t);
		pread(fd, buf, LEN, 0);
		for (i = 0; i < LEN; ++i) {
			if (buf[i] == '\n') {
				buf[i] = '\0';
				break;
			}
		}
		dprintf(STDOUT_FILENO, "%s%% %d-%02d-%02d %02d:%02d:%02d\n",
			buf,
			td.tm_year + 1900,
			td.tm_mon + 1,
			td.tm_mday, td.tm_hour, td.tm_min, td.tm_sec);
		t = (t + 1) * 1000000;
		for (t1 = time(NULL) * 1000000; t1 < t;
		     t1 = time(NULL) * 1000000) {
			t2 = t - t1;
			usleep(t2);
		}
	}
}
