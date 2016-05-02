/*
 * kernel/power/power_mode.c
 *
 * add /sys/power/power_mode for set save mode mode .
 *
 * Copyright (C) 2014 zhengwei. zhengwei <zhengwei@meizu.com>
 */

#include <linux/device.h>
#include <linux/mutex.h>
#include "power.h"

#include <linux/power_mode.h>
#include <mach/cpufreq.h>
#include <linux/perf_mode.h>

static struct power_mode_info *cur_power_mode = NULL;
static int power_mode_debug;
static int power_mode_init_flag = 0;

static DEFINE_MUTEX(power_lock);

#define MAX_CLUSTER0_FREQ	(1800000)
#define MAX_CLUSTER1_FREQ	(2400000)
#define MAX_GPU_FREQ	(852)

struct power_pm_qos {
	struct pm_qos_request cluster0_max_freq_qos;	/* freqs */
	struct pm_qos_request cluster1_max_freq_qos;
	struct pm_qos_request gpu_max_freq_qos;
	struct pm_qos_request gpu_min_freq_qos;
	struct pm_qos_request cluster0_min_freq_qos;
	struct pm_qos_request cluster1_min_freq_qos;
	struct pm_qos_request cluster1_min_num_qos;
	struct pm_qos_request cluster1_max_num_qos;		/* number */
	struct pm_qos_request cluster0_max_num_qos;
};

enum {
	PM_QOS_POWER_MODE = 0,
	PM_QOS_THERMAL,
	PM_QOS_END,
};

static struct power_pm_qos power_pm_qos[PM_QOS_END];

/* For the available freqs, please see: kernel/power/perf_mode.c */

static struct power_mode_info power_mode_info[POWER_MODE_END] = {
	/* name,       cluster1_max_freq, cluster0_max_freq, gpu_max_freq, gpu_min_freq, cluster0_min_freq, cluster1_min_freq, cluster1_min_num, cluster0_max_num, cluster1_max_num*/
	{ "low",       1200000,  1500000,     544,        0,         0,   	0,         0,   4,          0 },
	{ "normal",    1704000,  1500000,     700,        0,         0,   	0,         0,   4,          2 },
	{ "high",      2400000,  1800000,     852,        0,   	     0,   	0,         4,   4,          4 },
	{ "benchmark", 2400000,  1800000,     852,        0,         0,   	0,         4,   4,          4 },
	{ "custom",    2400000,  1800000,     852,        0,         0,         0,         4,   4,          4 },
	{ "thermal",   2400000,  1800000,     852,        0,         0,   	0,         4,   4,          4 },
};

/* add power mode notify service*/

static BLOCKING_NOTIFIER_HEAD(power_mode_notifier_list);

int power_mode_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&power_mode_notifier_list, nb);
}
EXPORT_SYMBOL(power_mode_register_notifier);

int power_mode_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&power_mode_notifier_list, nb);
}
EXPORT_SYMBOL(power_mode_unregister_notifier);

int power_mode_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&power_mode_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(power_mode_notifier_call_chain);

/* request_power_mode(): Request a mode of power
 *
 * @mode: POWER_MODE_LOW...POWER_MODE_CUSTOM
 * @qos: PM_QOS_POWER_MODE...PM_QOS_THERMAL
 * @timeout: 0, never timeout; > 0, timeout
 */

