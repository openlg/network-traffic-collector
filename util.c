//
// Created by Leon on 2024/5/23.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <printf.h>
#include <ctype.h>
#include <unistd.h>
#include "ntc.h"
#include "util.h"

const char *suffix[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int charset_size = sizeof(charset) - 1;

char *xstrdup(const char *s) {
    char *t;
    t = strdup(s);
    if (!t) abort();
    return t;
}

void readable_size(double long bytes, char *result) {
    long double size = bytes;
    int i = 0;
    if (bytes > 0) {
        while (size >= 1024 && i < (sizeof(suffix) / sizeof(suffix[0])) - 1) {
            size /= 1024;
            i++;
        }
    }
    sprintf(result, "%.2Lf%s", size, suffix[i]);
}


long parse_time(const char *time_str) {
    long total_seconds = 0;
    long len = strlen(time_str);
    char *num_str = (char *)malloc(len); // 分配一个足够大的缓冲区
    int num_index = 0;

    for (int i = 0; i < len; i++) {
        if (isdigit(time_str[i])) {
            num_str[num_index++] = time_str[i];
        } else {
            num_str[num_index] = '\0'; // 终止字符串
            int value = atol(num_str); // 将数字字符串转换为长整型

            switch (time_str[i]) {
                case 's':
                    total_seconds += value;
                    break;
                case 'm':
                    total_seconds += value * 60;
                    break;
                case 'h':
                    total_seconds += value * 3600;
                    break;
                default:
                    fprintf(stderr, "Unknown time unit: %c\n", time_str[i]);
                    break;
            }
            num_index = 0; // 重置数字缓冲区索引
        }
    }

    free(num_str); // 释放缓冲区
    return total_seconds;
}

void generate_random_string(int length, char *random_string) {
    if (length <= 0) return;

    srand(time(NULL));

    for (int i = 0; i < length; ++i) {
        random_string[i] = charset[rand() % charset_size];
    }
	random_string[length] = '\0';
}

int contains(const char *haystack, const char *needle) {
	return strstr(haystack, needle) != NULL;
}

#ifdef __linux__
#include <sys/sysinfo.h>
Meter get_memory_meter_linux() {
    struct sysinfo info;
    Meter meter = {0, 0, 0};

    if (sysinfo(&info) == 0) {
		meter.total = (long)info.totalram * info.mem_unit;
		meter.used = ((long)meter.total - (long)info.freeram) * info.mem_unit;
		meter.usage = (double)meter.used / (double)meter.total;
		return meter;
    }

    return meter;
}

// Function to read the CPU statistics from /proc/stat
void get_cpu_times(unsigned long long *cpu_ticks) {
	FILE *fp;
	char line[128];
	fp = fopen("/proc/stat", "r");
	if (!fp) {
		perror("Failed to open /proc/stat");
		exit(EXIT_FAILURE);
	}
	fgets(line, sizeof(line), fp);
	fclose(fp);

	sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu",
		   &cpu_ticks[0], &cpu_ticks[1], &cpu_ticks[2],
		   &cpu_ticks[3], &cpu_ticks[4], &cpu_ticks[5],
		   &cpu_ticks[6]);
}

// Function to calculate CPU usage
double calculate_cpu_usage(unsigned long long *prev_ticks, unsigned long long *curr_ticks) {
	unsigned long long total_ticks_prev, total_ticks_curr, used_ticks;

	total_ticks_prev = prev_ticks[0] + prev_ticks[1] + prev_ticks[2] + prev_ticks[3] +
					   prev_ticks[4] + prev_ticks[5] + prev_ticks[6];

	total_ticks_curr = curr_ticks[0] + curr_ticks[1] + curr_ticks[2] + curr_ticks[3] +
					   curr_ticks[4] + curr_ticks[5] + curr_ticks[6];

	used_ticks = (curr_ticks[0] - prev_ticks[0]) + (curr_ticks[2] - prev_ticks[2]);

	double cpu_usage = ((double)used_ticks / (double)(total_ticks_curr - total_ticks_prev));
	return cpu_usage;
}

double get_cpu_used_linux() {

	// user, nice, system, idle, iowait, irq, softirq
    unsigned long long prev_ticks[7], curr_ticks[7];

	get_cpu_times(prev_ticks);
	sleep(1);
	get_cpu_times(curr_ticks);

    return calculate_cpu_usage(prev_ticks, curr_ticks);
}
#elif __APPLE__
#include <sys/sysctl.h>
#include <mach/vm_statistics.h>
#include <mach/mach.h>

Meter get_memory_meter_macos() {
    Meter meter = {0, 0, 0};

    int64_t mem_size;
    size_t size = sizeof(mem_size);
    sysctlbyname("hw.memsize", &mem_size, &size, NULL, 0);
    meter.total = mem_size;

    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vm_info;
    kern_return_t ret = host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_info, &count);
    if (ret == KERN_SUCCESS) {
        long page_size = sysconf(_SC_PAGESIZE);
        meter.used = vm_info.active_count * page_size;
        meter.usage = (double)meter.used / (double)meter.total;
    }

    return meter;
}

double get_cpu_used_macos() {
    natural_t numCPUs;
    host_cpu_load_info_data_t cpuLoad;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

    // Get the number of CPUs
    host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &numCPUs, (processor_info_array_t *)&cpuLoad, &count);

    uint64_t totalTicks = 0;
    uint64_t usedTicks = 0;

    for (int i = 0; i < CPU_STATE_MAX; i++) {
        totalTicks += cpuLoad.cpu_ticks[i];
        if (i < CPU_STATE_IDLE) {
            usedTicks += cpuLoad.cpu_ticks[i];
        }
    }

    double cpu_usage = ((double)(usedTicks) / (double)totalTicks);
    return cpu_usage;
}

#endif

Meter get_memory_meter() {
#ifdef __linux__
    return get_memory_meter_linux();
#elif __APPLE__
    return get_memory_meter_macos();
#endif
}

Meter get_cpu_meter() {
    Meter meter = {0, 0, 0};
    meter.total = sysconf(_SC_NPROCESSORS_ONLN);
#ifdef __linux__
    meter.usage = get_cpu_used_linux();
#elif __APPLE__
    meter.usage = get_cpu_used_macos();
#endif

    return meter;
}