#!/bin/sh
user=$1
realm=$2
pass=$3
hash=`echo -n "$user:$realm:$pass" | md5sum | cut -b -32`
echo "$user:$realm:$hash"