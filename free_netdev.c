#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/netdevice.h>

#define _DECL_CMN_JRP(fn, symbol) static struct jprobe fn##_jp = { \
    .entry              = on_##fn##_ent,                           \
    .kp.symbol_name     = ""#symbol"",                             \
};

#define DECL_CMN_JRP(fn) _DECL_CMN_JRP(fn, fn)

#define _DECL_CMN_KRP(fn, symbol, cond) _DECL_CMN_KRP_##cond(fn, symbol)

#define _DECL_CMN_KRP_0(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = NULL,                                          \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = 0,                                             \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define _DECL_CMN_KRP_1(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = on_##fn##_ent,                                 \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = sizeof(struct fn##_args),                      \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define _DECL_CMN_KRP_2(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = on_##fn##_ent,                                 \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = 0,                                             \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define DECL_CMN_KRP(fn, cond) _DECL_CMN_KRP(fn, fn, cond)

#define WITHOUT_ENTEY        0
#define WITH_ENTEY           1
#define WITH_NODATA_ENTEY    2

struct per_cpu_net_data {
	const struct net_device *dev;
};

static DEFINE_PER_CPU(struct per_cpu_net_data, per_cpu_net);

static int on_netdev_refcnt_read_ent(const struct net_device *dev)
{
        struct per_cpu_net_data *data;

        data = &__get_cpu_var(per_cpu_net);
        if (data != NULL) {
                data->dev = dev;
		pr_warn("in ent dev->name: %s\n", dev->name);
        }

        jprobe_return();

        return 0;
}

DECL_CMN_JRP(netdev_refcnt_read);

static bool fixed = false;

static int on_netdev_refcnt_read_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        struct per_cpu_net_data *data;
	const struct net_device *dev;
	int i, ret, refcnt = 0;

	ret = (int)regs_return_value(regs);
	if (ret != 0 && !fixed) {
        	data = &__get_cpu_var(per_cpu_net);
		dev = data->dev;	
		pr_warn("in ret dev->name: %s\n", dev->name);
		if (strcmp(dev->name, "eth0") == 0) {
			for_each_possible_cpu(i) {
				refcnt = *per_cpu_ptr(dev->pcpu_refcnt, i);
				if (refcnt != 0) 
					*per_cpu_ptr(dev->pcpu_refcnt, i) = 0;
			}

			regs->ax = 0;
			
			fixed = true;
		}
	}

	return 0;
}

DECL_CMN_KRP(netdev_refcnt_read, WITHOUT_ENTEY);

static struct jprobe *fix_jps[] = {
    &netdev_refcnt_read_jp,
};

static struct kretprobe *fix_krps[] = {
    &netdev_refcnt_read_krp,
};

static int __init fix_events_init(void)
{
        int ret = 0;

        ret = register_jprobes(fix_jps,
                sizeof(fix_jps) / sizeof(fix_jps[0]));
        if(ret) {
                pr_err("Register fix jprobes failed\n");
                return ret;
        }

        ret = register_kretprobes(fix_krps,
                sizeof(fix_krps) / sizeof(fix_krps[0]));
        if(ret) {
                pr_err("Register fix kretprobes failed, %d\n", ret);
                return ret;
        }

        return 0;
}

static void __exit fix_events_exit(void)
{
        unregister_jprobes(fix_jps,
                sizeof(fix_jps) / sizeof(fix_jps[0]));
        unregister_kretprobes(fix_krps,
                sizeof(fix_krps) / sizeof(fix_krps[0]));
}

MODULE_AUTHOR("Zwb <ethercflow@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(fix_events_init);
module_exit(fix_events_exit);
