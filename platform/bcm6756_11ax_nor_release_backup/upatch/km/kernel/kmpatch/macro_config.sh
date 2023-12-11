#!/bin/bash
##################################################################################
# km宏配置脚本，此脚本主要解决当新增km宏时，导致build下各个方案的def.linux.config
# 都要更改，并且demo以及使用最新release包的产品侧需要作同步修改的问题。
# 此脚本会读取km/Kconfig中的config配置宏，根据依赖和默认值设置默认配置，不需要在各
# 个方案手动添加，仅仅需要在当前需要开启的方案中打开即可。
##################################################################################
MODULE_KCONFIG_PATH=$1
KERNEL_CONFIG=$2
MODULE_KCONFIG_PATH_BACKUP=MODULE_KCONFIG_PATH"_backup"

# 避免Kconfig最后一行不为空行，导致最后一行读取不到
cp $MODULE_KCONFIG_PATH $MODULE_KCONFIG_PATH_BACKUP
echo "" >> $MODULE_KCONFIG_PATH_BACKUP
depends_flag=0
cat $MODULE_KCONFIG_PATH_BACKUP | while read LINE
do
    case=0
    # read LINE 读出每一行，for循环取出使用空格分割的每个字符串
    for i in $LINE
    do
        # case代表当前判断的配置选项
        if [ "$i" == "config" ] || [ "$i" == "menuconfig" ]; then
            case=1
            km_macro=""
            depends_macro=""
            continue
        elif [ "$i" ==  "depends" ]; then
            case=2
            continue
        elif [ "$i" == "default" ]; then
            case=3
            continue
        fi

        if [ $case -eq 1 ]; then
            km_macro="CONFIG_"$i
        elif [ $case -eq 2 ]; then
            # 多个依赖时使用"&&" "||" 连接，km/Kconfig比较简单，复杂的&&和()并未实现
            if [ $i != "on" ] && [ $i != "&&" ] && [ $i != "||" ]; then
                depends_macro="CONFIG_"${i}"=y"
                search_result=`grep -w $depends_macro $KERNEL_CONFIG`
                if [ $? -eq 0 ]; then
                    depends_flag=1
                fi
            fi
        elif [ $case -eq 3 ]; then
            search_result=`grep -w $km_macro $KERNEL_CONFIG`
            # 如果已经在.config被设置，就不用再次设置了
            if [ $? -ne 0 ]; then
                # 判断依赖，若依赖未设置，此处不会设置到.config
                if [ $depends_flag -eq 1 ] || [ ${km_macro} == "CONFIG_TENDA_PRIVATE" ]; then
                    if [ "$i" == "n" ]; then
                        macro_value="# ${km_macro} is not set"
                    else
                        macro_value=${km_macro}=${i}
                    fi
                    echo "macro_config.sh set '$macro_value' to .config"
                    # 所有通过km_macro_config生成的配置文件都设置到.config
                    # 最后一行在内核编译时，它会自动排列到对应位置
                    echo $macro_value >> $KERNEL_CONFIG
                fi
            fi
            depends_flag=0
        fi
    done
done
# 删除备份
rm $MODULE_KCONFIG_PATH_BACKUP -f
