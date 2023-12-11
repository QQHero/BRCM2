#!/bin/sh
# utf-8 coding

# 删掉接口的全局IPV6地址
# llm

IFNAME=$1

if [[ -z ${IFNAME} ]] ; then
    echo "args err!"
    exit 0    
fi

# 获取全局地址列表
ADDR_LIST=`ifconfig ${IFNAME} | grep "Scope:Global" | awk '{print $3}'` 

# 逐条删除
for ADDR in ${ADDR_LIST}
do
    ip -6 addr del ${ADDR} dev ${IFNAME}
done
#删除全局地址对应的路由，一般来说在删除全局地址的时候内核会删除对应的路由，
#但是tmd隧道接入的时候eth1接口由ra报文生成的路由在删除地址时不会删除，坑爹啊
#由于在重启路由时accpet_ra标志位变为1,导致接收RA报文在wan口生成对应的路由，但是wan口全局地址对应的路由所对应的IP没有用到，导致ifconfig检测不到,就删不掉wan口对应的全局路由

for ADDR in ${ADDR_LIST}
do
    ip -6 route del ${ADDR} dev ${IFNAME}
done

#以下代码实现三个功能；第一个重启直接拨号删除pppoe和静态时wan口RA下发的全球路由和默认路由
#第二个功能实现删除多服务器拨号之间wan口RA报文下发的俩种路由
#免去静态拨号和DHCPv6拨号中disconnect函数中删除默认路由的操作，也就是实现IPv6开关关闭，默认路由删除
ADDR_LIST=`ip -6 route | grep "dev ${IFNAME}" | awk '{print $1}'`

if [[ "$IFNAME" != "br0" -a "$IFNAME" != "ppp1" ]] ; then
    for ADDR in ${ADDR_LIST}
    do
        ip -6 route del ${ADDR} dev ${IFNAME}
    done
fi