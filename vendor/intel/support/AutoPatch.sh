#!/bin/bash

#
# AutoPatch.sh - Intel Gerrit Patch Automation Script
#
# Copyright (C) 2013 Intel Corporation
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Learnings
# ssh -p 29418 android.intel.com gerrit query --current-patch-set is:starred	- TO LIST is:starred items

VERSION_STRING="01.30"
GERRIT_SERVER="android.intel.com"
GERRIT_GIT_URL="git://$GERRIT_SERVER"
GERRIT_USER=""

PATCH_SAVE_PATH="$HOME/Downloads"
PATCH_LIST=""
PATCH_FILE_NAME="$1"
PATCH_REMOVE_URL_PART="$GERRIT_SERVER:29418"
PATCH_NUMBER_STRING_LEN=16
PATCH_URL_REMOVAL_LIST="$GERRIT_SERVER|mcg-pri-gcr.jf.intel.com"
PATCH_BRCM_URL_REMOVAL_LIST="mcg-pri-gcr.jf.intel.com"

LINE_STRING="--------------------------------------------------------------------------------"

#PROGRAM FLOW
PATCH_MODE=""
REPO_ROOT=""
ISBRCM=0
CLEAN_PATCH_LIST=""
PROGRAM_RUN_CHECK="FALSE"

#CONSTANTS
START_PATH=$PWD
CSV_HEAD="NO"

LogMe ()
{
	LOGME_FLAG=$1
	LOGME_MESSAGE=$2
	case $LOGME_FLAG in
		"INFO")
			printf "%-100s : [\033[1;34m%s\033[0m]\n" "$LOGME_MESSAGE" " INFO "
			;;
		"WARN")
			printf "%-100s : [\033[1;35m%s\033[0m]\n" "$LOGME_MESSAGE" " WARN "
			;;
		"PASS")
			printf "%-100s : [\033[1;32m%s\033[0m]\n" "$LOGME_MESSAGE" " PASS "
			;;
		"FAIL")
			printf "%-100s : [\033[1;31m%s\033[0m]\n" "$LOGME_MESSAGE" " FAIL "
			# UnSet_ENV_Variables
			exit 1
			;;
		*)
			printf "%-100s\n" "$LOGME_FLAG"
			;;
	esac
}

