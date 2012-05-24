IFCONFIG=/sbin/ifconfig
GREP=/bin/grep

if ($IFCONFIG | $GREP -q "eth1") then 
	exit 1
else 
	exit 0
fi
