#!/bin/bash

#_PP_DEBUG=1
function _dpp() {
    if [[ -n $_PP_DEBUG  && $_PP_DEBUG -eq 1 ]]; then
        echo ${@}
    fi
}

function get_files_inpatch() {
    local rbfile=$1    
    local files=($(awk '{ if (match($1,"\\+\\+\\+")) { print $2 }}' $rbfile))
    echo "${files[@]}"
}

function find_file_in_dir() {
    local impl_dir=$(realpath -s $1)
    local rbfile=$2
    local file_name=$(basename $rbfile)
    found=($(find $impl_dir -type f -name $file_name))
    #if find more than one,contineu up to find uniqe dir    
    if [[  ${#found[@]} -gt 1 ]]; then
        local file_parts=($(echo $(dirname $rbfile)|tr "/" " "))
        for i in $(seq ${#file_parts[@]} -1 1)
        do
            local part=${file_parts[$((i-1))]}
            found=($(find $impl_dir -type d -name $part))
            if [[ ${#found[@]} -eq 1  && ${found[@]} != "." ]];then
                found=$(find ${found[0]} -type f -name $file_name)
                break;
            fi
        done 
    fi
    #if still did not found, need input
    if [[ -z $found ]]; then
        local founds=($(find $impl_dir -type d -name ${file_parts[-1]}))
        echo "$rbfile is in:"
        local item=0
        for part in ${founds[@]}
        do
            echo "$item:$part"
            item=$((item+1))
        done
        echo "which one to patch:"
        read select
        if [[ ! -z $select ]]; then
            found=$(find ${founds[$select]} -type f -name $file_name)
        else
            echo "no select"
        fi
    fi
    g_found_result="$found"
    echo $found
}

function patch_files_d2u_convert() {

    local impl_dir=$(realpath -s $1)
    local patchfile=$(realpath $2)
    local files=($(get_files_inpatch $patchfile $files))
    if [[ ! -z $files ]]; then
        for fl in ${files[@]}
        do
            find_file_in_dir $impl_dir $fl >/dev/null
            local result="$g_found_result"
            if [[ ! -z $result ]]; then
                dos2unix -q $result 
            fi
        done
    fi
}
# return patch package's last bsp and wifi version number in file wifi_patch_version.txt
# Parameters:
#    in: top_dir
#    in: wlimpl "impl55"
#    out[optional]: last bsp number
#    out[optional]: last wifi numberh

function get_pp_last_ver() {
    if [[ $# -gt 1 ]]; then
        local top_dir=$1
        local wlimpl=$(basename $2)
        local wifi_version=""
        if [ $wlimpl == "impl00" ]; then
            wifi_version="00"
        else
            wlimpl_get_ver $top_dir/bcmdrivers/broadcom/net/wl/$wlimpl  wifi_version
        fi
        if [[ -f $top_dir/wifi_patch_version.$wifi_version ]]; then
            local src_ver="$(cat $top_dir/wifi_patch_version.$wifi_version)"
            local src_last=($(echo $src_ver|sed 's/.*PvB\([0-9_c]\+\)W\([0-9c]\+\).*/\1 \2/'))
            if [ ${#src_last[@]} -eq 2 ];then
                if [[ ! -z $3 ]]; then
                    local tmp=${src_last[0]}
                    tmp=(${tmp/c/ } 0)
                    eval "${3}=(${tmp[@]})"
                    tmp=${src_last[1]}
                    tmp=(${tmp/c/ } 0)
                    eval "${4}=(${tmp[@]})"
                    return 0;
                else
                    echo "${src_last[0]} ${src_last[1]}"
                fi
            fi
        fi
        if [[ ! -z $3 ]]; then
            eval "${3}=(-1 0)"
            eval "${4}=(-1 0)"
        else
            echo "-1 0 -1 0"
        fi
    else
        echo "get_pp_last_ver topdir wlimpl last_bsp_num[out] last_wifi_num[out]"
    fi
}

#return a patch files version number,patches all in 0xxx-0xxx-yyy.patch
#Parameters:
#   in:patch name
#   out:array of patch_number[at most two]

function get_patch_number() {
    local patch_name="$1"
    patch_name=$(basename $patch_name)
    #get second number first
    local nums=($(echo $patch_name |sed -n "s/^0*\([0-9]\+\)-0*\([1-9][0-9]*\)[_-].*/\1 \2/p"))
    if [ ${#nums[@]} -eq 2 ]; then
        echo $(seq ${nums[0]} 1 ${nums[1]})
    else
        num=$(echo $patch_name |sed -n "s/^0*\([1-9][0-9]*\)[_-].*/\1/p")
        if [[ ! -z $2 ]]; then
            eval "${2}=($num)"
        fi
        echo "$num"
    fi
}

#return an array to dindicate wifi release version and it is release/internal source
#Parameter:
#    in: wlimpl_dir
#   out: (relver dirwithpatch wifi_source_ver wlimpl)
#        (17_10_188 internal|release KUDU_REL_xxxx impl81)

function wlimpl_get_ver() {
    if [[ $# -gt 1 ]]; then
        local wlimpl_dir=$(realpath $1)
        if [[ -e $wlimpl_dir/main ]]; then
            local epivers_hdr=$(find $wlimpl_dir/main -name "epivers.h")
            local release_ver=""
            local wifi_source_ver=""
            local patch_dir="internal"
            if [  -z "$epivers_hdr" ];then
                epivers_hdr=$(find $wlimpl_dir/main -name "epivers.sh")
                if [ ! -z "$epivers_hdr" ];then
                    local release_ver_line="$(awk '/^\s*SVNURL.*KUDU((_REL_)|(_BRANCH_)|(_TWIG_)).*\/src.*/{print}' $epivers_hdr)"
                    release_ver="$(echo $release_ver_line |sed 's/.*KUDU_[TRB][^0-9]*_\([0-9_]*_[0-9]\+\).*/\1/')"
                    wifi_source_ver="$(echo $release_ver_line |sed 's/.*\(KUDU_[TRB][^0-9]*_[0-9_]*_[0-9]\+\).*/\1/')"
                fi
            else
                local release_ver=$(awk '/.*EPI_VERSION_STR/{print $3; exit}' $epivers_hdr)
                if [ ! -z $release_ver ];then
                    release_ver=${release_ver//./_}
                    release_ver=${release_ver//\"/}
                fi
                patch_dir="release"
                wifi_source_ver="KUDU_REL_${release_ver}"
            fi
            #echo "$release_ver" "$patch_dir"
            eval "${2}=($release_ver $patch_dir $wifi_source_ver)"
        fi
    else
        echo "wlimpl_get_ver wlimpldir verresult"
    fi
}

# return all the patch numbers in a directory
function get_dir_patch_numbers() {
    local patch_dir=$1
    local patch_type=$2
    if [[ -d $patch_dir ]]; then
        local patches=($(find $patch_dir -maxdepth 1 -type f -regex "$patch_type"|sort))
        for patch in ${patches[@]}
        do
            patch_numbers+=($(get_patch_number "$patch"))
        done
        if [[ -n $patch_numbers ]]; then
            if [[ -n $3 ]]; then
                eval "$3=(${patch_numbers[@]})"
            else
                echo ${patch_numbers[@]}
            fi
        fi
    fi
}


function wl_update_patch_ver_str() {

    local patch_version=$(printf "PvB%s" ${g_bsp_patch_result["lastpatch"]})
    if [[ $g_bsp_src_last -ne ${g_bsp_patch_result["lastpatch"]}  &&  $g_bsp_src_last -ge 0 ]]; then
        patch_version+="*$g_bsp_src_last"
    fi
    if  [[ ${g_bsp_patch_result[lastcpatch]} -gt 0 ]]; then
        patch_version+="c${g_bsp_patch_result[lastcpatch]}"
    fi

    patch_version+=".W${g_wifi_patch_result[lastpatch]}"
    _dpp $LINENO "g_wifi_src_last:$g_wifi_src_last"
    _dpp  $LINENO "{g_wifi_patch_result[lastpatch]}:${g_wifi_patch_result[lastpatch]}"
    if [[ $g_wifi_src_last -lt ${g_wifi_patch_result[lastpatch]} && $g_wifi_src_last -ge 0 ]]; then
        patch_version+="*${g_wifi_src_last}"
    fi
    if  [[ ${g_wifi_patch_result[lastcpatch]} -gt 0 ]]; then
        patch_version+="c${g_wifi_patch_result['lastcpatch']}"
    fi

    if [[ -n ${g_bsp_patch_result[mstr]} ]]; then
        patch_version+="."
        patch_version+=${g_bsp_patch_result[mstr]}
        _dpp $LINENO $patche_version
    else
        _dpp $LINENO "what ???"
    fi
    _dpp $LINENO $patche_version
    if [[ -n ${g_wifi_patch_result[mstr]} ]]; then
        patch_version+="."
        patch_version+=${g_wifi_patch_result[mstr]}
    fi
    _dpp $LINENO $patch_version

    local epivers_hdr=$(find $g_wifi_impl_dir/main -name "epivers.h")
    if [  -z "$epivers_hdr" ];then
        epivers_hdr=$(find $g_wifi_impl_dir/main -name "epivers.h.in")
        if [ ! -z "$epivers_hdr" ];then
            sed -i 's/-PvB[0-9*.c]\+W[0-9*_c]*//g' $epivers_hdr
            sed -i 's/Bm[0-9_,.c]\+//g' $epivers_hdr
            sed -i 's/Wm[0-9,.c]\+//g' $epivers_hdr
            if [[ ! -z `grep "EPI_CUSTOM_VER" $epivers_hdr` ]]; then
                # for internal tob
                sed -i "s/\(^#define\s\+EPI_VERSION_STR\s\+.*@\)\()\"\s\+EPI_CUSTOM_VER$\)/\1-${patch_version}\2/g" $epivers_hdr
            else
                # for releae tag internal source
                sed -i "s/\(^#define\s\+EPI_VERSION_STR\s\+.*\))\"$/\1-${patch_version})\"/g" $epivers_hdr
            fi
        fi
    else
        sed -i 's/-PvB[0-9*.c]\+W[0-9*_c]*//g' $epivers_hdr
        sed -i 's/Bm[0-9_,.c]\+//g' $epivers_hdr
        sed -i 's/Wm[0-9,.c]\+//g' $epivers_hdr
        #release version
        sed -i "s/\(^#define\s\+EPI_VERSION_STR\s\+.*\?)\).*\"$/\1-${patch_version}\"/g" $epivers_hdr
    fi
    echo "patching file "$(echo ${epivers_hdr}|sed "s|$g_top_dir|.|")
}

#get the string repsentation of missing/failed patches from dir
#parameters:
#    in: prefix -"Bm/Wm/c"
#    in: dir
#    out: "3,4-5"

function update_patchstr_with_issue() {
    local missing_str=""
    if [[ $# -gt 2 && -d $2 ]]; then
        local prefix=$1
        local srcmax=0
        local failed_patches
        local patch_numbers
        _dpp $LINNO "! Searching for  missing dir:$2"
        get_dir_patch_numbers $2 ".*patch.failed$" failed_patches
        get_dir_patch_numbers $2 ".*patch$" patch_numbers

        if [[ -z $(eval "printf "%s" $"$3"") ]]; then
            local mstr_empty=1
        else
            local mstr_empty=0
        fi

        _dpp $LINENO "#patch_numbers:${#patch_numbers[@]}"

        if [[ -n $patch_numbers ]]; then
            _dpp $LINENO "#patch_numbers:${patch_numbers[-1]}"
            if [[ ${#patch_numbers[@]} -lt ${patch_numbers[-1]} ]]; then
                local tmp_numbers=(${patch_numbers[@]} $(seq 1 1 ${patch_numbers[-1]}))
                tmp_numbers=($(echo ${tmp_numbers[@]}|tr  ' '  '\n'|sort -n|uniq -u|tr '\n' ' '))
                failed_patches=(${tmp_numbers[@]} ${failed_patches[@]})
                _dpp $LINENO "${failed_patches[@]}"
            fi

            local max=${patch_numbers[-1]}
            case $prefix in
                Bm)
                    srcmax=$g_bsp_src_last
                    g_bsp_patch_result[lastpatch]=${patch_numbers[-1]}
                    ;;
                Bmc)
                    g_bsp_patch_result[lastcpatch]="${patch_numbers[-1]}"
                    if [[ $mstr_empty -eq 0 ]]; then
                        prefix="c"
                    fi
                    srcmac=max
                    ;;
                Wm)
                    srcmax=$g_wifi_src_last
                    g_wifi_patch_result["lastpatch"]=${patch_numbers[-1]}
                    ;;
                Wmc)
                    g_wifi_patch_result["lastcpatch"]="${patch_numbers[-1]}"
                    if [[ $mstr_empty -eq 0 ]]; then
                        prefix="c"
                    fi
                    srcmac=max
                    ;;
                *)
                    srcmac=max
            esac

            local failed_patches=(${failed_patches[@]} $(seq $((max+1)) 1 $srcmax))
            if [[  -n $failed_patches ]]; then
                failed_patches=($(echo ${failed_patches[@]}|tr  ' '  '\n'|sort -n|uniq -u|tr '\n' ' '))
            fi
            local base_num=${failed_patches[0]}
            local current_num=$base_num
            local base_index=0
            local step=1;


            for (( i=1 ; i < ${#failed_patches[@]} ; i++ )); do
                current_num=${failed_patches[$i]}
                if [[ ${current_num} != $((base_num+$step)) ]]; then
                    if [[ $step -eq 1 ]]; then
                        missing_str+="$base_num,"
                    else
                        missing_str+="$base_num-${failed_patches[$((i-1))]},"
                    fi
                    base_num=$current_num
                    base_index=i
                    step=1
                else
                    step=$((step+1))
                fi
            done
            if [[ $step -eq 1 ]]; then
                missing_str+="$current_num"
            else
                missing_str+="$base_num-$current_num"
            fi
        fi
    fi


    if [[ -n $missing_str ]]; then
        eval "${3}+="${prefix}${missing_str}""
    fi
}

function patch_dir_with_patches() {
    local dest_dir=$1
    local is_revert=$2
    local is_test=$3
    shift 3
    local patch_files=(${@})
    local  patch_num=0
    local revert_patch=""
    if [[ $is_revert -eq 1 ]]; then revert_patch="-R"; fi

    _dpp $LINENO "is_revert:$is_revert"  "is_test:$is_test"
    _dpp $LINENO " $dest_dir ${patch_files[@]}"
    if [[ ${#patch_files[@]} -gt 0 ]]; then
        echo "=== Patching "$(echo $dest_dir|sed "s|$g_top_dir|.|")" #######"
    fi
    pushd $dest_dir >/dev/null
    for pf in ${patch_files[@]}
    do
        if [[ "$pf" == *"D2UCONVERT"* ]]; then
            patch_files_d2u_convert $dest_dir $pf
        fi
        if [[ ( $is_revert -eq 0 && ! -e $pf.done  && ! -e $pf.rdone ) || $is_revert -eq 1 ]]; then
        patch_num=$((patch_num+1))
        if [[ -n $(grep "GIT binary patch" $pf) ]]; then
            _dpp $LINENO "git binary patch using p1"
            git apply $revert_patch -p1 <$pf
            touch "${pf}.done"
        elif  patch -p0 -N $revert_patch --dry-run --silent <$pf 2>&1 >/dev/null; then
            patch -p0 $revert_patch <$pf
            if [[ $is_revert -eq 0 ]]; then touch "${pf}.done"; else  rm -f "${pf}.done"; fi
        elif  patch -p1 -N $revert_patch --dry-run --silent <$pf 2>&1 >/dev/null; then
            patch -p1 $revert_patch <$pf
            if [[ $is_revert -eq 0 ]]; then touch "${pf}.done"; else  rm -f "${pf}.done"; fi
        else
            local p0failure=$(patch -p0 -N $revert_patch --dry-run <$pf 2>&1)
            local p1failure=$(patch -p1 -N $revert_patch --dry-run <$pf 2>&1)
            if [[ $g_wifi_patch_src == "release" ]]; then
               #if only for release, all are for p0, if p0 failure is could
               #not find file, that's likely ok as in the release may not have
               #such as RDKB files
               if [[ $p0failure =~ .*can\'t[[:space:]]find.*file.*to.*patch.* ||
                   $p0failure =~ .*Reversed.*patch[[:space:]]detected.*  ||
                  $p1failure  =~ .*Reversed.*patch[[:space:]]detected.* ]]; then
                  _dpp $LINENO "patch file:$pf is likely to patch a file not exist"
                  touch "${pf}.done"
               else
                   if [[ $is_test -eq 0 ]]; then touch "${pf}.failed"; fi
                   echo "Failed to patch:$pf"
               fi

            else
               if [[ $p0failure =~ .*Reversed.*patch[[:space:]]detected.*  ||
                  $p1failure  =~ .*Reversed.*patch[[:space:]]detected.* ]]; then
                  touch "${pf}.done"
                  echo "$(basename $pf) seems patched"
               else
                  if [[ $is_test -eq 0 ]]; then touch "${pf}.failed"; fi
                  echo "$(basename $pf) failed"
                  printf "%s\n%s\n%s\%s\n" "p0 failure:" "$p0failure" "p1 failure:" "$p1failure"
               fi
               if  [[ is_test -eq 1 ]]; then
                   popd >/dev/null
                   _dpp $LINENO "JJJJ- return $patch_num "
                   return $patch_num
               fi
            fi
        fi
    fi
    done
    popd >/dev/null
    return 0

}

function pp_patch_version_modify() {
    local src_patch_version=$1;
    local dst_dir_version=$2
    shift 2
    local patch_files=($@)
    _dpp $LINENO "src_patch_version:$src_patch_version"
    _dpp $LINENO "dst_dir_version: $dst_dir_version"
    if [[ -f $src_patch_version ]]; then
        for pf in $ ${patch_files[@]}
        do
            if [[ -f $pf.done ]]; then
                local jira=$(echo $(basename $pf)|sed 's|.*\(SWBCACPE-[[:digit:]]\+\).*|\1|')
                if [[ -n $jira ]]; then
                    local jira_item=$(awk '{if ($5=="'$jira',") {print $0}}' $src_patch_version)
                    if [[ -n $jira_item ]]; then
                        echo "$jira_item" >>$dst_dir_version
                    else
                        echo "!!!! there is no release version in $src_patch_version for $jira"
                    fi
                fi
            fi
        done
    else
        echo "??why there is not $src_patch_version??"
    fi
}

# function to patch destination dir with patches from the source dir
# Parameters:
#    in: dest_dir
#   in: src_dir
# Result:
#   .done file is created when patch succeeds
#   .failed file is created when failed to patch

function patch_dir_from_dir() {
    local dest_dir=$1
    local src_dir=$2
    local is_revert=$3
    local is_test=$4
    _dpp $LINENO "#### PATCHING: dest_dir:$dest_dir from src_dir:$src_dir is_revert:$is_revert, is_test:$is_test"

    if [[ -d $dest_dir  && -d $src_dir ]]; then
        local patch_files=($(find $src_dir -maxdepth 1 -type f -regex ".*patch$"|sort))
        local patch_files_done=($(find $src_dir -maxdepth 1 -type f -regex ".*patch\.[r]?done$"|sort))
        _dpp $LINENO: ${patch_files[@]}
        for fld in ${patch_files_done[@]}
        do
            fld=${fld%.*done}
            _dpp $LINENO: "to remove $fld"
            patch_files=(${patch_files[@]/$fld})
            if [[ ${#patch_files[@]} -eq 1 && $patch_files == $fld ]]; then
                patch_files=()
            fi
        done
        _dpp $LINENO "PATCHES to be applied" ${patch_files[@]}
        if [[ $g_brcm_list_patches_only -eq 1 ]]; then
            if [[ ${#patch_files[@]} -gt 0 ]]; then
                local allfiles=$(echo ${patch_files[@]}|tr ' ' '\n'|sed "s|$g_top_dir|.|")
                echo "$allfiles"
            fi
            return 0
        fi

        #now get all the patches needs to be patched, try to patch
        if [[ ${#patch_files[@]} -eq 0 ]]; then
            ret_code=0
        else
            patch_dir_with_patches $dest_dir $is_revert $is_test ${patch_files[@]}
            local ret_code=$?
            #for BSP and RDKB,update the patch.list file with new changes. 
            if [[ $5 =~ ^_new_patch_BSP_num$ || $5 =~ ^_new_patch_RDKB(SOC|SYS)_num$ ]]; then
                _dpp $LINENO "pp_patch_version_modify $src_dir/patch.list $dest_dir/patch.version.wifi ${patch_files[@]}"
                pp_patch_version_modify $src_dir/patch.list $dest_dir/patch.version.wifi ${patch_files[@]}
                if [[ $5 =~ ^_new_patch_BSP_num$ ]]; then
                    echo "patching file ${dest_dir//$g_top_dir/.}/patche.version.wifi"
                else
                    echo "patching file ${dest_dir//$(realpath $g_top_dir/../)/../bcasdk}/patche.version.wifi"
                fi
            fi
            if [[ $5 =~ ^_new_patch_BSPWIFI_num$ ]]; then
                _dpp $LINENO "pp_patch_version_modify $src_dir/patch.list $dest_dir/patch.version.bspwifi ${patch_files[@]}"
                pp_patch_version_modify $src_dir/patch.list $dest_dir/patch.version.bspwifi ${patch_files[@]}
                    echo "patching file ${dest_dir//$g_top_dir/.}/patche.version.bspwifi"
            fi
            if [[ $is_test -eq 0  &&  $is_revert -eq 0  && $5 =~ ^_new_patch_WIFI_num$ ]]; then
                local all_wifi_done_file=($(find $src_dir -maxdepth 1 -name *.patch.done))
                if [[ -n $all_wifi_done_file ]];then
                    local wifi_done_file=""
                    for wifi_done_file in ${all_wifi_done_file[@]}
                    do
                        if [[ -f $g_top_dir/wifi_patch_version.$g_wifi_version && -z $(grep $wifi_done_file $g_top_dir/wifi_patch_version.$g_wifi_version) ]]; then
                            echo $(basename $wifi_done_file) >> $g_top_dir/wifi_patch_version.$g_wifi_version
                        fi
                    done
                fi
            fi
            if [[ ! -z $5 ]]; then
                eval "$5=$(($5+1))"
                g_new_patch=$(($g_new_patch+1))
            fi
        fi
        if [[ $is_test -eq 1 && $is_revert -eq  0 ]]; then
            #if it is test, anyway need to revert all patched  files
            _dpp $LINENO "there are $ret_code to be reverted!!!!!"
            if [[ $ret_code -eq 0 ]]; then
                local revert_patches=($(echo ${patch_files[@]}|tr  ' '  '\n'|sort -n|uniq -u|tr '\n' ' '))
            else
                local revert_patches=()
                for ((i=$((ret_code-2));i>=0;i--))
                do
                    _dpp $LINENO "$(basename ${patch_files[i]})"
                    revert_patches+=( ${patch_files[i]} )
                done
            fi
            local dd=$(patch_dir_with_patches $dest_dir 1 0  ${revert_patches[@]})
            return $ret_code
        fi
    else
        _dpp $LINENO "what?$dest_dir or $src_dir does not exists"
    fi
    return 0
}

function _get_wifi_patched_num() {
    local wlimpl_dir="$1/bcmdrivers/broadcom/net/wl/$(basename $2)"
    if [[ $g_internal_build -eq 1 ]]; then
        epivers_hdr=$(find $wlimpl_dir/main -name "epivers.h.in")
    else
        epivers_hdr=$(find $wlimpl_dir/main -name "epivers.h")
    fi
    if [[ ! -z "$epivers_hdr" ]];then
        if [[ $g_internal_build -eq 1 ]]; then
            local release_ver_line=($(awk '/^\s*#define\s*EPI_VERSION_STR/{print $5}' $epivers_hdr))
        else
            local release_ver_line=($(awk '/^\s*#define\s*EPI_VERSION_STR/{print $6}' $epivers_hdr))
        fi
        if [[ -n $release_ver_line ]]; then
            local vers=($(echo ${release_ver_line[0]}|sed 's/.*PvB\([0-9_c]\+\).W\([0-9c]\+\).*/\1 \2/'))
            if [[ ${#vers[@]} -eq 2 ]]; then
                eval "$3=${vers[0]}"
                eval "$4=${vers[1]}"
            fi
        fi
    fi
}

function mark_done_by_number() {
    local patches_dir=$1
    local until_number=$2
    local _is_kudu_=$3
    local patch_name=""
    if [[  -d  $patches_dir ]]; then
        local patch_files=($(find $patches_dir -maxdepth 1 -regex '.*/[0-9]+[^/]*\.patch$'|sort))
        if [[ -n $patch_files ]]; then 
            for patch_name in ${patch_files[@]}
            do
                local num=$(echo $(basename $patch_name) |sed -n "s/^[0-9]\+-0*\([1-9][0-9]*\)[_-].*/\1/p")
                if [[ -z "$num" ]];then
                    num=$(echo $(basename $patch_name) |sed -n "s/^0*\([1-9][0-9]*\)[_-].*/\1/p")
                fi
                if [[ ! -z "$num" ]]; then 
                    if [[ $num -le $until_number ]]; then 
                        if [[ ! -f $patch_name.failed && ! -f  $patch_name.rdone && ! -f $patch_name.done ]]; then
                            touch $patch_name.done
                        fi
                    elif [[ $_is_kudu_ -eq 1 ]]; then
                        rm -f $patch_name.done
                    fi
                fi
            done
        fi
    fi
}

function _mark_patch_versioned_done_wifi() {
    local _dest_dir=$1
    local _wifi_version_file="$g_top_dir/wifi_patch_version.$g_wifi_version"
    local _is_kudu_=0
    if [[ -f $_wifi_version_file ]]; then
        local alldone=($(awk '{if (match($0,"^[[:digit:]]+.*done")) {print $1}}' $_wifi_version_file))
        local done_file
        for done_file in ${alldone[@]}
        do
            touch $_dest_dir/$done_file
        done
        fi
    local _bsp_src_last=0
    local _wifi_src_last=0
    _get_wifi_patched_num $g_top_dir $g_wifi_impl_dir _bsp_src_last _wifi_src_last
    local _last_number=0
    if [[ -z $2 ]]; then 
        # for kudu patch
        _last_number=$_wifi_src_last
        _is_kudu_=1
    else
        _last_number=$_bsp_src_last
    fi
    mark_done_by_number $_dest_dir $_last_number $_is_kudu_
}

function _mark_patch_versioned_done() {
    local patch_version_repo=$1
    local patch_src_dir=$2
    local ppfile
    _dpp $LINENO $patch_version_repo
    _dpp $LINENO $patch_src_dir
    if [[ -f $patch_version_repo && -d $patch_src_dir ]]; then
        local existing_patches=($( awk -F "," '{if (match($0,"- {csp: CS[0-9]+.*")) { print $2 }}' $patch_version_repo ))
        if [[ -n $existing_patches ]]; then
            existing_patches=(${existing_patches[@]//ref:/})
            existing_patches=($(echo ${existing_patches[@]}|tr  ' '  '\n'|sort -n|uniq |tr '\n' ' '))
            _dpp $LINENO "existing_patches:${existing_patches[@]}"
            for ppfile in ${existing_patches[@]}
            do
                local patchfile=$(find $patch_src_dir -name *$ppfile*.patch)
                if [[ -n $patchfile && ! -f $patchfile.rdone ]]; then
                    touch $patchfile.done
                fi
            done
        else
            _dpp $LINENO "$patch_version_repo did not have patch"
        fi
    else
        _dpp $LINENO "$patch_version_repo || $patch_src_dir  does not exists"
    fi
}

function wl_handle_rdkb_patch() {

    local rdkb_local_patch_dir=$1
    local rdkb_repo_root_dir=$(realpath $g_top_dir/../)
    _dpp $LINENO $1
    _dpp $LINENO $g_top_dir
    if [[ -d $1 && -d $rdkb_repo_root_dir/meta-rdk-broadcom-bca-system ]]; then
        local rdk_new_patches_soc=0
        local rdk_new_patches_sys=0
        _mark_patch_versioned_done $rdkb_repo_root_dir/meta-rdk-broadcom-bca-system/patch.version  $rdkb_local_patch_dir/system
        _mark_patch_versioned_done $rdkb_repo_root_dir/meta-rdk-broadcom-bca-soc/patch.version $rdkb_local_patch_dir/soc
        patch_dir_from_dir $rdkb_repo_root_dir/meta-rdk-broadcom-bca-system $rdkb_local_patch_dir/system 0 0 rdk_new_patches_sys
        patch_dir_from_dir $rdkb_repo_root_dir/meta-rdk-broadcom-bca-soc $rdkb_local_patch_dir/soc  0  0 rdk_new_patches_soc
    else
        _dpp $LINENO "RDKB does not eixsits"
    fi
}


# Main Function to handle patches
# Use global variables to retrive patches from right folder
# Parameters:
#    in: type of patch
#   out: (maxpatch num, patch resultstring)

function  wl_handle_patch() {

    if [[ $g_brcm_list_patches_only -gt 0 ]]; then
        echo -e "\n##### Patches for: $1  ###########"
    else
        echo "##### Start to handle $1 patches ###########"
    fi

    local patches_src_dir="$g_top_dir/patches/$g_bsp_rel_version"
    local patch_dst_dir=$g_top_dir
    local new_patch_num_name="_new_patch_${1}_num";
    eval "${new_patch_num_name}=0"
    case  $1 in

        BSP)
            patches_src_dir=$patches_src_dir/bsp
            _mark_patch_versioned_done  $g_top_dir/patch.version $patches_src_dir
            _mark_patch_versioned_done  $g_top_dir/patch.version.wifi  $patches_src_dir
            ;;

        BSPWIFI)
            patches_src_dir="$g_top_dir/patches/$g_bsp_rel_version/wifi/bspwifi"
            local missing_lable="Bm"
            _mark_patch_versioned_done  $g_top_dir/patch.version.bspwifi  $patches_src_dir
            _mark_patch_versioned_done_wifi $patches_src_dir "BSP"
            ;;

        RDKBSYS)
            patches_src_dir=$patches_src_dir/rdkb/system
            patch_dst_dir="$(realpath $g_top_dir/../)/meta-rdk-broadcom-bca-system"
            _mark_patch_versioned_done $patch_dst_dir/patch.version.wifi  $patches_src_dir
            #wl_handle_rdkb_patch $patches_src_dir
            ;;

        RDKBSOC)
            patches_src_dir=$patches_src_dir/rdkb/soc
            patch_dst_dir="$(realpath $g_top_dir/../)/meta-rdk-broadcom-bca-soc"
            _mark_patch_versioned_done $patch_dst_dir/patch.version.wifi  $patches_src_dir
            #wl_handle_rdkb_patch $patches_src_dir
            ;;

        WIFI)
            local missing_lable="Wm"
            patches_src_dir="$g_wifi_patch_src_dir/$g_wifi_version"
            patch_dst_dir=$g_wifi_impl_dir
            _mark_patch_versioned_done_wifi $patches_src_dir
#            _mark_patch_version_done_wifi $patches_src_dir $patch_dst_dir
        
            _dpp $LINENO $patches_src_dir
            ;;
        *)
            _dpp $LINENO "NOT SUPPORTED $1"
            ;;
    esac


    if  [ -d $patches_src_dir ];then

        local missing_patches_str=""
        if [[ $1 == "BSPWIFI" && ($g_brcm_jenkin_rel_bld -eq 1 || $g_internal_build -eq 1)  ]]; then
            echo "internal release build, no need for BSP patch"
        else
            patch_dir_from_dir $patch_dst_dir $patches_src_dir 0 0 $new_patch_num_name
            patch_dir_from_dir $patch_dst_dir $patches_src_dir/customer 0 0 $new_patch_num_name
        fi

        _dpp $LINENO "$new_patch_num_name :${!new_patch_num_name}"

        case  $1 in
            BSPWIFI)
                g_new_BSPWIFI_patch=${!new_patch_num_name}
                ;;
            WIFI)
                g_new_WIFI_patch=${!new_patch_num_name}
                ;;
        esac

        if [[  $g_has_wifi -eq 1 && -n $missing_lable ]]; then
            update_patchstr_with_issue      $missing_lable $patches_src_dir missing_patches_str
            update_patchstr_with_issue      $(printf "%s%s" $missing_lable "c") $patches_src_dir/customer missing_patches_str
            _dpp $LINENO $missing_patches_str
            _dpp $LINENO  "bsp last patch:" ${g_bsp_patch_result["lastpatch"]}
            if [[ $1 == "BSPWIFI" ]]; then
                g_bsp_patch_result[mstr]="$missing_patches_str"
                _dpp $LINENO ${g_bsp_patch_result[mstr]}
            else
                g_wifi_patch_result[mstr]="$missing_patches_str"
            fi
        fi
    fi
}


# for wifi patches, a few releases may share the same patches, so we
# have  wifi_patch_mateher.ini file to record this

# parameters:
# in: wifi_version
#
function adjust_wifi_patch_version() {
    local _wifi_version_=$1
    _dpp $LINENO $@
    if [[ $# -gt 2 ]]; then
        g_wifi_patch_src_dir=$1/$3
        _wifi_patch_src_dir=$g_wifi_patch_src_dir
        _wifi_version_=$2
    else
        _wifi_patch_src_dir="$g_top_dir/patches/$g_bsp_rel_version/wifi/release"
    fi
    _dpp $LINENO $_wifi_patch_src_dir
    
    while [[ -n $_wifi_version_ && ! -d $_wifi_patch_src_dir/$_wifi_version_ ]]
    do
        _dpp $LINENO "$g_wifi_patch_src_dir/$_wifi_version_"
        _dpp $LINENO  ": $_wifi_version_ patch does not exist, find matching"
        local match_line=$(grep "^$_wifi_version_:[0-9_]\+$" $g_top_dir/patches/patch_mapping.ini)
        _dpp $LINENO $match_line
        if  [[ -n $match_line ]]; then
            _wifi_version_=$(echo $match_line|sed "s/${_wifi_version_}:\([0-9_]\+\)/\1/")
        else
            _wifi_version_=""
        fi
    done
    if [[ -n $_wifi_version_ ]]; then
    if [[ $# -gt 2 ]]; then
        eval "$4=$_wifi_version_"
    else
        eval "$2=$_wifi_version_"
    fi
    fi
}

function adjust_bsp_patch_version() {
    local _wl_dir=$1
    local _wifi_version_=$2
    local _wifi_patch_src=$3
    local _bsp_version_=$4
    local _bsp_version_pre=$4
    local _bsp_patch_dir="$g_top_dir/patches"

    _dpp $LINENO "$_bsp_patch_dir/$_bsp_version_"
    while [[ -n $_bsp_version_ && (! -d $_bsp_patch_dir/$_bsp_version_/wifi  ||  \
        -z "`find $_bsp_patch_dir/$_bsp_version_/wifi  -maxdepth 1 -name *.patch`") ]]
    do
        _dpp $LINENO  ": $_bsp_version_ patch does not exist, find matching"
        local match_line=$(grep "^$_bsp_version_:.\+$" $_bsp_patch_dir/patch_mapping.ini)
        _dpp $LINENO $match_line
        if  [[ -n $match_line ]]; then
            _bsp_version_=$(echo $match_line|sed "s/${_bsp_version_}:\(.\+\)/\1/")
        else
            _bsp_version_=""
        fi
        if [[ $_bsp_version_pre == $_bsp_version_ ]];then
            break
        else
            _bsp_version_pre=$_bsp_version_
        fi
    done
    if [[ -n  $_bsp_version_ ]]; then
       _dpp $LINENO $_bsp_version_ "$5=$_bsp_version_"
       eval "$5=$_bsp_version_"
    fi
}

#Init all needed global variables


function declare_global_variable() {
    g_top_dir=$1
    if [[ -n $2 ]]; then
      g_router_dir=$2
    fi
    if [[ -n $3 ]]; then
        g_wl_dir=$3
    else
       g_wl_dir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    fi
    g_new_WIFI_patch=0
    g_new_BSPWIFI_patch=0
    g_new_patch=0

    _dpp $LINENO "g_top_dir:$g_top_dir"
    _dpp $LINENO "g_router_dir:$g_router_dir"
    if [[ $g_has_wifi -eq 1 ]]; then
        g_wifi_impl=$(echo $g_router_dir|sed -n 's/.*\(impl[0-9]\+\).*/\1/p')
        g_wifi_impl_dir=$g_wl_dir/${g_wifi_impl}
        _dpp $LINENO "g_wifi_impl_dir:$g_wifi_impl_dir"
        wlimpl_get_ver $g_wifi_impl_dir  g_wifi_versions >/dev/null
        _dpp $LINENO: "g_wifi_impl_dir $g_wifi_impl_dir  && g_wifi_versions:${g_wifi_versions[@]}"
        get_pp_last_ver $g_top_dir $g_wifi_impl_dir g_bsp_src_last g_wifi_src_last
        g_wifi_version=${g_wifi_versions[0]}
        g_wifi_patch_src=${g_wifi_versions[1]}
        g_wifi_patch_src_dir=$g_top_dir/patches/$g_bsp_rel_version/wifi/$g_wifi_patch_src
        _dpp $LINENO $g_wifi_version  $g_wifi_patch_src_dir
        adjust_wifi_patch_version  $g_wifi_version  g_wifi_version
        _dpp $LINENO $g_wifi_version
        adjust_bsp_patch_version $g_wl_dir $g_wifi_version $g_wifi_patch_src $g_bsp_rel_version  g_bsp_rel_version
        if [[ -z $g_wifi_version ]]; then
            _dpp  $LINENO "No Wifi Patch found"
        else
            _dpp $LINENO $g_wifi_version
        fi
    fi
    declare -g -A g_bsp_patch_result
    declare -g -A g_wifi_patch_result
    g_bsp_patch_result[lastpatch]=0
    g_bsp_patch_result[lastcpatch]=0
    g_bsp_patch_result[mstr]=''
    g_wifi_patch_result[lastpatch]=0
    g_wifi_patch_result[lastcpatch]=0
    g_wifi_patch_result[mstr]=''
}

#unset all global variables

function unset_global_variable() {
    unset g_top_dir
    unset g_router_dir
    unset g_wifi_patch_src_dir
    unset g_wl_dir
    unset g_bsp_rel_version
    unset g_wifi_versions
    unset g_wifi_impl
    unset g_wifi_impl_dir
    unset g_wifi_version
    unset g_wifi_patch_src
    unset g_bsp_patch_result
    unset g_new_WIFI_patch
    unset g_new_BSPWIFI_patch
    unset g_new_patch
}


function choose_wlimpl_options() {

    local wldir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    local local_wlvers=()
    if [[ $g_internal_build -eq 0 ]]; then
        #for release/customer build,do the patch martching..#
        local allwlimpls=($(find $wldir -maxdepth 2 -name "main"))
        local wifi_rel_patch_dir="$g_top_dir/patches/$g_bsp_rel_version/wifi/release"
        
        _dpp $LINENO $wifi_rel_patch_dir
        if [[ -d $wifi_rel_patch_dir ]]; then 
            pushd $wifi_rel_patch_dir >/dev/null
            local wlversion=($(find . -maxdepth 1 -type d))
            _dpp $LINENO ${allwlimpls[@]}
            _dpp $LINENO ${wlversion[@]}
            local wlver="";
            wlversion=(${wlversion[@]//./})
            wlversion=(${wlversion[@]//\//})
            _dpp $LINENO ${wlversion[@]}
            for wlimpl in ${allwlimpls[@]}
            do
                wlimpl_get_ver $(dirname $wlimpl)  wlver
                _dpp $LINENO $wlver
                local_wlvers+=($wlver)
                adjust_wifi_patch_version  $wlver wlver
                _dpp $LINENO $wlver
                if [[ -n $wlver ]]; then
                for ver in ${wlversion[@]}
                do
                    if  [[ $wlver == $ver ]]; then
                        allwlimpl+=($wlimpl)
                    fi
                done
                else
                    _dpp $LINENO "there is no patch for ${local_wlvers[@]}"
                fi
            done
            popd >/dev/null
        else
            _dpp $LINENO "$wifi_rel_patch_dir does not exists"
        fi
    else
        allwlimpl+=($(find $wldir -maxdepth 2 -name "main"))
    fi

    if [[ ${#allwlimpl[@]} -gt 1 ]]; then
        local sn=$(seq 1 1 ${#allwlimpl[@]})
        for i in ${sn[@]}
        do
            echo "$i: $(dirname ${allwlimpl[$((i-1))]})"
        done
        printf "Select one wlarch to continue:"
        read select
        if [[ ! -z $select  &&  $select =~ [0-9]+$ && $select -le ${#allwlimpl[@]}  && $select -gt 0 ]]; then
            select=$((select-1))
                eval "${1}=${allwlimpl[$select]}"
                g_has_wifi=1
            else
                echo "Not valide input[$select], quit!"
        fi
    elif [[ ! -z $allwlimpl ]]; then
        eval "${1}=$(dirname $allwlimpl)"
        g_has_wifi=1
    else
        if [[ $g_internal_build -eq 0 ]]; then
            echo -e "\nYou source tree has no any matching wifi source ${wlversion[@]} to patch"
            if [[ -n ${local_wlvers[@]} ]]; then
                echo "You source tree wifi_version is: ${local_wlvers[@]} to patch"
            fi
        fi
    fi
}

function pp_main() {

    if [[ -n $2 ]]; then g_has_wifi=1; fi
    declare_global_variable $1 $2  $3
    if [[ $g_internal_build -eq 0 || $g_brcm_list_patches_only -gt 0 ]]; then
        #Only need to handle BSP & RDK patch for release build
        wl_handle_patch "BSP"
        if [[ $g_has_rdkb_src -gt 0 ]];  then
            wl_handle_patch "RDKBSOC"
            wl_handle_patch "RDKBSYS"
        fi
    else
        _dpp $LINENO "Internal build, no need to patch BSP and RDKB"
    fi

    if [[ $g_has_wifi -eq 1 ]]; then
        wl_handle_patch "BSPWIFI"
        wl_handle_patch "WIFI"
        if [[ $g_new_WIFI_patch -gt 0 || g_new_BSPWIFI_patch -gt 0 ]];  then
            wl_update_patch_ver_str
        else 
            _dpp $LINENO "no wifi patches, so not updated wifi version"
        fi
    else
        _dpp $LINENO "NO wifi source, no need to patch"
    fi


    if [[ -z $g_brcm_list_patches_only || $g_brcm_list_patches_only -eq  0 ]]; then
       if [[ $g_new_patch -gt 0 ]]; then
           echo -e "\nPatching Done"
       else
           echo -e "\nAll patches have been applied, no new patches.\n"
       fi
    else
        echo -e "\n----------"
    fi
    unset_global_variable
}



function _base_global() {
    g_top_dir=$(realpath "$1")
    g_wl_dir="$g_top_dir/bcmdrivers/broadcom/net/wl"
    source $g_top_dir/version.make
    g_bsp_rel_version="$BRCM_VERSION"."$BRCM_RELEASE"L."$BRCM_EXTRAVERSION"
    g_has_rdkb_src=0;
    if [[ -d $g_top_dir/../meta-rdk-broadcom-bca-system ]]; then
        g_has_rdkb_src=1
    fi
    if [[ -f $g_top_dir/patches/pputil.sh ]]; then
        g_internal_build=1;
    else 
        g_internal_build=0;
    fi
}

(return 0 2>/dev/null) && sourced=1 || sourced=0
g_has_wifi=0;
if [ $sourced -eq 0 ]; then
    if [[ $# -eq 0 ]]; then
       _base_global "../"
       choose_wlimpl_options _pp_wlimpl_dir
       if  [[ -n $g_brcm_list_patches_only && $g_brcm_list_patches_only -gt 0 ]]; then
           echo -e "\n--- Patch Files to be used for Patching----\n"
       fi
       pp_main $g_top_dir $_pp_wlimpl_dir
       unset g_top_topdir
       unset g_internal_build
       unset g_wl_dir
       unset _pp_wlimpl_dir
    elif [[ $# -ge 2 ]]; then
        if [[ $1 =~ 'wlversion' ]]; then
            #Be careful, here only to print version.No other Echo
            wlimpl_get_ver $2  g_wifi_versions
            if [[ ${#g_wifi_versions[@]} -eq 3 ]]; then
                echo ${g_wifi_versions[2]}
            else
                echo ""
            fi
            unset g_wifi_versions
        else
            _base_global $1
            pp_main $1 $2 $3
        fi
    else
                    echo "utility.sh builddir wlrouterdir"
    fi
else
    _base_global "../"
fi