static void request_power_mode_unlocked(unsigned int mode, int qos, unsigned long timeout)
{
	struct power_mode_info *p;
	struct power_pm_qos *q;

	if (mode >= POWER_MODE_END || mode < POWER_MODE_LOW)
		return;

	if (qos >= PM_QOS_END || qos < PM_QOS_POWER_MODE)
		return;

	p = &power_mode_info[mode];
	q = &power_pm_qos[qos];

	if (timeout) {
		/* Lock Cluster0_MAX_FREQ cpus: 0~3 */
		pm_qos_update_request_timeout(&q->cluster0_max_freq_qos, p->cluster0_max_freq, timeout);

		/* Lock Cluster1_MAX_FREQ cpus: 4~7 */
		pm_qos_update_request_timeout(&q->cluster1_max_freq_qos, p->cluster1_max_freq, timeout);

		/* Lock GPU_MAX_FREQ */
		pm_qos_update_request_timeout(&q->gpu_max_freq_qos, p->gpu_max_freq, timeout);

		/* Lock GPU_MIN_FREQ */
		pm_qos_update_request_timeout(&q->gpu_min_freq_qos, p->gpu_min_freq, timeout);

		/* Lock Cluster0_MIN_FREQ cpus: 0~3 */
		pm_qos_update_request_timeout(&q->cluster0_min_freq_qos, p->cluster0_min_freq, timeout);

		/* Lock Cluster1_MIN_FREQ cpus: 4~7 */
		pm_qos_update_request_timeout(&q->cluster1_min_freq_qos, p->cluster1_min_freq, timeout);

		/* Lock Cluster1_MIN_NUM */
		pm_qos_update_request_timeout(&q->cluster1_min_num_qos, p->cluster1_min_num, timeout);

		/* Lock CLUSTER1_MAX_NUM */
		pm_qos_update_request_timeout(&q->cluster1_max_num_qos, p->cluster1_max_num, timeout);

		/* Lock CLUSTER0_MAX_NUM */
		pm_qos_update_request_timeout(&q->cluster0_max_num_qos, p->cluster0_max_num, timeout);
	} else {
		/* Lock Cluster0_MAX_FREQ cpus: 0~3 */
		pm_qos_update_request(&q->cluster0_max_freq_qos, p->cluster0_max_freq);

		/* Lock Cluster1_MAX_FREQ cpus: 4~7 */
		pm_qos_update_request(&q->cluster1_max_freq_qos, p->cluster1_max_freq);

		/* Lock GPU_MAX_FREQ */
		pm_qos_update_request(&q->gpu_max_freq_qos, p->gpu_max_freq);

		/* Lock GPU_MIN_FREQ */
		pm_qos_update_request(&q->gpu_min_freq_qos, p->gpu_min_freq);

		/* Lock Cluster0_MIN_FREQ cpus: 0~3 */
		pm_qos_update_request(&q->cluster0_min_freq_qos, p->cluster0_min_freq);

		/* Lock Cluster1_MIN_FREQ cpus: 4~7 */
		pm_qos_update_request(&q->cluster1_min_freq_qos, p->cluster1_min_freq);

		/* Lock Cluster1_MIN_NUM */
		pm_qos_update_request(&q->cluster1_min_num_qos, p->cluster1_min_num);

		/* Lock CLUSTER1_MAX_NUM */
		pm_qos_update_request(&q->cluster1_max_num_qos, p->cluster1_max_num);

		/* Lock CLUSTER0_MAX_NUM */
		pm_qos_update_request(&q->cluster0_max_num_qos, p->cluster0_max_num);
	}

	if (power_mode_debug)
		pr_info("%s: cluster0: %u cluster1: %u gpu: %u mif: %u int: %u disp: %u isp: %u cpu: %d kfc: %d timeout: %lu (ms)\n", __func__,
			p->cluster0_max_freq, p->cluster1_max_freq, p->gpu_max_freq, p->gpu_min_freq, p->cluster0_min_freq, p->cluster1_min_freq, p->cluster1_min_num,
			p->cluster1_max_num, p->cluster0_max_num, timeout ? timeout / 1000 : 0);
}

void __request_power_mode(unsigned int mode, int qos, unsigned long timeout)
{
	mutex_lock(&power_lock);
	request_power_mode_unlocked(mode, qos, timeout);
	mutex_unlock(&power_lock);
}

static void request_power_mode(unsigned int cluster1_max_freq, unsigned int cluster0_max_freq,
		unsigned int gpu_max_freq, unsigned int gpu_min_freq, unsigned int cluster0_min_freq,
		unsigned int cluster1_min_freq, unsigned int cluster1_min_num,
		unsigned int cluster0_max_num, unsigned int cluster1_max_num, int mode, int qos)
{
	struct power_mode_info *p;

	mutex_lock(&power_lock);

	p = &power_mode_info[mode];
	p->cluster1_max_freq = cluster1_max_freq;
	p->cluster0_max_freq = cluster0_max_freq;
	p->gpu_max_freq = gpu_max_freq;
	p->gpu_min_freq = gpu_min_freq;
	p->cluster0_min_freq = cluster0_min_freq;
	p->cluster1_min_freq = cluster1_min_freq;
	p->cluster1_min_num = cluster1_min_num;
	p->cluster0_max_num = cluster0_max_num;
	p->cluster1_max_num = cluster1_max_num;

	request_power_mode_unlocked(mode, qos, 0);

	mutex_unlock(&power_lock);
}

void request_thermal_power_mode(unsigned int cluster1_max_freq, unsigned int cluster0_max_freq,
		unsigned int gpu_max_freq, unsigned int gpu_min_freq, unsigned int cluster0_min_freq,
		unsigned int cluster1_min_freq, unsigned int cluster1_min_num,
		unsigned int cluster0_max_num, unsigned int cluster1_max_num)
{
	if (!power_mode_init_flag)
		return;

	request_power_mode(cluster1_max_freq, cluster0_max_freq, gpu_max_freq, gpu_min_freq, cluster0_min_freq, cluster1_min_freq, cluster1_min_num,
		cluster0_max_num, cluster1_max_num, POWER_MODE_THERMAL, PM_QOS_THERMAL);
}

