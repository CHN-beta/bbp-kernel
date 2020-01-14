#include "common/common.h"

MODULE_AUTHOR("Haonan Chen");
MODULE_DESCRIPTION("Modify UA in HTTP for anti-detection of router in XMU.");
MODULE_LICENSE("GPL");

static struct nf_hook_ops nfho[3];		// 需要在 INPUT、OUTPUT、FORWARD 各挂一个
static struct bbpWorker* worker[3];

extern struct bbpWorker* creatWorker_ua(void);
extern struct bbpWorker* creatWorker_win(void);
extern struct bbpWorker* creatWorker_id(void);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,2,0)
unsigned hook_funcion(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
#else
unsigned hook_funcion(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
#endif
{
	unsigned i;
	for(i = 0; i < 3; i++)
	{
		unsigned rtn;
		rtn = worker[i] -> execute(worker[i], skb);
		if(rtn == NF_ACCEPT)
			continue;
		else if(rtn == NF_STOLEN)
			return NF_STOLEN;
		else if(rtn == NF_DROP)
			return NF_DROP;
	}
	return NF_ACCEPT;
}

unsigned goon(struct bbpWorker* bbpw, struct sk_buff* skb)
{
	unsigned p, i;
	for(i = 0; i < 3; i++)
		if(worker[i] == bbpw)
			p = i;
	for(i = p + 1; i < 3; i++)
	{
		unsigned rtn;
		rtn = worker[i] -> execute(worker[i], skb);
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

	worker[0] = creatWorker_ua();
	worker[1] = creatWorker_win();
	worker[2] = creatWorker_id();
	for(i = 0; i < 3; i++)
		worker[i] -> goon = goon;

	nfho[0].hooknum = NF_INET_LOCAL_IN;
	nfho[1].hooknum = NF_INET_LOCAL_OUT;
	nfho[2].hooknum = NF_INET_FORWARD;
	for(i = 0; i < 3; i++)
	{
		nfho[i].hook = hook_funcion;
		nfho[i].pf = NFPROTO_IPV4;
		nfho[i].priority = NF_IP_PRI_MANGLE + 1;
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
}

module_init(hook_init);
module_exit(hook_exit);
