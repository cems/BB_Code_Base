IFCONFIG=/sbin/ifconfig
IWCONFIG=/sbin/iwconfig
GREP=/bin/grep

if ($IWCONFIG eth1 | $GREP -q "Access Point: Not-Associated") then
	exit 0
else 
	exit 1
fi
