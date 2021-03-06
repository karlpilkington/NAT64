#include "nat64/xt_core.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <net/ipv6.h>
#include <net/netfilter/nf_conntrack.h>

#include "nat64/nat64.h"
#include "nat64/ipv6_hdr_iterator.h"
#include "nat64/pool4.h"
#include "nat64/pool6.h"
#include "nat64/bib.h"
#include "nat64/session.h"
#include "nat64/config.h"
#include "nat64/determine_incoming_tuple.h"
#include "nat64/filtering_and_updating.h"
#include "nat64/compute_outgoing_tuple.h"
#include "nat64/translate_packet.h"
#include "nat64/handling_hairpinning.h"
#include "nat64/send_packet.h"


unsigned int nat64_core(struct sk_buff *skb_in,
		bool (*compute_out_tuple_fn)(struct nf_conntrack_tuple *in, struct sk_buff *skb_in,
				struct nf_conntrack_tuple *out),
		bool (*translate_packet_fn)(struct nf_conntrack_tuple *, struct sk_buff *,
				struct sk_buff **),
		bool (*send_packet_fn)(struct sk_buff *))
{
	struct sk_buff *skb_out = NULL;
	struct nf_conntrack_tuple *tuple_in = NULL, tuple_out;

	if (!determine_in_tuple(skb_in, &tuple_in))
		goto free_and_fail;
	if (filtering_and_updating(skb_in, tuple_in) != NF_ACCEPT)
		goto free_and_fail;
	if (!compute_out_tuple_fn(tuple_in, skb_in, &tuple_out))
		goto free_and_fail;
	if (!translate_packet_fn(&tuple_out, skb_in, &skb_out))
		goto free_and_fail;
	if (is_hairpin(&tuple_out)) {
		if (!handling_hairpinning(skb_out, &tuple_out))
			goto free_and_fail;
	} else {
		if (!send_packet_fn(skb_out))
			goto fail;
	}

	log_debug("Success.");
	return NF_DROP;

free_and_fail:
	kfree_skb(skb_out);
	// Fall through.

fail:
	log_debug("Failure.");
	return NF_DROP;
}

unsigned int nat64_tg4(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct iphdr *ip4_header = ip_hdr(skb);
	__u8 l4protocol = ip4_header->protocol;
	struct in_addr daddr;

	// Validate.
	daddr.s_addr = ip4_header->daddr;
	if (!pool4_contains(&daddr))
		return NF_ACCEPT; // Let something else handle it.

	log_debug("===============================================");
	log_debug("Catching IPv4 packet: %pI4->%pI4", &ip4_header->saddr, &ip4_header->daddr);

	// TODO (test) validate l4 headers further?
	if (l4protocol != IPPROTO_TCP && l4protocol != IPPROTO_UDP && l4protocol != IPPROTO_ICMP) {
		log_debug("Packet does not use TCP, UDP or ICMP.");
		return NF_ACCEPT;
	}

	// Set the skb's transport header pointer.
	// It's yet to be set because the packet hasn't reached the kernel's transport layer.
	// And despite that, its availability will be appreciated.
	skb_set_transport_header(skb, 4 * ip4_header->ihl);

	return nat64_core(skb,
			compute_out_tuple_4to6,
			translating_the_packet_4to6,
			send_packet_ipv6);
}

