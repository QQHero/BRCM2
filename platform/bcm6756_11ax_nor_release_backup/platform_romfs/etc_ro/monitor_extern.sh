#!/bin/sh

WAN_ID=`echo $1 | awk -Fwan '{printf $2}'`
cfm post multiWAN 17 $WAN_ID 0

