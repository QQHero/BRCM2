#!/bin/sh

:<<!
# 可使用的编译命令:
sh multi_make.sh W20E V30
sh multi_make.sh W60E V60
sh multi_make.sh G3v3 G3v3_en M50v2 M50v2_en
sh multi_make.sh W15E W15Ev1_en V20
sh multi_make.sh M80v1 G6v1
!

function make_other_target()
{
	ROMFS_PATH=${TOP_PATH}"/targets/"${1}"/romfs";


	# 拷贝共用的部分
	rm -rf targets/${1}/romfs
	cp -Rf targets/${2}/romfs   targets/${1}/romfs
	cp targets/${2}/bin/vmlinuz  targets/${1}/bin/vmlinuz
	cp targets/${2}/bin/vmlinux  targets/${1}/bin/vmlinux
	cp targets/${2}/bin/zImage   targets/${1}/bin/zImage

	# fw_version.h
	rm -f ${TOP_PATH}/fw_version.h ;
	ln -s ${TOP_PATH}/targets/${1}/product_version.h  ${TOP_PATH}/fw_version.h

	# 版本号
	make check_svn TARGETS_FILE=${1}

	# 重新编译 httpd
	make prod/httpd/httpd/_clean TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	make prod/httpd/httpd/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH} 

	# 重新编译 ate 
	make prod/ate/_clean TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	make prod/ate/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH} 

	# 重新编译 在线升级
	make prod/ucloud/_clean TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	make prod/ucloud/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH} 

	# 重新编译 netctrl
	make prod/agent/netctrl/_clean TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	make prod/agent/netctrl/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}

	# 重新编译 ac管理
	if [[ CONFIG_AP_MANAGE=y == $( grep CONFIG_AP_MANAGE targets/${1}/config.mk) ]]; then
		make  prod/ac_wtp/_clean TARGETS_FILE=${1}
		make  prod/ac_wtp/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	fi

	# 重新编译 web
	make prod/httpd/web/_clean TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
	make prod/httpd/web/_only TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}

	# 重新编译 image
	make image TARGETS_FILE=${1} ROMFS_PATH=${ROMFS_PATH}
}


# 如果没有参数
if [ $# -eq 0 ]
then
	exit 1
fi

# 项目的绝对路径
TOP_PATH=$(pwd)

# 编译第一个目标
make tsfile=$1
echo -e "\033[32m \033[05m Make $1 done \033[m \033[0m"

# 依次编译后续目标
for target in $*
do
	if [ ${target} != $1 ]
	then
		# 目标不存在,跳过
		if [ ! -d ${TOP_PATH}"/targets/"${target} ]
		then
			echo ${TOP_PATH}"/targets/"${target} not exist
		else
			# 编译差异的部分
			make_other_target ${target} $1
			echo -e "\033[32m \033[05m Make ${target} done \033[m \033[0m"
		fi
	fi
done
