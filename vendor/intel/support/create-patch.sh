#!/bin/bash
#

# parse parameters
usage() {
cat <<EOF
================================
Usage:
================================
create-patch.sh [-m1 upstream manifest | -b1 upstream branch/tag/HEAD] [-m2 working manifest | -b2 working branch/tag/HEAD] [-o output directory] {upstream_rev_option1} {upstream_rev_option2} ...

================================
Example:
================================

$ create-patch.sh -m1 build-20100903-manifest.xml -b2 HEAD -o
/work/temp3

$ create-patch.sh -m1 build-20100903-manifest.xml -m2
build-20100904-manifest.xml -o /work/temp3
$ create-patch.sh -m1 .repo/manifest.xml -b2 HEAD -o
/work/temp3

$ create-patch.sh -b1 android-2.2_r1 -m2
build-20100904-manifest.xml -o /work/temp3

$ create-patch.sh -b1 android-2.2_r1 -b2 my_branch -o /work/temp3

================================
Features:
================================
(1)	generate patches for each project, so that you can git am them
        directly
(2)	archive the patches using same directory structure as android
(3)	copy the whole git tree if the that project is Intel created.
(4)	It uses "git rev-list" to find out the patches only existing in
        the working branch, and will not include the patches from mainline,
        even if the working branch is branched off older version of the mainline.
(5)	It can handle manifest.xml, branch, tag, HEAD
(6)	Add optional commit number/tag support. It can allow the users
	to append optional commit number/tag/branch to the end of the command.
	It is useful to the project created by Intel. For example , the project
	$\(KERNEL_SRC_DIR\) is newly created project, compared with the
	original Froyo code base. If we use command below, you will get a whole
	linux-2.6 copied:
	$ create-patch.sh -b1 android-2.2_r1 -m2 build-20100904-manifest.xml -o
	/work/temp3

	If you want to only see the kernel changes since v2.6.31.6, you can find
	out the commit number of v2.6.31.6  first:
	$ cd $\(KERNEL_SRC_DIR\)
	$ git log --pretty=oneline
	120f68c426e746771e8c09736c0f753822ff3f52 Linux 2.6.31.6

	And then append the commit number
	120f68c426e746771e8c09736c0f753822ff3f52 to the end of the command line:
	$ create-patch.sh -b1 android-2.2_r1 -m2 build-20100903-manifest.xml -o
	/work/temp3 120f68c426e746771e8c09736c0f753822ff3f52
	You will get the patches since V2.6.31.6, instead of the whole linux-2.6
	project.

(7)	Better output message:
	sdk: (modified)
        	0001-Add-AT-keyboard-mappings-for-a-BACK-and-MENU-key.patch
	system/core: (modified)
		0001-DBUS-is-not-fully-implemented.-Disable-it-for-now.patch
		0002-Build-the-host-version-of-libdiskconfig.patch
		0003-Disable-building-libdiskconfig-on-non-Linux-hosts.patch
		0004-Add-the-ADB-test-command-for-problems-with-early-boo.patch
		0005-Add-init.-bootmedia-.rc-support.patch
		0006-fedora-13-requires-lpthread-for-some-binaries.patch
		0007-Assembly-coded-android_memset16-and-android_memset32.patch
		0008-Add-help-message-for-continue-command.patch
		0009-Set-the-modes-for-rc.-files-in-system-etc.patch
		0010-Set-file-owner-group-permission-for-ncg.list-file.patch
		0011-Move-the-ATOM-changes-to-the-ARCH_VARIANT.patch
		0012-Fix-line-numbers-in-init-error-messages.patch
		0013-support-debuggerd-for-x86.patch
		0014-VME-rel-1-upd-1.patch
	system/vold: (modified)
       		0001-Avoid-array-overrun.-We-can-now-mount-the-sdcard-pa.patch
	device/intel/aboot: (created)
       		copied
	device/intel/firmware: (created)
       		copied

================================
Use it for non-Android projects:
================================

This script can also be used for any git tree, not just
Android projects. And you can use it to find out the patches which exist
only on the branch (some of the patches exist on both mainline and
branch when you try to merge mainline into branch using git merge).

For example, for the google android kernel 2.6.35:
google starts android-2.6.35 from v2.6.35-rc6 for a while, and then
merge v2.6.35 into it, so its tree is not a simple linear tree.

You will get both android specific patches and patches between
v2.6.35-rc6 and v2.6.35, if you just use following commands:
$ git format-patch v2.6.35-rc6..origin/android-2.6.35

If you want to get Android specific patches:
$ git clone
git://android.git.kernel.org/kernel/common.git
$ git remote add torvalds
git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux-2.6.git
$ git fetch torvalds
$ mkdir .repo
$ echo "." .repo/project.list
$ create-patch.sh -b1 torvalds/master -b2 origin/android-2.6.35 -o /tmp/android-2.6.35-patches

Then you will get all android specific patches under
folder /tmp/android-2.6.35-patches, the patches between v2.6.35-rc6 and
v2.6.35 will be excluded.
EOF
}

