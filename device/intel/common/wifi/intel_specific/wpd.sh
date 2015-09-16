#!/bin/sh
# WPD stands for Wprd Process Delivery

#
# TODO:
#  - add a --resume option
#

###############################################################
#          S C R I P T     P A R A M  E T E R S               #
###############################################################
#
# Script user-parameters
#
WPD_SRC_GIT_PATH=
WPD_DST_GIT_PATH=
WPD_TAG_FROM=
WPD_TAG_TO=
BZ_ID=

#
# Script internal parameters dynamically updated
#
WPD_OUTPUT_PATH=
WPD_PATCHES_ORI_PATH=
WPD_PATCHES_PROCESS_PATH=

#
# Script fixed internal parameters
#
WPD_CLEANUP_PATTERNS="-e Reported-and-tested-by: -e Signed-off-by: -e Reviewed-on: -e Tested-by: -e x-iwlwifi-stack-dev: -e Reviewed-by: -e Reported-by: -e Cc: -e x-iwlwifi-hostap-dev-build"
WPD_PATCHES_PREFIX=
# for driver integration:
#     WPD_PATCHES_PREFIX="--src-prefix=a/iwl-stack-dev/ --dst-prefix=b/iwl-stack-dev/"
# for supplicant integration leave empty:
#     WPD_PATCHES_PREFIX=

###############################################################
#                 F U N C T  I O N S                          #
###############################################################
#
# name: usage()
# desc: prints the script usage
#
usage()
{
echo "Usage is:
wpd.sh -s <WPRD_GIT_PATH> -d <DRD_GIT_PATH> -t <WPRD_GIT_TAG> -f <WPRD_GIT_TAG_FROM> -b <BZ_ID> -p <PREFIX>

WPD stands for Wprd Process Delivery.
It automatically integrates WPRD delivery into DRD mainline.

Mandatory parameters:
         -s <WPRD_GIT> : path of the WPRD git repository to integarte from
         -d <DRD_GIT> : path of the DRD git repository to integrate to
         -t <WPRD_GIT_TAG> : tag name of the WPRD delivery we want to integrate
         -b <BZ_ID>: bugzilla number to use for integration

Optional parameters:
         -f <WPRD_GIT_TAG_FROM> : tag name from which we want to start integration. If not provided,
                                  integration will start from tag that precedes WPRD_GIT_TAG.
                                  Case where WPRD_GIT_TAG is the very first tag is also managed.
         -p <PREFIX> : if WPRD_GIT and DRD_GIT repositories don't have the same tree structure, you can
                       make script take it into account in its format-patch operations.
                       ex: for driver integration:  -p iwl-stack-dev

Example:
    wpd.sh -s ~/workspace/wprd -d ~/workspace/mailine -t LnP_PRE_PLATFORM_INT_17102013 -f LnP_PRE_PLATFORM_INT_18062013 -b 14325 -p iwl-stack-dev
"
}

#
# name: clean_vars()
# desc: clean all variables used by script
#
clean_vars()
{
    # Clean vars
    src_git_exist=0
    dst_git_exist=0
    tag_exist=0
    previous_tag=0
}

#
# name: check_git_repo_exist()
# desc: check that the directory provided in arg is a GIT repository
#
check_git_repo_exist()
{
    if [ -e "$1/.git" ]; then
	echo "1"
    else
	echo "0"
    fi
}

#
# name: check_git_tag_exist()
# desc: check that tag given in arg is a valid tag withing WPD_SRC_GIT_PATH git repository.
#
check_git_tag_exist()
{
    cd $WPD_SRC_GIT_PATH
    tmp=`git tag | grep $1`
    if [ "$tmp" = "" ]; then
	echo "0"
    else
	echo "1"
    fi
}