TRIM_String() {
	TRIM_STRING_VALUE=""
    TRIM_STRING_VALUE="$1"
    TRIM_STRING_VALUE="${TRIM_STRING_VALUE#"${TRIM_STRING_VALUE%%[![:space:]]*}"}"   # remove leading whitespace characters
    TRIM_STRING_VALUE="${TRIM_STRING_VALUE%"${TRIM_STRING_VALUE##*[![:space:]]}"}"   # remove trailing whitespace characters
}

CheckCommandExit ()
{
	if [ $? -eq 0 ]
	then
		LogMe "PASS" "$1"
	else
		LogMe "FAIL" "$1"
	fi
}

SET_BRCM()
{
	ISBRCM=1
	GERRIT_GIT_URL="ssh://""$USER""@$PATCH_BRCM_URL_REMOVAL_LIST"
	GERRIT_SERVER="$PATCH_BRCM_URL_REMOVAL_LIST"
}

SET_INTEL()
{
	ISBRCM=0
	if [ "$GERRIT_USER" == "" ]
	then
		GERRIT_GIT_URL="git://""$GERRIT_SERVER"
	else
		GERRIT_GIT_URL="ssh://$GERRIT_USER""$PATCH_REMOVE_URL_PART"
	fi
	GERRIT_SERVER="$GERRIT_SERVER"
}

Clean_Argument()
{
	CLEAN_PATCH_LIST=""
	Clean_Argument_String="$1"
	TRIM_String "$Clean_Argument_String"
	Clean_Argument_String="$TRIM_STRING_VALUE"
	if [ "${Clean_Argument_String:0:1}" != "#" -a "$Clean_Argument_String" != "" ]
	then
		ISDIRTY=`echo "$Clean_Argument_String" | egrep "$PATCH_URL_REMOVAL_LIST"`
		if [ ! -z $ISDIRTY ]; then
			# CHECK IF / IS PRESENT AT THE END OF THE PATCH STRING.. IF NOT ADD IT
			if [[ "$ISDIRTY" != *"/" ]]
			then
				ISDIRTY="$ISDIRTY""/"
			fi

			# Extract GERRIT number.
			CLEAN_PATCH_LIST=`echo "$ISDIRTY" | awk '
			BEGIN{FS="/"}
			{
				i = NF -1
				print $i
			}'`

			ISBRCM=`echo "$Clean_Argument_String" | egrep "$PATCH_BRCM_URL_REMOVAL_LIST"`
			if [ ! -z $ISBRCM ]; then
				SET_BRCM
			else
				SET_INTEL
			fi

		else
			CLEAN_PATCH_LIST="$Clean_Argument_String"
		fi
	fi
}

Sanity_Check()
{
	if [ ! -e ".repo" ]; then
		#if [ $PATCH_MODE -lt 5 ]; then
			LogMe "FAIL" "Please RUN this Script from REPO root.."
		#fi
	fi
	REPO_ROOT="$PWD"
}

Clean_VARS()
{
	PATCH_NUMBER=0
	LAST_TWO=0
	PATCH_INFO=0
	REQ_PATCHSET=0
	PROJECT=0
	PROJEC_DIR=0
	ISPRESENT=0
	ISDIROK=0
	SUBJECT=0
	RAWDATA=0
	STATUS=""
	OWNER=""
	REVISION=""
}

IS_DIR_OK()
{
	cd $REPO_ROOT
	if [ -e $PROJECT_DIR ]; then
		ISDIROK=1
	else
		ISDIROK=0
	fi
}

IS_PATCH_Present()
{
	RAW_GIT_LOG=""
	RAW_PATCH_INFO_FROM_GERRIT=""
	cd $PROJECT_DIR
	RAW_GIT_LOG=`git log HEAD~1000..HEAD 2>/dev/null`

	HASDATA=`echo "$RAW_GIT_LOG"`
	if [ -z "$HASDATA" ]; then
		RAW_GIT_LOG=`git log`
	fi

	# FIND THE CHANGE IF ON THE GIT LOG.
	ISPRESENT=`echo "$RAW_GIT_LOG" | grep $CHANGEID`

	if [ -z "$ISPRESENT" ]; then
		ISPRESENT=0
	else
		ISPRESENT=1
	fi
	cd $REPO_ROOT
}

GET_Patch_Data()
{
	Clean_VARS;
	PATCH_NUMBER_ORIG=$1

	PATCH_NUMBER="$(echo ${PATCH_NUMBER_ORIG} | cut -d"/" -f 1)"
	if [ "$(echo ${PATCH_NUMBER_ORIG} | grep -o "/" | wc -l)" -ge "1" ]; then
		REQ_PATCHSET="$(echo ${PATCH_NUMBER_ORIG} | cut -d"/" -f 2)"
	fi

	RAW_PATCH_INFO_FROM_GERRIT=""

	# GET THE LAST TWO DIGITS OF THE PATCH
	LAST_TWO=`echo $PATCH_NUMBER| awk '{ print substr( $PATCH_NUMBER, length($PATCH_NUMBER) - 1, length($PATCH_NUMBER) ) }'`

	if [ "$PATCH_MODE" != "32" ]; then # Getting info from Gerrit

		# GO TO SOME GIT DIR, TO BE ABLE TO TALK TO GERRIT
		cd build

		RAW_PATCH_INFO_FROM_GERRIT=`ssh -p 29418 $GERRIT_USER$GERRIT_SERVER gerrit query --current-patch-set $PATCH_NUMBER`

		if [ "$RAW_PATCH_INFO_FROM_GERRIT" != "" ]
		then
			# PARSE THE INFO
			PATCH_INFO="$RAW_PATCH_INFO_FROM_GERRIT"
			LATEST_PATCHSET=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep number | tail -1 | awk '{print $2}'`
			# CHECK IF THE PATCH REALLY EXISTS
			if [ "$LATEST_PATCHSET" == "" ]
			then
				LogMe "FAIL" "Cannot get Patch Information for PATCH #$PATCH_NUMBER from Gerrit.."
			fi
			if [ $REQ_PATCHSET -lt 1 ]; then
				REQ_PATCHSET="$LATEST_PATCHSET"
			fi
			if [ $REQ_PATCHSET -gt $LATEST_PATCHSET ]; then
				LogMe "FAIL" "Cannot get Patch Information for PATCH #$PATCH_NUMBER, patchset #$REQ_PATCHSET from Gerrit.."
			fi
			PROJECT=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep "project:" | tail -1 | awk '{print $2}'`
			RAWDATA="$RAW_PATCH_INFO_FROM_GERRIT"
			PROJECT_DIR="$(repo list | grep $PROJECT$ | cut -d":" -f1)"
			CHANGEID=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep change | grep -v : | awk '{print $2}'`
			CURR_PATCHSET_ID=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep "revision:" | tail -1 | awk '{print $2}'`
			SUBJECT=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep "subject:" | awk 'BEGIN{FS="subject:"}{print $1 $2}'`
			STATUS=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep "status:" | awk 'BEGIN{FS=" "}{print $2}'`
			REVISION=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep "revision:" | awk 'BEGIN{FS=" "}{print $2}'`
			#OWNER=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep email | awk 'BEGIN{FS="email:"}{print $1 $2}'`
			OWNER=`echo "$RAW_PATCH_INFO_FROM_GERRIT" | grep -A 3 "owner:" | grep email | awk 'BEGIN{FS=" "}{print $2}'`

			# CHECK IF THE PATCH REALLY EXISTS - PARANOID CHECK
			if [ "$CHANGEID" == "" ]
			then
				LogMe "FAIL" "Cannot get Patch Information for PATCH #$PATCH_NUMBER from Gerrit.."
			fi

			cd $REPO_ROOT

			IS_DIR_OK;
			if [ $ISDIROK -eq 1 ]; then
				IS_PATCH_Present;
			fi
		fi
	else # Getting info from local saved patches
		PWD_SAVE=$PWD ; cd $LOCAL_PATCH_DIR ; LOCAL_PATCH_DIR=$PWD ; cd $PWD_SAVE
		if [ -e $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch ]; then
			PROJECT_DIR=`grep "#RepoProjectDir:" $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch | awk 'BEGIN{FS=" "}{print $2}'`
			CHANGEID=`grep "Change-Id" $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch | awk 'BEGIN{FS=" "}{print $2}'`
			SUBJECT=`grep "Subject" $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch | awk 'BEGIN{FS="Subject:"}{print $2}'`
			cd $REPO_ROOT
			IS_DIR_OK;
			if [ $ISDIROK -eq 1 ]; then
				IS_PATCH_Present;
			fi
		else
			LogMe "FAIL" "Cannot get Patch file $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch"
			exit 0
		fi
	fi
}

Run_Action()
{
	Sanity_Check;
	Clean_Argument "$1";
	if [ "$PATCH_MODE" == "" ]; then
		PATCH_MODE=1
	fi

	if [ "$CLEAN_PATCH_LIST" != "" ]
	then
		PROGRAM_RUN_CHECK="TRUE"
		GET_Patch_Data $CLEAN_PATCH_LIST;

		case $PATCH_MODE in
			0)Patch_Report;;
			64)Patch_Report_csv;;
			1)Patch_Report_Short;;
			2)Apply_Patch;;
			4)Revert_Patch;;
			8)SAVE_Patch_LOCAL;;
			10)Apply_Patch;SAVE_Patch_LOCAL;;
			16)Abandon_Patch;;
			32)Apply_Patch;;
			*)LogMe "WARN" "UNKNOWN MODE $PATCH_MODE" ;;
		esac
	fi
}