void request_custom_power_mode(unsigned int cluster1_max_freq, unsigned int cluster0_max_freq,
		unsigned int gpu_max_freq, unsigned int gpu_min_freq, unsigned int cluster0_min_freq,
		unsigned int cluster1_min_freq, unsigned int cluster1_min_num,
		unsigned int cluster0_max_num, unsigned int cluster1_max_num)
{
	request_power_mode(cluster1_max_freq, cluster0_max_freq, gpu_max_freq, gpu_min_freq, cluster0_min_freq, cluster1_min_freq, cluster1_min_num,
		cluster0_max_num, cluster1_max_num, POWER_MODE_CUSTOM, PM_QOS_POWER_MODE);
}

static void show_power_mode_list(void)
{
	int i;

	pr_debug("================== power mode ====================\n");

	pr_debug("%10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n", "name", "cluster1_max_freq", "cluster0_max_freq", "gpu_max_freq", "gpu_min_freq", "cluster0_min_freq", "cluster1_min_freq", "cluster1_min_num", "cluster0_max_num", "cluster1_max_num");
	for (i = 0; i < POWER_MODE_END; i++) {
		if (i == POWER_MODE_BENCHMARK)
			continue;
		pr_debug("%10s %10u %10u %10u %10u %10u %10u %10u %10u %10u\n",
			power_mode_info[i].name, power_mode_info[i].cluster1_max_freq, power_mode_info[i].cluster0_max_freq,
			power_mode_info[i].gpu_max_freq, power_mode_info[i].gpu_min_freq, power_mode_info[i].cluster0_min_freq,
			power_mode_info[i].cluster1_min_freq, power_mode_info[i].cluster1_min_num,
			power_mode_info[i].cluster0_max_num, power_mode_info[i].cluster1_max_num);
	}
}

static ssize_t show_power_mode(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int ret;

	mutex_lock(&power_lock);
	if (strcmp("benchmark", cur_power_mode->name) == 0)
		ret = sprintf(buf, "power_mode: high.\n");
	else
		ret = sprintf(buf, "power_mode: %s\n", cur_power_mode->name);
	mutex_unlock(&power_lock);
	show_power_mode_list();

	return ret;
}

static ssize_t store_power_mode(struct kobject *kobj, struct attribute *attr,const char *buf, size_t count)
{
	char str_power_mode[POWER_MODE_LEN];
	int ret , i;

	ret = sscanf(buf, "%11s", str_power_mode);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&power_lock);
	for (i = 0; i < POWER_MODE_END; i++) {
		if (!strnicmp(power_mode_info[i].name, str_power_mode, POWER_MODE_LEN)) {
			break;
		}
	}

	if (i < POWER_MODE_END) {
		cur_power_mode = &power_mode_info[i];
		if (i <= POWER_MODE_CUSTOM)
			request_power_mode_unlocked(i, PM_QOS_POWER_MODE, 0);
		else
			request_power_mode_unlocked(i, PM_QOS_THERMAL, 0);

		power_mode_notifier_call_chain(i, cur_power_mode);

		if (strcmp("benchmark", cur_power_mode->name) == 0)
			pr_info("store power_mode to high... \n");
		else
			pr_info("store power_mode to %s \n", cur_power_mode->name);
	}
	mutex_unlock(&power_lock);

	return count;
}

static ssize_t show_power_debug(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int ret;

	mutex_lock(&power_lock);
	ret = sprintf(buf, "debug: %u\n", power_mode_debug);
	mutex_unlock(&power_lock);

	return ret;
}

static ssize_t store_power_debug(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int debug;

	mutex_lock(&power_lock);

	ret = sscanf(buf, "%u", &debug);
	if (ret != 1)
		goto fail;

	power_mode_debug = debug;
	pr_info("%s: debug = %u\n", __func__, power_mode_debug);
	mutex_unlock(&power_lock);

	return count;

fail:
	pr_err("usage: echo debug > /sys/power/power_debug\n\n");
	mutex_unlock(&power_lock);

	return -EINVAL;
}

static ssize_t show_power_custom(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int ret;
	struct power_mode_info *p;

	mutex_lock(&power_lock);
	p = &power_mode_info[POWER_MODE_CUSTOM];
	ret = sprintf(buf, "cluster1: %u cluster0: %u gpu: %u mif: %u int: %u disp: %u isp: %u kfc: %u cpu: %u\n",
		p->cluster1_max_freq, p->cluster0_max_freq, p->gpu_max_freq, p->gpu_min_freq, p->cluster0_min_freq, p->cluster1_min_freq, p->cluster1_min_num,
		p->cluster0_max_num, p->cluster1_max_num);
	mutex_unlock(&power_lock);

	return ret;
}

