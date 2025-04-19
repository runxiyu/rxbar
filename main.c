/*
 * SPDX-License-Identifier: CC0-1.0
 * SPDX-FileContributor: Runxi Yu <https://runxiyu.org>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include <time.h>

#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/macsmc-battery/capacity"
#define BATTERY_STATUS_PATH "/sys/class/power_supply/macsmc-battery/status"
#define BATTERY_CURRENT_PATH "/sys/class/power_supply/macsmc-battery/current_now"
#define BATTERY_CHARGE_MAX_PATH "/sys/class/power_supply/macsmc-battery/charge_full"
#define BATTERY_CHARGE_MAX_DESIGN_PATH "/sys/class/power_supply/macsmc-battery/charge_full_design"
#define BATTERY_CHARGE_NOW_PATH "/sys/class/power_supply/macsmc-battery/charge_now"

#define COLOR_NORMAL "#aaaaaa"
#define COLOR_WARNING "#ff5555"
#define COLOR_CAUTION "#ffaa00"
#define COLOR_GOOD "#33ff33"

int battery_capacity_fd;
int battery_charge_full_fd;
int battery_charge_full_design_fd;
int battery_charge_now_fd;
int battery_status_fd;
int battery_current_fd;

_Bool master_warning;
_Bool master_caution;

long long battery_capacity;
long long battery_current;
long long battery_charge_full;
long long battery_charge_full_design;
long long battery_charge_now;
char battery_status;
_Bool battery_warning;
_Bool battery_caution;


struct timespec ts;

void sleep_to_next_second(void) {
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 1;
	ts.tv_nsec = 0;

	clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, NULL);
}

long long read_ll_from_fd(int fd) {
	char buf[30] = {0};
	if (pread(fd, buf, sizeof(buf) - 1, 0) == -1) {
		perror("failed to read int");
		return -1;
	}
	strchr(buf, '\n')[0] = '\0';
	return atoll(buf); // whatever for zeros
}

int setup(void) {
	battery_capacity_fd = open(BATTERY_CAPACITY_PATH, O_RDONLY);
	if (battery_capacity_fd == -1) {
		perror("failed to open battery capacity file");
		return -1;
	}
	battery_charge_full_fd = open(BATTERY_CHARGE_MAX_PATH, O_RDONLY);
	if (battery_charge_full_fd == -1) {
		perror("failed to open battery capacity file");
		return -1;
	}
	battery_charge_full_design_fd = open(BATTERY_CHARGE_MAX_DESIGN_PATH, O_RDONLY);
	if (battery_charge_full_design_fd == -1) {
		perror("failed to open battery capacity file");
		return -1;
	}
	battery_charge_now_fd = open(BATTERY_CHARGE_NOW_PATH, O_RDONLY);
	if (battery_charge_now_fd == -1) {
		perror("failed to open battery capacity file");
		return -1;
	}
	battery_status_fd = open(BATTERY_STATUS_PATH, O_RDONLY);
	if (battery_status_fd == -1) {
		perror("failed to open battery status file");
		close(battery_capacity_fd);
		return -1;
	}
	battery_current_fd = open(BATTERY_CURRENT_PATH, O_RDONLY);
	if (battery_current_fd == -1) {
		perror("failed to open battery current file");
		close(battery_capacity_fd);
		close(battery_status_fd);
		return -1;
	}
	return 0;
}

char read_char_from_fd(int fd) {
	char ret;
	if (pread(fd, &ret, 1, 0) == -1) {
		perror("failed to read char");
		return '?';
	}
	return ret;
}

cJSON *component_battery(void) {
	cJSON *json_obj = cJSON_CreateObject();

	battery_capacity = read_ll_from_fd(battery_capacity_fd);
	battery_current = read_ll_from_fd(battery_current_fd);
	battery_status = read_char_from_fd(battery_status_fd);
	battery_charge_now = read_ll_from_fd(battery_charge_now_fd);
	battery_charge_full = read_ll_from_fd(battery_charge_full_fd);
	battery_charge_full_design = read_ll_from_fd(battery_charge_full_design_fd);

	char *battery_text;
	if (asprintf(&battery_text, "%c%.2f %lld", battery_status, ((double)(100*battery_charge_now))/battery_charge_full, battery_current/1000) == -1)
		return NULL;
	
	cJSON_AddStringToObject(json_obj, "full_text", battery_text);
	cJSON_AddStringToObject(json_obj, "color", COLOR_NORMAL);

	battery_warning = 0;
	battery_caution = 0;

	if (battery_capacity < 15) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_WARNING);
		battery_warning |= 1;
	} else if (battery_capacity < 30) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
		battery_caution |= 1;
	}

	if (battery_status == 'C') {
		cJSON_AddStringToObject(json_obj, "color", COLOR_GOOD);
	}

	if (battery_status == 'N') {
		battery_caution |= 1;
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
	}

	if (battery_current < -620000) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
		battery_caution |= 1;
	}

	if (battery_charge_now > battery_charge_full_design) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_WARNING);
		battery_warning |= 1;
	}

	free(battery_text);
	return json_obj;
}

char time_str[100];

cJSON *component_clock(void) {
	cJSON *json_obj = cJSON_CreateObject();

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	if (strftime(time_str, sizeof(time_str), "%a %Y-%m-%d %H:%M:%S", tm) == 0)
		return NULL;
	cJSON_AddStringToObject(json_obj, "full_text", time_str);
	cJSON_AddStringToObject(json_obj, "color", COLOR_NORMAL);

	return json_obj;
}

cJSON *component_warning(void) {
	cJSON *json_obj = cJSON_CreateObject();

	if (battery_warning) { // EDIT
		cJSON_AddStringToObject(json_obj, "full_text", "WARNING");
		cJSON_AddStringToObject(json_obj, "color", COLOR_WARNING);
		return json_obj;
	}

	return NULL;
}

cJSON *component_caution(void) {
	cJSON *json_obj = cJSON_CreateObject();

	if (battery_caution) { // EDIT
		cJSON_AddStringToObject(json_obj, "full_text", "CAUTION");
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
		return json_obj;
	}

	return NULL;
}

int main(void) {
	if (setup() != 0) {
		return 1;
	}

	dprintf(STDOUT_FILENO, "{ \"version\": 1, \"click_events\": true }\n[\n");

	while (1) {
		cJSON *json_array = cJSON_CreateArray();

		cJSON_AddItemToArray(json_array, component_battery());
		cJSON_AddItemToArray(json_array, component_clock());

		cJSON_AddItemToArray(json_array, component_warning());
		cJSON_AddItemToArray(json_array, component_caution());

		char *json_string = cJSON_PrintUnformatted(json_array);
		cJSON_Delete(json_array);
		int json_string_len = strlen(json_string);
		json_string[json_string_len] = ',';
		write(STDOUT_FILENO, json_string, json_string_len + 1);
		free(json_string);

		sleep_to_next_second();
	}

	close(battery_capacity_fd);
	close(battery_status_fd);

	return 0;
}
