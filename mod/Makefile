MODULES_DIR := /lib/modules/$(shell uname -r)
KERNEL_DIR := ${MODULES_DIR}/build
EXTRA_CFLAGS += -DDEBUG

ccflags-y := -I$(src)/../include
#ccflags-y += -I$(src)/../mod


obj-m += nat64.o

nat64-objs += types.o
nat64-objs += ipv6_hdr_iterator.o
nat64-objs += rfc6052.o
nat64-objs += pool6.o
nat64-objs += pool4.o
nat64-objs += bib.o
nat64-objs += session.o
nat64-objs += static_routes.o
nat64-objs += config.o
nat64-objs += config_validation.o
nat64-objs += config_proto.o
nat64-objs += determine_incoming_tuple.o
nat64-objs += filtering_and_updating.o
nat64-objs += compute_outgoing_tuple.o
nat64-objs += translate_packet.o
nat64-objs += handling_hairpinning.o
nat64-objs += send_packet.o
nat64-objs += xt_core.o


all:
	make -C ${KERNEL_DIR} M=$$PWD;
insert:
	# insert the dependencies
	sudo modprobe ipv6
	sudo modprobe ip_tables
	sudo modprobe nf_conntrack
	sudo modprobe nf_conntrack_ipv4
	sudo modprobe nf_conntrack_ipv6
	# enable ipv6 and ipv4 forwarding
	sudo sysctl -w net.ipv4.conf.all.forwarding=1
	sudo sysctl -w net.ipv6.conf.all.forwarding=1
	# prevent martian packets from reaching the module
	sudo sysctl -w net.ipv4.conf.all.log_martians=1
	# insert the module
	sudo insmod nat64.ko
	dmesg | tail -15
	# insert the rules
	sudo iptables -t mangle --flush
	sudo ip6tables -t mangle --flush
	sudo iptables -t mangle -A PREROUTING -j NAT64
	sudo ip6tables -t mangle -A PREROUTING -j NAT64
remove:
	# remove the rules
	sudo iptables -t mangle --flush
	sudo ip6tables -t mangle --flush
	# remove the module
	sudo rmmod nat64
	dmesg | tail -15
modules:
	make -C ${KERNEL_DIR} M=$$PWD $@;
modules_install:
	make -C ${KERNEL_DIR} M=$$PWD $@;
clean:
	make -C ${KERNEL_DIR} M=$$PWD $@;