List_Starred()
{
	ssh -p 29418 $GERRIT_SERVER gerrit query --format=JSON --patch-sets is:starred | grep -Po '"url":.*?[^\\]"'
}

Apply_Patch()
{
	APPLY_PATCH_INFO=""
	cd $PROJECT_DIR

	if [ $ISPRESENT -eq 0 ]; then
		printf "%s\n" "$LINE_STRING"
		LogMe "INFO" "Applying PATCH $PATCH_NUMBER/$REQ_PATCHSET -> $PROJECT_DIR"
		printf "%s\n" "$LINE_STRING"
		if [ $PATCH_MODE == "32" ] ; then #Apply patches from local dir
			if [ ! -e  $LOCAL_PATCH_DIR ]; then
				LogMe "FAIL" "Directory $LOCAL_PATCH_DIR is not valid ; Exiting"
				exit 0
			fi
			PWD_SAVE=$PWD ; cd $LOCAL_PATCH_DIR ; LOCAL_PATCH_DIR=$PWD ; cd $PWD_SAVE
			pushd $REPO_ROOT/$PROJECT_DIR
			git am $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch
			popd
		else
			git fetch $GERRIT_GIT_URL/$PROJECT refs/changes/$LAST_TWO/$PATCH_NUMBER/$REQ_PATCHSET && git cherry-pick FETCH_HEAD
		fi
		printf "%s\n" "$LINE_STRING"

		doSync
		# CHECK IF PATCH APPLY WORKED.
		APPLY_PATCH_INFO=`git show`
		APPLY_PATCH_SUCCESS_CHECK=`echo "$APPLY_PATCH_INFO" | grep $CHANGEID`
		if [ -z "$APPLY_PATCH_SUCCESS_CHECK" ]; then
			printf "\n%s\n" "$LINE_STRING"
			LogMe "INFO" "Applying Patch# $PATCH_NUMBER FAILED, Resetting the current GIT to HEAD.."
			if [ $PATCH_MODE == "32" ] ; then
				git am --abort
				printf "Patch application error\n"
				printf "Hint: Try applying it manually with command:\n"
				printf "Hint:   pushd $REPO_ROOT/$PROJECT_DIR\n"
				printf "Hint:   git apply --binary $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch\n"
				printf "Hint:   popd\n"
				printf "Hint: Then git add . and git commit -s -m \"Applying patch $PATCH_NUMBER with changeId $CHANGEID.\"\n"
				printf "Hint: and relaunch Autopatch cmd\n"
				printf "Hint: or"
				printf "Hint: cherry-pick it from Gerrit"
				printf "Hint:  repo download -c $PROJECT $PATCH_NUMBER"
			else
				printf "Hint: Apply the patch manually and check the error message\n"
				printf "Hint:   cd $REPO_ROOT/$PROJECT_DIR\n"
				printf "Hint:   git fetch $GERRIT_GIT_URL/$PROJECT refs/changes/$LAST_TWO/$PATCH_NUMBER/$REQ_PATCHSET && git cherry-pick FETCH_HEAD\n"
			fi
			git reset --hard

			printf "%s\n" "$LINE_STRING"

			LogMe "FAIL" "ERROR APPLYING PATCH!!! ***$PATCH_NUMBER***"
		else
			LogMe "PASS" "PATCH $PATCH_NUMBER APPLIED SUCCESSFULLY to $PROJECT"
			printf "%s\n" "$LINE_STRING"
		fi
	else
		TRIM_String "$SUBJECT"
		LogMe "PASS" "[FOUND] $PATCH_NUMBER - $TRIM_STRING_VALUE"
	fi

	cd $REPO_ROOT
}