#
# name: get_previous_tag()
# desc: find out what is the previous tag within WPD_SRC_GIT_PATH before the tag provided in arg.
#       If the tag in arg is the only one, return ""
# WARNING: we'd like to retrieve all tags sorted by date.
# However, "git tag" prints tags in alphabetical orders.
# We could have used commands such as:
#     git for-each-ref --format="%(taggerdate): %(refname)" --sort=-taggerdate refs/tags
# Unfortunately WPRD git tags are unannotated (not created with -a option).
# That means that we cannot sort them by taggerdate since this field
# is not filled in when unannotated.
# Furthermore, WPRD uses a tag name format that cannot be used to order consistently tags
# by date.  ex: LnP_PRE_PLATFORM_INT_17102013
# So no other choice than horrible complex git log and grep commands
get_previous_tag()
{
   cd $WPD_SRC_GIT_PATH
   tmp=`git log --pretty=format:'%h %ad | %d'  | grep tag | grep $1 -A 1 | tail -1 | awk -F"[()]" '{print $2}' | cut -d ' ' -f 2`

   # if no previous commit return empty (it is the 1st one)
   if [ "$tmp" = "$1" ]; then
       echo ""
   else
       echo $tmp
   fi
}

#
# name: get_patches()
# desc: produces the base patches.
#       Just adapt them to main directory structure that differ
get_patches()
{
     cd $WPD_SRC_GIT_PATH

     if [ "$1" = "" ]; then
	 git format-patch $WPD_TAG_TO -o $WPD_PATCHES_ORI_PATH $WPD_PATCHES_PREFIX > /dev/null
     else
	 git format-patch $1..$WPD_TAG_TO -o $WPD_PATCHES_ORI_PATH $WPD_PATCHES_PREFIX  > /dev/null
     fi
}

#
# name: process_patches()
# desc: modifies the patches to fit mainline requirements regarding commit message
#
process_patches()
{
    cd $WPD_PATCHES_ORI_PATH

    for i in `ls`; do

	# Store the signed-off-by (the first one if several)
	signed_off=`cat $i | grep Signed-off-by: | head -1`
	if [ -z "$signed_off" ]; then
	    signed_off="Signed-off-by: wprd@intel.com"
	fi

	# Remove useless patterns (signed-off, reviewed-by,...)
	cat $i | grep -v $WPD_CLEANUP_PATTERNS > $WPD_PATCHES_PROCESS_PATH/$i.tmp
	mv $WPD_PATCHES_PROCESS_PATH/$i.tmp $WPD_PATCHES_PROCESS_PATH/$i

	# Add Bugzilla field
	cat $WPD_PATCHES_PROCESS_PATH/$i | sed -e "0,/^$/{s/^$/\\nBZ: $BZ_ID\\n/}"  > $WPD_PATCHES_PROCESS_PATH/$i.tmp
	mv $WPD_PATCHES_PROCESS_PATH/$i.tmp $WPD_PATCHES_PROCESS_PATH/$i

	# Replace change-id with wprd-change-id
	cat $WPD_PATCHES_PROCESS_PATH/$i | sed -e "s/Change-Id/Wprd-Change-Id/g" > $WPD_PATCHES_PROCESS_PATH/$i.tmp
	mv $WPD_PATCHES_PROCESS_PATH/$i.tmp $WPD_PATCHES_PROCESS_PATH/$i

	# Add categorization
	cat $WPD_PATCHES_PROCESS_PATH/$i | sed -e "0,/---/{s/---/Category: device enablement\\nDomain: CWS.WIFI-Intel\\nOrigin: internal\\nUpstream-Candidate: no, proprietary\\n$signed_off\\n\\n---/}" > $WPD_PATCHES_PROCESS_PATH/$i.tmp
	mv $WPD_PATCHES_PROCESS_PATH/$i.tmp $WPD_PATCHES_PROCESS_PATH/$i

	# Just in supplicant case, we sometimes have invalid email
	cat $WPD_PATCHES_PROCESS_PATH/$i | sed -e "s/From: WCD hostap <>/From: WCD hostap <wcd@hostap.com>/g" > $WPD_PATCHES_PROCESS_PATH/$i.tmp
	mv $WPD_PATCHES_PROCESS_PATH/$i.tmp $WPD_PATCHES_PROCESS_PATH/$i

    done
}

