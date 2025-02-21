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

#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/macsmc-battery/capacity"
#define BATTERY_STATUS_PATH "/sys/class/power_supply/macsmc-battery/status"

#define COLOR_NORMAL "#aaaaaa"
#define COLOR_WARNING "#ff5555"
#define COLOR_CAUTION "#ffaa00"
#define COLOR_GOOD "#33ff33"

int battery_capacity_fd;
int battery_status_fd;

_Bool master_warning;
_Bool master_caution;

int battery_capacity;
char battery_status;
_Bool battery_warning;
_Bool battery_caution;

int setup(void) {
	battery_capacity_fd = open(BATTERY_CAPACITY_PATH, O_RDONLY);
	if (battery_capacity_fd == -1) {
		perror("failed to open battery capacity file");
		return -1;
	}
	battery_status_fd = open(BATTERY_STATUS_PATH, O_RDONLY);
	if (battery_status_fd == -1) {
		perror("failed to open battery status file");
		close(battery_capacity_fd);
		return -1;
	}
	return 0;
}

int read_int_from_fd(int fd) {
	char buf[4];
	if (pread(fd, buf, sizeof(buf) - 1, 0) == -1) {
		perror("failed to read int");
		return -1;
	}
	return atoi(buf); // whatever for zeros
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

	battery_capacity = read_int_from_fd(battery_capacity_fd);
	battery_status = read_char_from_fd(battery_status_fd);

	char *battery_text;
	if (asprintf(&battery_text, "%c%d", battery_status, battery_capacity) == -1)
		return NULL;
	
	cJSON_AddStringToObject(json_obj, "full_text", battery_text);
	cJSON_AddStringToObject(json_obj, "color", COLOR_NORMAL);

	if (battery_status == 'C') {
		cJSON_AddStringToObject(json_obj, "color", COLOR_GOOD);
		battery_warning = 0;
		battery_caution = 0;
	}

	if (battery_capacity < 15) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_WARNING);
		battery_warning = 1;
		battery_caution = 0;
	} else if (battery_capacity < 30) {
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
		battery_warning = 0;
		battery_caution = 1;
	}

	free(battery_text);
	return json_obj;
}

cJSON *component_warning(void) {
	cJSON *json_obj = cJSON_CreateObject();

	if (master_warning) {
		cJSON_AddStringToObject(json_obj, "full_text", "WARNING");
		cJSON_AddStringToObject(json_obj, "color", COLOR_WARNING);
		return json_obj;
	}

	return NULL;
}

cJSON *component_caution(void) {
	cJSON *json_obj = cJSON_CreateObject();

	if (master_caution) {
		cJSON_AddStringToObject(json_obj, "full_text", "CAUTION");
		cJSON_AddStringToObject(json_obj, "color", COLOR_CAUTION);
		return json_obj;
	}

	return NULL;
}

void update_warning_cautions(void) {
	master_warning = battery_warning;
	master_caution = battery_caution;
}

int main(void) {
	if (setup() != 0) {
		return 1;
	}

	dprintf(STDOUT_FILENO, "{ \"version\": 1, \"click_events\": true }\n[\n");

	while (1) {
		cJSON *json_array = cJSON_CreateArray();

		cJSON_AddItemToArray(json_array, component_battery());

		update_warning_cautions();

		cJSON_AddItemToArray(json_array, component_warning());
		cJSON_AddItemToArray(json_array, component_caution());

		char *json_string = cJSON_PrintUnformatted(json_array);
		cJSON_Delete(json_array);
		int json_string_len = strlen(json_string);
		json_string[json_string_len] = ',';
		write(STDOUT_FILENO, json_string, json_string_len + 1);
		free(json_string);
		sleep(1);
	}

	close(battery_capacity_fd);
	close(battery_status_fd);

	return 0;
}