Abandon_Patch()
{
	ABANDON_PATCH_INFO=""

	printf "%s\n" "$LINE_STRING"
	LogMe "INFO" "Abandoning PATCH $PATCH_NUMBER -> $PROJECT"
	printf "%s\n" "$LINE_STRING"

	if [ "$CURR_PATCHSET_ID" != "" ]; then
		ssh -p 29418 $GERRIT_SERVER gerrit review --abandon $CURR_PATCHSET_ID
		CheckCommandExit "Abandonded Patch #$PATCH_NUMBER"
	else
		LogMe "FAIL" "PatchSet ChangeID information is NOT found to Abandon the Patch"
	fi
}

Revert_Patch()
{
	if [ $ISPRESENT -eq 0 ]; then
		LogMe "FAIL" "Patch is not applied in this env. Can NOT revert it"
	else
		cd $PROJECT_DIR
		git revert --no-edit $REVISION
		if [ $? -eq 0 ]; then
			LogMe "PASS" "Patch $PATCH_NUMBER in $PROJECT_DIR is reverted"
		else
			LogMe "FAIL" "Patch $PATCH_NUMBER NOT reverted. See above error"
		fi
		cd $REPO_ROOT
	fi
}

doSync ()
{
	sync
}

SAVE_Patch_LOCAL()
{
	if [[ $LOCAL_PATCH_DIR=="" && ! -e  $LOCAL_PATCH_DIR ]]; then
		LogMe "INFO" "Directory $LOCAL_PATCH_DIR is not valid ; Defaulting to $REPO_ROOT"
		LOCAL_PATCH_DIR=$REPO_ROOT
	fi
	cd $LOCAL_PATCH_DIR ; LOCAL_PATCH_DIR=$PWD
	cd $REPO_ROOT/$PROJECT_DIR
	LogMe "INFO" "Getting Patch ... $PATCH_NUMBER/$REQ_PATCHSET in $LOCAL_PATCH_DIR"
	git fetch $GERRIT_GIT_URL/$PROJECT refs/changes/$LAST_TWO/$PATCH_NUMBER/$REQ_PATCHSET && git format-patch -1 --stdout FETCH_HEAD > $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch
	echo "#RepoProjectDir: $PROJECT_DIR"  >> $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch
	CheckCommandExit "Get Patch $PATCH_NUMBER/$REQ_PATCHSET -> $LOCAL_PATCH_DIR/${PATCH_NUMBER}_${REQ_PATCHSET}.patch"
	cd $REPO_ROOT
}