unsigned int nat64_tg6(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct ipv6hdr *ip6_header = ipv6_hdr(skb);
	struct hdr_iterator iterator = HDR_ITERATOR_INIT(ip6_header);
	enum hdr_iterator_result iterator_result;
	__u8 l4protocol;

	// Validate.
	if (!pool6_contains(&ip6_header->daddr))
		goto failure;

	log_debug("===============================================");
	log_debug("Catching IPv6 packet: %pI6c->%pI6c", &ip6_header->saddr, &ip6_header->daddr);

	iterator_result = hdr_iterator_last(&iterator);
	switch (iterator_result) {
	case HDR_ITERATOR_SUCCESS:
		log_crit(ERR_ITERATOR_IS_LYING, "Iterator reports there are headers beyond the payload.");
		goto failure;
	case HDR_ITERATOR_END:
		l4protocol = iterator.hdr_type;
		break;
	case HDR_ITERATOR_UNSUPPORTED:
		// RFC 6146 section 5.1.
		log_info("Packet contains an Authentication or ESP header, which I do not support.");
		goto failure;
	case HDR_ITERATOR_OVERFLOW:
		log_warning("IPv6 extension header analysis ran past the end of the packet. "
				"Packet seems corrupted; ignoring.");
		goto failure;
	default:
		log_crit(ERR_UNKNOWN_RCODE, "Unknown header iterator result code: %d.", iterator_result);
		goto failure;
	}

	switch (l4protocol) {
	case NEXTHDR_TCP:
		if (iterator.data + tcp_hdrlen(skb) > iterator.limit) {
			log_warning("TCP header doesn't fit in the packet. Packet seems corrupted; ignoring.");
			goto failure;
		}
		break;

	case NEXTHDR_UDP: {
		struct udphdr *hdr = iterator.data;
		if (iterator.data + sizeof(struct udphdr) > iterator.limit) {
			log_warning("UDP header doesn't fit in the packet. Packet seems corrupted; ignoring.");
			goto failure;
		}
		if (iterator.data + be16_to_cpu(hdr->len) > iterator.limit) {
			log_warning("UDP header + payload do not fit in the packet. "
					"Packet seems corrupted; ignoring.");
			goto failure;
		}
		break;
	}

	case NEXTHDR_ICMP: {
		struct icmp6hdr *hdr = iterator.data;
		if (iterator.data + sizeof(*hdr) > iterator.limit) {
			log_warning("ICMP header doesn't fit in the packet. Packet seems corrupted; ignoring.");
			goto failure;
		}
		break;
	}

	default:
		log_info("Packet does not use TCP, UDP or ICMPv6.");
		goto failure;
	}

	// Set the skb's transport header pointer.
	// It's yet to be set because the packet hasn't reached the kernel's transport layer.
	// And despite that, its availability will be appreciated.
	skb_set_transport_header(skb, iterator.data - (void *) ip6_header);

	return nat64_core(skb,
			compute_out_tuple_6to4,
			translating_the_packet_6to4,
			send_packet_ipv4);

failure:
	return NF_ACCEPT;
}

int nat64_tg_check(const struct xt_tgchk_param *par)
{
//	int ret = nf_ct_l3proto_try_module_get(par->family);
//	if (ret < 0)
//		log_info("cannot load support for proto=%u", par->family);
//	return ret;
//
//	log_info("Check function.");
	return 0;
}

static struct xt_target nat64_tg_reg[] __read_mostly = {
	{
		.name = MODULE_NAME,
		.revision = 0,
		.family = NFPROTO_IPV4,
		.table = "mangle",
		.target = nat64_tg4,
		.checkentry = nat64_tg_check,
		.hooks = (1 << NF_INET_PRE_ROUTING),
		.me = THIS_MODULE,
	},
	{
		.name = MODULE_NAME,
		.revision = 0,
		.family = NFPROTO_IPV6,
		.table = "mangle",
		.target = nat64_tg6,
		.checkentry = nat64_tg_check,
		.hooks = (1 << NF_INET_PRE_ROUTING),
		.me = THIS_MODULE,
	}
};

int __init nat64_init(void)
{
	int result;

	log_debug("%s", banner);
	log_debug("Inserting the module...");

	need_conntrack();
	need_ipv4_conntrack();

	if (!(config_init()
			&& pool6_init() && pool4_init(true)
			&& bib_init() && session_init()
			&& determine_in_tuple_init()
			&& filtering_init()
			&& translate_packet_init()))
		return false;

	result = xt_register_targets(nat64_tg_reg, ARRAY_SIZE(nat64_tg_reg));
	if (result == 0)
		log_debug("Ok, success.");
	return result;
}

void __exit nat64_exit(void)
{
	xt_unregister_targets(nat64_tg_reg, ARRAY_SIZE(nat64_tg_reg));

	translate_packet_destroy();
	filtering_destroy();
	determine_in_tuple_destroy();
	session_destroy();
	bib_destroy();
	pool4_destroy();
	pool6_destroy();
	config_destroy();

	log_debug("NAT64 module removed.");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NIC-ITESM");
MODULE_DESCRIPTION("\"NAT64\" (RFC 6146)");
MODULE_ALIAS("ipt_nat64");
MODULE_ALIAS("ip6t_nat64");

module_init(nat64_init);
module_exit(nat64_exit);