generate_patch()
{
	git_repo=$1
	upstream=$2
	branch=$3

        #echo "handling $git_repo($upstream..$branch) ..."

	cd $top_dir/$git_repo

	git rev-list ^$upstream $branch > /dev/null
	if [ $? -ne 0 ];then
		upstream=
		# try revision options if not found
		for upstream_rev_option in $upstream_rev_options;do
			git rev-list ^$upstream_rev_option $branch > /dev/null
			if [ $? -eq 0 ];then
				upstream=$upstream_rev_option
				break;
			fi
		done
	fi

	if [ -z $upstream ];then
		mkdir -p $output/$git_repo
		echo "$git_repo: (created)"
		echo "	copied"
		cp -fr $top_dir/$git_repo/* $output/$git_repo
		cp -fr -L $top_dir/$git_repo/.git $output/$git_repo
		continue
	fi

	# get revision list
	revs=`git rev-list ^$upstream $branch --no-merges --reverse`
        if [ -z "$revs" ];then
		continue;
	fi
	echo "$git_repo: (modified)"
	# generate patches
	#echo generating patchs for $git_repo...
	mkdir -p $output/$git_repo
	count=10000;
	rm -fr 0001*.patch
	for rev in $revs;do
		git format-patch $rev^..$rev > /dev/null

		count=`expr $count + 1`
		oldpatch=`ls 0001*.patch`;
		newpatch=`echo $oldpatch|sed s/0001/$count/g|sed s/^1//g`;
		mv $oldpatch $output/$git_repo/$newpatch
		echo "	$newpatch"
	done
}

get_revision()
{
	if [ ! -z "$manifest" ] && [ -f $manifest ];then
		revision=`grep -v "\!--" $manifest|grep "\"$git_repo\""|grep "revision"|sed 's/.*revision="//g'|sed 's/".*//g'`
		remote=`grep -v "\!--" $manifest|grep "\"$git_repo\""|grep "remote"|sed 's/.*remote="//g'|sed 's/".*//g'`;
		if [ -z "$remote" ];then
			grep "project path=" $manifest > /dev/null
			if [ $? -eq 0 ];then
				# branch style
				remote=$default_remote
			fi
		fi
		if [ -z $revision ];then
			revision=$default_revision
		fi
		if [ ! -z $remote ];then
			commit=$remote/$revision
		else
			commit=$revision
		fi
	else
		commit=$branch
	fi
}

if [ $# -lt 6 ];then usage;exit;fi

# redirect all err to a log
exec 2> /tmp/create-patch.err

while getopts hm:b:o: opt
do
	case "${opt}" in
	h)
		usage
		exit 0;
		;;
	m)
		if [ "${OPTARG}" -eq "1" ];then
			last_manifest=$2
		else
			curr_manifest=$2
		fi
		OPTIND=0
		shift 2
		;;
	b)
		if [ "${OPTARG}" -eq "1" ];then
			last_branch=$2
		else
			curr_branch=$2
		fi
		OPTIND=0
		shift 2
		;;
	o)
		output="${OPTARG}"
		;;
	?)
		echo "${OPTARG}"
		;;
	esac
done
shift $(($OPTIND-1))
upstream_rev_options=$*

if [ ! -e $last_manifest ];then
	echo "$last_manifest not existed"
	exit 1
fi
if [ ! -e $curr_manifest ];then
	echo "$curr_manifest not existed"
	exit 1
fi

# get repo list
if [ ! -z "$curr_manifest" ];then
	git_repos=`grep -v "\!--" $curr_manifest|grep " path="|sed 's/.* path=\"//g'|sed 's/\".*//g'`
else
	git_repos=`cat .repo/project.list`
fi
top_dir=`pwd`

mkdir -p $output
cd $output
output=`pwd`
cd $top_dir

if [ ! -z "$last_manifest" ];then
	default_upsteam_revision=`grep -v  "\!--" $last_manifest|grep "default revision"|sed 's/.*<default revision="//g'|sed 's/" .*//g'`;
	default_upsteam_remote=`grep -v  "\!--" $last_manifest|grep "default revision"|sed 's/.*remote="//g'|sed 's/".*//g'`;
fi
if [ ! -z "$curr_manifest" ];then
	default_branch_revision=`grep -v  "\!--" $curr_manifest|grep "default revision"|sed 's/.*<default revision="//g'|sed 's/" .*//g'`;
	default_branch_remote=`grep -v  "\!--" $curr_manifest|grep "default revision"|sed 's/.*remote="//g'|sed 's/".*//g'`;
fi
for git_repo in $git_repos;do
	cd $top_dir

	#echo git_repo: $git_repo...
	manifest=$last_manifest
	branch=$last_branch
	default_remote=$default_upsteam_remote
	default_revision=$default_upsteam_revision
	commit=
	get_revision
	upstream=$commit
	if [ -z $upstream ];then
		upstream=null;
	fi
	#echo upstream: $upstream

	manifest=$curr_manifest
	branch=$curr_branch
	default_remote=$default_branch_remote
	default_revision=$default_branch_revision
	commit=
	get_revision
	branch=$commit
	#echo branch: $branch

        generate_patch $git_repo $upstream $branch
done

