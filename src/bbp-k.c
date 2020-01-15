#include "common/common.h"

MODULE_AUTHOR("Haonan Chen");
MODULE_DESCRIPTION("Modify UA in HTTP for anti-detection of router in XMU.");
MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho[3];				// 需要在 INPUT、OUTPUT、FORWARD 各挂一个
static struct bbpWorker* worker[16];			// 多开几个，不费几个内存

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
unsigned hook_funcion(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
#else
unsigned hook_funcion(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
#endif
{
	unsigned i;
	bool status[3];
	memset(status, 0, sizeof(bool) * 3);
	for(i = 0; worker[i] != 0; i++)
	{
		unsigned rtn;
		rtn = worker[i] -> execute(worker[i], skb, status);
		if(rtn == NF_ACCEPT)
			continue;
		else if(rtn == NF_STOLEN)
			return NF_STOLEN;
		else if(rtn == NF_DROP)
			return NF_DROP;
	}
	if(status[2])
		bbpCommon_csumTcp(skb);
	else if(status[1])
		bbpCommon_csumIp(skb);
	return NF_ACCEPT;
}

static unsigned goon(struct bbpWorker* bbpw, struct sk_buff* skb, bool* status)
{
	unsigned p = -1, i;
	for(i = 0; worker[i] != 0; i++)
		if(worker[i] == bbpw)
			p = i;
	if(p != -1)
		for(i = p + 1; worker[i] != 0; i++)
		{
			unsigned rtn;
			rtn = worker[i] -> execute(worker[i], skb, status);
			if(rtn == NF_ACCEPT)
				continue;
			else if(rtn == NF_STOLEN)
				return NF_STOLEN;
			else if(rtn == NF_DROP)
				return NF_DROP;
		}
	return NF_ACCEPT;
} 

static int __init hook_init(void)
{
	int ret;
	unsigned i;
	extern bbpWorkerCreator workerCreator_ua, workerCreator_win, workerCreator_id;
	const bbpWorkerCreator creator[] = {workerCreator_ua, workerCreator_win, workerCreator_id, 0};
	const unsigned hooknum[] = {NF_INET_LOCAL_IN, NF_INET_LOCAL_OUT, NF_INET_FORWARD};

	bbpSetting_common_init();

	memset(worker, 0, sizeof(struct bbpWorker*) * 16);
	for(i = 0; creator[i] != 0; i++)
		worker[i] = creator[i](goon);

	for(i = 0; i < 3; i++)
	{
		nfho[i].hook = hook_funcion;
		nfho[i].pf = NFPROTO_IPV4;
		nfho[i].priority = NF_IP_PRI_MANGLE + 1;
		nfho[i].hooknum = hooknum[i];
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	ret = nf_register_net_hooks(&init_net, nfho, 3);
#else
	ret = nf_register_hooks(nfho, 3);
#endif

	return 0;
}

//卸载模块
static void __exit hook_exit(void)
{
	unsigned i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	nf_unregister_net_hooks(&init_net, nfho, 3);
#else
	nf_unregister_hooks(nfho, 3);
#endif

	for(i = 0; worker[i] != 0; i++)
		worker[i] -> delete(worker[i]);
}

module_init(hook_init);
module_exit(hook_exit);