Patch_Report()
{
	printf "%s\n" "$LINE_STRING"
	printf "PATCH       = %s\n" "$PATCH_NUMBER"
	printf "PROJECT     = %s\n" "$PROJECT"
	printf "PATCHSET    = %s\n" "$REQ_PATCHSET"
	printf "PROJECT DIR = %s\n" "$PROJECT_DIR"
	printf "ISDIROK     = %s\n" "$ISDIROK"
	printf "CHANGE ID   = %s\n" "$CHANGEID"
	printf "ISPRESENT   = %s\n" "$ISPRESENT"
	printf "STATUS      = %s\n" "$STATUS"
	printf "OWNER       = %s\n" "$OWNER"
	printf "%s\n" "$LINE_STRING"
}

Patch_Report_csv()
{
	if [ $CSV_HEAD == "NO" ]; then
		CSV_HEAD="Patch;Patchset;LatestPatchset;IsApplied;URL;BZ;ProjectDir;Status;Owner;Subject\n"
		printf "$CSV_HEAD"
	fi
	COMMIT_MSG=`ssh -p 29418 $GERRIT_SERVER gerrit query --commit-message $PATCH_NUMBER`
	BZ_NB=`echo "$COMMIT_MSG" | grep "^\s*BZ\:\s*[0-9]*\s*$" | awk 'BEGIN{FS="BZ: "}{print $2}'`
	printf "%s;" "$PATCH_NUMBER"
	printf "%s;" "$REQ_PATCHSET"
	printf "%s;" "$LATEST_PATCHSET"
	printf "%s;" "$ISPRESENT"
	printf "%s;" "http://$GERRIT_SERVER:8080/$PATCH_NUMBER "
	printf "%s;" "$BZ_NB"
	printf "%s;" "$PROJECT_DIR"
	printf "%s;" "$STATUS"
	printf "%s;" "$OWNER"
	printf "%s" "$SUBJECT"
	printf "\n"
}

Patch_Report_Short()
{
	if [ $ISPRESENT -eq 1 ]; then
		LogMe "WARN" "$PATCH_NUMBER - IS PRESENT - $SUBJECT"
	else
		LogMe "INFO" "$PATCH_NUMBER - IS NOT PRESENT - $SUBJECT"
	fi
}

Read_From_File()
{
	PATCH_FILE_NAME="$1"
	PATCH_LIST=""

	if [ -s "$PATCH_FILE_NAME" ]
	then
		PROGRAM_RUN_CHECK="TRUE"
		while read Patch_Line
		do
			if [ "${Patch_Line:0:1}" != "#" ]
			then
				PATCH_LIST+=" "$Patch_Line
			fi
		done < $PATCH_FILE_NAME

		# Call MAIN Function to Run list of Patches..
		main $PATCH_LIST
	else
		LogMe "FAIL" "Patch List File Name: $PATCH_FILE_NAME NOT Found.."
	fi
}

