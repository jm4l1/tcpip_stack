# tcpip_stack
TCPI/IP network stack simulator implemented in C. The simulator implements Basic Layer 2 (MAC Addresses, Arp) and Layer 3 (Routing , IPs) functionality.

## Supported Operated Systems
Implementation works with Linux and MacOS

## Dependencies for Running Locally
* make >= 4.1 (Linux, Mac)

## Basic Build Instructions

1. Clone this repo.
2. Make a build directory in the top level directory: `mkdir build && cd build`
3. Compile: `make`
4. Run it: `./test`.

## Running the application
```bash
$./test
root@juniper> $ show topo saved 1
1 - Linear

    +-----------+                        +-----------+                         +-----------+                         +-----------+
     |           |0/1                  0/2|           |0/3                   0/4|           |0/3                   0/2|           |
     +     R0    +------------------------+     R1    +-------------------------+     R2    +-------------------------+     R3    +
     |           |10.1.1.1/24  10.1.1.2/24|           |20.1.1.2/24   20.1.1.1/24|           |30.1.1.1/24   30.1.1.2/24|           |
     +-----------+                        +-----------+                         +-----------+                         +-----------+


root@juniper> $ config topo load 1
network receiver thread started for topology "Linear Topology"
Topology Loaded.
root@juniper> $ show node R0 interface
Interface Name : eth0/1 (IP Routed)
 	Nbr Node R1, Local Node : eth0/1 , cost = 1
	IP Addr : 10.1.1.1(10.1.1.0/24 )  MAC : 62:86:8e:21:80:40

root@juniper> $  run node R1 ping 10.1.1.2
[send_ping_request] Sending 1 packets to 10.1.1.2
Comparing lo 2.2.2.2 to 10.1.1.2
Comparing lo 2.2.2.2 to 10.1.1.2
Comparing lo 2.2.2.2 to 10.1.1.2
Comparing lo 2.2.2.2 to 10.1.1.2
Node R1 : 12 bytes received from : icmp_seq=0 ttl=64
```

