IFCONFIG=/sbin/ifconfig
GREP=/bin/grep

if ($IFCONFIG eth1 | $GREP -q "inet addr") then 
	exit 1
else 
	exit 0
fi