Usage_Help()
{
	printf "Gerrit Patch Automation Script Rev %s\n\n" $VERSION_STRING
	echo "Usage is:
	AutoPatch <OPTION> <PATCH_LIST> <OPTION> <PATCH_LIST> ...

	AutoPatch tries to administer gerrit patches on your
	local repo. it should always run from the root dir
	of the repo.

	-<OPTIONS> will apply to all patches that follow it.
	-OPTIONS are:
		-f <file>  - Get the Patch List from a file
			     Patch File can contain # infront of the Line to discard Parsing
		-prv       - Print verbose patch info
		-prx       - Print verbose patch info in csv format for excel report
		-prs       - Print short patch info
		-apply     - Try to apply patch from Gerrit
		-apply_local - Try to apply patch from local .patch file
		-abandon   - Abandond the Patch
		-revert    - Try to revert patch
		-brcm      - Patches are from brcm gerrit.
		-gp        - Get Patch - format patch in .patch file
		-patchdir  - Directory where local patch is stored or read from
		-lstarred  - List Starred Patch Numbers

	-<PATCH_LIST> is a gerrit link or gerrit patch number.
	-PATCH_LIST should contian gerrit number and can be in format of:
		http://android.intel.com:8080/#/c/67880/
		67880
		https://mcg-pri-gcr.jf.intel.com:8080/#/c/103/

	-example:
		AutoPatch.sh -prv 67871 https://mcg-pri-gcr.jf.intel.com:8080/#/c/103/ brcm 200
		AutoPatch.sh -prs 67871 67873 67874 70852
		AutoPatch.sh -apply http://android.intel.com:8080/#/c/70006/
		AutoPatch.sh -apply 70006/3
		AutoPatch.sh -apply -f Patches.txt
		AutoPatch.sh -user cbrouat -apply_local -patchdir ~/SSD/CHT/PATCHES/ 148450
	"
	exit 0;
}

SET_User()
{
	GERRIT_USER=$1+"@"
}

# MAIN State Machine
main ()
{
	while [ ! -z "$1" ]; do
		case $1 in
			-prv)
				PATCH_MODE=0
				;;
			-prx)
				PATCH_MODE=64
				;;
			-prs)
				PATCH_MODE=1
				;;
			-clean)
				REPO_BRANCH_NAME=$(date +%Y%m%d_%H%M%S)
				repo start --all $REPO_BRANCH_NAME
				CheckCommandExit "CREATING NEW REPO Branch: $REPO_BRANCH_NAME"
				doSync
				repo forall -c "git clean -d -f -x"
				CheckCommandExit "CLEAN NEW REPO Branch: $REPO_BRANCH_NAME"
				doSync
				repo forall -c "git reset --hard HEAD"
				CheckCommandExit "RESET NEW REPO Branch: $REPO_BRANCH_NAME"
				doSync
				repo sync -c -j24
				CheckCommandExit "SYNC NEW REPO Branch: $REPO_BRANCH_NAME"
				doSync
				;;
			-apply)
				PATCH_MODE=$(($PATCH_MODE+2))
				;;
			-revert)
				PATCH_MODE=4
				;;
			-brcm)
				set_brcm
				;;
			-gp)
				PATCH_MODE=$(($PATCH_MODE+8))
				;;
			-abandon)
				PATCH_MODE=16
				;;
			-apply_local)
				PATCH_MODE=32
				;;
			-patchdir)
				LOCAL_PATCH_DIR=$2; shift
				;;
			-lstarred)
				List_Starred
				exit 0
				;;
			-f)
				Read_From_File $2
				exit 0
				;;
			-DEBUG)
				DEBUG=1
				;;
			-user)
				SET_User $2; shift
				;;
			-help)
				Usage_Help
				exit 0
				;;
			--help)
				Usage_Help
				exit 0
				;;
			*)
				Run_Action $1
				;;
		esac
		shift
	done

	if [ "$PROGRAM_RUN_CHECK" == "FALSE" ]
	then
		Usage_Help
	else
		printf "\n%s\n" "$LINE_STRING"
		LogMe "PASS" "AutoPatch Exited Successfully by Parsing ALL the Patches..."
		printf "%s\n" "$LINE_STRING"
	fi
}

# PRINT THE VERSION STRING INFORMATION
printf "%s\n" "$LINE_STRING"
printf "PATCH Automation Script Rev %s\n" $VERSION_STRING
printf "%s\n\n" "$LINE_STRING"

# CALL THE MAIN FUNCTION TO CHERRY PICK THE PATCHES
main "$@";
