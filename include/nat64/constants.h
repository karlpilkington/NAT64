#ifndef _NF_NAT64_CONSTANTS_H
#define _NF_NAT64_CONSTANTS_H

/**
 * @file
 * General purpose #defines, intended to minimize use of numerical constants elsewhere in the code.
 *
 * @author Roberto Aceves
 * @author Ramiro Nava
 * @author Miguel Gonzalez
 * @author Alberto Leiva
 */


// -- Timeouts, defined by RFC 6146, section 4. --

/**
 * Minimum allowable session lifetime for UDP bindings, in seconds.
 */
#define UDP_MIN (2 * 60)
/**
 * Defined in the RFC as the minimum allowable default value for the session lifetime of UDP bindings,
 * in seconds. We use it as the actual default value.
 */
#define UDP_DEFAULT (5 * 60)
/** Transitory connection idle timeout.
 *  In other words, the timeout of several states in the TCP state machine. In seconds.
 */
#define TCP_TRANS (4 * 60)
/** Established connection idle timeout.
 *  In other words, the timeout of several states in the TCP state machine. In seconds.
 */
#define TCP_EST (2 * 60 * 60)
/** Timeout of several types of new STEs created during the CLOSED state of the TCP state machine. */
#define TCP_INCOMING_SYN (6)
// /** Default time interval fragments are allowed to arrive in. In seconds. */
// #define FRAGMENT_MIN (2)
/** Default session lifetime for ICMP bindings, in seconds. */
#define ICMP_DEFAULT (1 * 60)


// -- Config defaults --
#define POOL6_DEF_PREFIX		"64:ff9b::"
#define POOL6_DEF_PREFIX_LEN	96

#define POOL4_DEF { "192.168.2.1", "192.168.2.2", "192.168.2.3", "192.168.2.4" }

#define FILT_DEF_ADDR_DEPENDENT_FILTERING false
#define FILT_DEF_FILTER_ICMPV6_INFO false
#define FILT_DEF_DROP_EXTERNAL_CONNECTIONS true

#define TRAN_DEF_USR_HEAD_ROOM 0
#define TRAN_DEF_USR_TAIL_ROOM 32
#define TRAN_DEF_OVERRIDE_IPV6_TRAFFIC_CLASS false
#define TRAN_DEF_OVERRIDE_IPV4_TRAFFIC_CLASS false
#define TRAN_DEF_TRAFFIC_CLASS 0
#define TRAN_DEF_DF_ALWAYS_SET true
#define TRAN_DEF_GENERATE_IPV4_ID false
#define TRAN_DEF_IMPROVE_MTU_FAILURE_RATE true
#define TRAN_DEF_IPV6_NEXTHOP_MTU 1280
#define TRAN_DEF_IPV4_NEXTHOP_MTU 576
#define TRAN_DEF_MTU_PLATEAUS { 65535, 32000, 17914, 8166, 4352, 2002, 1492, 1006, 508, 296, 68 }


// IPv6 Pool
#define POOL6_PREFIX_LENGTHS { 32, 40, 48, 56, 64, 96 }


// -- Session --
#define SESSION_TIMER_INTERVAL (10 * 1000)


// -- ICMP constants missing from icmp.h and icmpv6.h. --

/** Code 0 for ICMP messages of type ICMP_PARAMETERPROB. */
#define ICMP_PTR_INDICATES_ERROR 0
/** Code 2 for ICMP messages of type ICMP_PARAMETERPROB. */
#define ICMP_BAD_LENGTH 2


/* ICMP error messaging */
//      Types:
#define DESTINATION_UNREACHABLE ICMP_DEST_UNREACH 
//      Codes:
#define HOST_UNREACHABLE        ICMP_HOST_UNREACH
#define ADDRESS_UNREACHABLE     ICMP_HOST_UNREACH
#define COMMUNICATION_ADMINISTRATIVELY_PROHIBITED   ICMP_PKT_FILTERED



#endif
