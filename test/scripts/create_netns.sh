#!/bin/sh

ip netns add vpn_client1
ip netns add vpn_client2

echo "Creating links for veth interfaces..."
ip link add client1-server type veth peer name server-client1
ip link add client2-server type veth peer name server-client2

echo "Putting links in corresponding netns..."
ip link set client1-server netns vpn_client1
ip link set client2-server netns vpn_client2

echo "Bringing interfaces up and setting up IP addresses..."
ip link set server-client1 up
ip a a 192.168.1.1/24 dev server-client1
ip netns exec vpn_client1 ip link set lo up
ip netns exec vpn_client1 ip link set client1-server up
ip netns exec vpn_client1 ip a a 192.168.1.2/24 dev client1-server

ip link set server-client2 up
ip a a 192.168.2.1/24 dev server-client2
ip netns exec vpn_client2 ip link set lo up
ip netns exec vpn_client2 ip link set client2-server up
ip netns exec vpn_client2 ip a a 192.168.2.2/24 dev client2-server

echo "Successfully configured namespaces: "
ip netns
