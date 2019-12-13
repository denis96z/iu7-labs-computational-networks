iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT

iptables -F

iptables -N KNOCK
iptables -N KNOCKING
iptables -N KNOCK_PASS
iptables -A INPUT -p icmp -j ACCEPT
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -i lo -j ACCEPT
iptables -A INPUT -j KNOCK
iptables -A KNOCKING -p tcp --dport 27 -m recent --name K_OK --set -j DROP
iptables -A KNOCKING -j DROP
iptables -A KNOCK_PASS -m recent --name K_OK --remove
iptables -A KNOCK_PASS -p tcp --dport 22 -j ACCEPT
iptables -A KNOCK_PASS -j KNOCKING
iptables -A KNOCK -m recent --rcheck --seconds 60 --name K_OK -j KNOCK_PASS
iptables -A KNOCK -j KNOCKING