static ssize_t store_power_custom(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int cluster1_max_freq, cluster0_max_freq, gpu_max_freq, gpu_min_freq, cluster0_min_freq, cluster1_min_freq, cluster1_min_num, cluster0_max_num, cluster1_max_num;
	struct power_mode_info *p;

	mutex_lock(&power_lock);

	ret = sscanf(buf, "%u %u %u %u %u %u %u %u %u", &cluster1_max_freq, &cluster0_max_freq, &gpu_max_freq, &gpu_min_freq, &cluster0_min_freq, &cluster1_min_freq,
			&cluster1_min_num, &cluster0_max_num, &cluster1_max_num);
	if (ret != 9)
		goto fail;

	p = &power_mode_info[POWER_MODE_CUSTOM];
	p->cluster1_max_freq = cluster1_max_freq;
	p->cluster0_max_freq = cluster0_max_freq;
	p->gpu_max_freq = gpu_max_freq;
	p->gpu_min_freq = gpu_min_freq;
	p->cluster0_min_freq = cluster0_min_freq;
	p->cluster1_min_freq = cluster1_min_freq;
	p->cluster1_min_num = cluster1_min_num;
	p->cluster0_max_num = cluster0_max_num;
	p->cluster1_max_num = cluster1_max_num;

	if (power_mode_debug)
		pr_info("custom:\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
			p->cluster1_max_freq, p->cluster0_max_freq, p->gpu_max_freq, p->gpu_min_freq, p->cluster0_min_freq, p->cluster1_min_freq, p->cluster1_min_num,
			p->cluster0_max_num, p->cluster1_max_num);

	mutex_unlock(&power_lock);

	return count;

fail:
	pr_err("usage: echo cluster1_max_freq cluster0_max_freq gpu_max_freq gpu_min_freq cluster0_min_freq cluster1_min_freq cluster1_min_num cluster0_max_num cluster1_max_num > /sys/power/power_custom\n\n");
	mutex_unlock(&power_lock);

	return -EINVAL;
}

define_one_global_rw(power_mode);
define_one_global_rw(power_debug);
define_one_global_rw(power_custom);

static struct attribute * g[] = {
	&power_mode.attr,
	&power_debug.attr,
	&power_custom.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static int power_mode_init(void)
{
	int error, i;

	pr_err("M86 Power mode init, default POWER_MODE_HIGH\n");

	error = sysfs_create_group(power_kobj, &attr_group);
	if (error)
		return error;

	mutex_init(&power_lock);
	cur_power_mode = &power_mode_info[POWER_MODE_HIGH];
	/* show_power_mode_list(); */

	for (i = PM_QOS_POWER_MODE; i < PM_QOS_END; i++) {
		pm_qos_add_request(&power_pm_qos[i].cluster0_max_freq_qos, PM_QOS_CLUSTER0_FREQ_MAX, MAX_CLUSTER0_FREQ);
		pm_qos_add_request(&power_pm_qos[i].cluster1_max_freq_qos, PM_QOS_CLUSTER1_FREQ_MAX, MAX_CLUSTER1_FREQ);
		pm_qos_add_request(&power_pm_qos[i].gpu_max_freq_qos, PM_QOS_GPU_FREQ_MAX, MAX_GPU_FREQ);
		pm_qos_add_request(&power_pm_qos[i].gpu_min_freq_qos, PM_QOS_GPU_FREQ_MIN, 0);
		pm_qos_add_request(&power_pm_qos[i].cluster0_min_freq_qos, PM_QOS_CLUSTER0_FREQ_MIN, PM_QOS_CLUSTER0_FREQ_MIN_DEFAULT_VALUE);
		pm_qos_add_request(&power_pm_qos[i].cluster1_min_freq_qos, PM_QOS_CLUSTER1_FREQ_MIN, PM_QOS_CLUSTER1_FREQ_MIN_DEFAULT_VALUE);
		pm_qos_add_request(&power_pm_qos[i].cluster1_min_num_qos, PM_QOS_CLUSTER1_NUM_MIN, 0);
		pm_qos_add_request(&power_pm_qos[i].cluster1_max_num_qos, PM_QOS_CLUSTER1_NUM_MAX, NR_CLUST1_CPUS);
		pm_qos_add_request(&power_pm_qos[i].cluster0_max_num_qos, PM_QOS_CLUSTER0_NUM_MAX, NR_CLUST0_CPUS);
	}

	power_mode_init_flag = 1;

	return error;
}

late_initcall_sync(power_mode_init);