#
# name: apply_patches()
# desc: apply processed patches into WPD_DST_GIT_PATH
#
apply_patches()
{
    cd $WPD_DST_GIT_PATH
    git clean -xfd
    git am --abort
    git reset --hard
    repo abandon Integ_$WPD_TAG_TO .
    repo start   Integ_$WPD_TAG_TO .
    for i in `ls $WPD_PATCHES_PROCESS_PATH`; do
	echo "Applying patch $WPD_PATCHES_PROCESS_PATH/$i\n"
	# apply patch
	git am $WPD_PATCHES_PROCESS_PATH/$i

	# did patch apply well?
	tmp=`git status | grep "fix conflicts" | wc -l`
	if [ "$tmp" != "0" ]; then
	    echo "ERROR! Patch $WPD_PATCHES_PROCESS_PATH/$i did not apply well."
	    echo "Resolve conflict (git add, git am --resolved). Then press ENTER to continue!"
	    read ans
	fi
	# add a Change-Id
	git commit --amend -m ""
	# remove blank line between Categorizeation block and Change-Id
    done
}

#
# name: clean_wpd_output()
# desc: remove wpd output within WPD_OUTPUT_PATH
#
clean_wpd_output()
{
    rm -rf $WPD_OUTPUT_PATH
    mkdir -p $WPD_PATCHES_ORI_PATH
    mkdir -p $WPD_PATCHES_PROCESS_PATH
}

###############################################################
#                S C R I P T     E N T R Y                    #
###############################################################

# parse cmd line args
while getopts s:d:p:b:f:t: opt
do
    case "$opt" in
      s)  WPD_SRC_GIT_PATH="$OPTARG";;
      d)  WPD_DST_GIT_PATH="$OPTARG";;
      f)  WPD_TAG_FROM="$OPTARG";;
      t)  WPD_TAG_TO="$OPTARG";;
      p)  WPD_PATCHES_PREFIX="--src-prefix=a/$OPTARG/ --dst-prefix=b/$OPTARG/";;
      b)  BZ_ID="$OPTARG";;
      \?) # unknown flag
          usage
	  exit 1;;
    esac
done
shift `expr $OPTIND - 1`

# check that mandatory cmd line options are here
if [ -z $WPD_SRC_GIT_PATH ] || [ -z $WPD_DST_GIT_PATH ] || [ -z $BZ_ID ] || [ -z $WPD_TAG_TO ]
then
      echo "ERROR! Incomplete command\n"
      usage
      exit 1
fi

# Update internal parameters
WPD_OUTPUT_PATH=$WPD_SRC_GIT_PATH/wpd
WPD_PATCHES_ORI_PATH=$WPD_OUTPUT_PATH/patches_ori
WPD_PATCHES_PROCESS_PATH=$WPD_OUTPUT_PATH/patches_processed

# Clean variables used by WPD
clean_vars

# Check src & dst git repositories
src_git_exist=$(check_git_repo_exist $WPD_SRC_GIT_PATH)
if [ $src_git_exist -eq 0 ]; then
    echo "ERROR! Source git repo <$WPD_SRC_GIT_PATH> is not a valid GIT repository."
    exit 1
fi
dst_git_exist=$(check_git_repo_exist $WPD_DST_GIT_PATH)
if [ $dst_git_exist -eq 0 ]; then
    echo "ERROR! Destination git repo <$WPD_DST_GIT_PATH> is not a valid GIT repository."
    exit 1
fi

# Check tags provided exists
tag_exist=$(check_git_tag_exist $WPD_TAG_TO)
if [ $tag_exist -eq 0 ]; then
    echo "ERROR! Tag <$WPD_TAG_TO> doesn't exist."
    exit 1
else
    if [ -n "$WPD_TAG_FROM"  ]; then
	tag_exist=$(check_git_tag_exist $WPD_TAG_FROM)
	if [ $tag_exist -eq 0 ]; then
	    echo "ERROR! Tag <$WPD_TAG_FROM> doesn't exist."
	    exit 1
	fi
    fi
fi

# If previous tag to integrate from not provided get the previous one ?
if [ -n "$WPD_TAG_FROM" ]; then
    previous_tag=$WPD_TAG_FROM
else
    previous_tag=$(get_previous_tag $WPD_TAG_TO)
fi

# Clean previous wpd output if any and creates new output dirs
clean_wpd_output

# Get patches from WPRD git tree
get_patches $previous_tag

# Reformat patches to meet main patch format requirements
process_patches

# Apply patches
apply_patches

# Clean wpd output
clean_wpd_output

