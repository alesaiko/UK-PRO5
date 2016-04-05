#ifndef POWER_MODE
#define POWER_MODE

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pm_qos.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/notifier.h>

#define POWER_MODE_LEN	(11)
#define BENCHMARK_HMP_BOOST_TIMEOUT_US (3 * 60 * 1000 * 1000UL)

struct power_mode_info {
	char		*name;
	unsigned int	cluster1_max_freq;
	unsigned int	cluster0_max_freq;
	unsigned int	gpu_max_freq;
	unsigned int	gpu_min_freq;
	unsigned int	cluster0_min_freq;
	unsigned int	cluster1_min_freq;
	unsigned int	cluster1_min_num;
	unsigned int	cluster0_max_num;
	unsigned int	cluster1_max_num;
};

enum power_mode_idx {
	POWER_MODE_LOW,
	POWER_MODE_NORMAL,
	POWER_MODE_HIGH,
	POWER_MODE_BENCHMARK,
	POWER_MODE_CUSTOM,
	POWER_MODE_THERMAL,
	POWER_MODE_END,
};

#ifdef CONFIG_POWER_MODE
extern int power_mode_register_notifier(struct notifier_block *nb);
extern int power_mode_unregister_notifier(struct notifier_block *nb);
extern int power_mode_notifier_call_chain(unsigned long val, void *v);
extern void request_thermal_power_mode(unsigned int cluster1_max_freq, unsigned int cluster0_max_freq,
		unsigned int gpu_max_freq, unsigned int gpu_min_freq, unsigned int cluster0_min_freq,
		unsigned int cluster1_min_freq, unsigned int cluster1_min_num,
		unsigned int cluster0_max_num, unsigned int cluster1_max_num);
#else
#define power_mode_register_notifier(nb)	do { } while (0)
#define power_mode_unregister_notifier(nb)	do { } while (0)
#define power_mode_notifier_call_chain(val, v)	do { } while (0)
#define request_thermal_power_mode(a,b,c,d,e)	do { } while (0)
#endif

#endif /* POWER_MODE */

