#!/bin/bash
# 
# Tool to diagnose common problems for the repo tool and builds
# 
# This is mostly experimental at this point.  Email any comments to 
# geoffx.smith@intel.com
#

version=1.02r
tmpf=/tmp/`basename $0`.$$.tmp


show_help()
{
cat <<__
NAME
	troubleshoot.sh - troubleshoot MCG build environment

SYNOPSIS
        ./troubleshoot.sh [--version] [--check buildtime|all]

DESCRIPTION
       This tool diagnoses common problems in your build environment that
       may impair your ability to use the repo tool or perform builds

       If any checks fail, please look into the one-time setup instructions
       at http://mcgwiki.intel.com/wiki/?title=UMSE_One-Time_Setup

OPTIONS
       --check buildtime|all

       Perform only specific checks

            buildtime -- Only checks that might impair building.  Do not
                         perform other checks such as connectivity
            all       -- Chjeck everything (default)

       --version

       Just print the version number and quit
__
}

# 1 = buildtime
# 2 = connectivity
# 8 = target control
enabled_checks=15

while [ $# -gt 0 ]; do
	case "$1" in
	--help|-help|--h|-h)
		show_help ; exit 0 ;;
	--version|-V)
		echo $version
		exit 0
		;;
	--check)
		shift
		case "z$1" in
			zbuildtime)
				enabled_checks=1
				;;
			zall)
				enabled_checks=15
				;;
		esac
		;;
	-*|+*)
		echo ""
		echo "ERROR - unrecognized switch \"$1\""
		echo "" ; show_help ; exit 1 ;;
	*)
		echo ""
		echo "ERROR - unrecognized argument \"$1\""
		echo "" ; show_help ; exit 1 ;;
	esac
	shift
done

export scc_server=android.intel.com
export gerrit_server=${scc_server}:8080
export scc_git_server=android.intel.com
export mcgwiki=mcgwiki.intel.com/wiki
export umgbugzilla=umgbugzilla.sh.intel.com


echo ""
echo "Troubleshooter v. ${version} for MCG SCC"
echo ""
echo "This tool diagnoses common problems in your build environment that"
echo "may impair your ability to use the repo tool or perform builds"
echo ""
echo "If any checks fail, please look into the one-time setup instructions."
echo "at http://mcgwiki.intel.com/wiki/?title=UMSE_One-Time_Setup"
echo ""


errors_found=0
warnings_found=0

error_found() {
        echo ""
        errors_found=$(( $errors_found + 1 ))
}

warning_found() {
        echo ""
        warnings_found=$(( $warnings_found + 1 ))
}

echo === Basic checks
echo ""

if [ -n "$EUID" ]; then
        if [ "$EUID" -eq 0 ]; then
          warning_found
          echo "WARNING - Running as root."
          echo "          Please re-run this script under the username you"
          echo "          will use for builds.  If you will use multiple"
          echo "          Linux usernames, run this script under each one."
          echo ""
        fi
else
        tmp=`whoami`
        if [ "$tmp" = "root" ]; then
          warning_found
          echo "WARNING - Running as root."
          echo "          Please re-run this script under the username you"
          echo "          will use for builds.  If you will use multiple"
          echo "          Linux usernames, run this script under each one."
          echo ""
        fi
fi

echo Check distribution
distro=
if [ ! -f /etc/issue ]; then
        error_found
        echo "ERROR - Cannot determine distribution"
        echo "        This machine is not running a supported distribution"
else
        grep Ubuntu /etc/issue > /dev/null
        if [ $? -eq 0 ]; then
                distro=ubuntu
                distro_version=`awk 'NR==1{print $2}' /etc/issue`
                distro_major=`echo $distro_version | cut -f1 -d.`
                distro_minor=`echo $distro_version | cut -f2 -d.`
                if [ "$distro_major" -lt 10 -o "$distro_major" -gt 11 ]; then
                  warning_found
                  echo "WARNING - Not a supported Ubuntu distribution ($distro_version)"
                  echo "          This should work, but 10.04.2 or 11.04 is recommended"
                fi
        else
          grep Fedora /etc/issue > /dev/null
          if [ $? -eq 0 ]; then
                distro=fedora
                if [ "`awk 'NR==1{print $3}' /etc/issue`" != "12" ]; then
                  warning_found
                  echo "WARNING - Not a supported Fedora distribution"
                  echo "          This should work, but Fedora 12 is rcommended"
                fi
          else
                error_found
                echo "ERROR -   Not a supported distribution"
                echo "          The following are recommended instead:"
                echo "          -  Ubuntu 9.10"
                echo "          -  Fedora 12"
          fi
        fi
fi

echo Check host arch
case "x`uname -m`" in
	xx86_64)
		# Current recommended is x86_64
		;;
	xi[356]86)
		warning_found
		echo -n "WARNING - ${distro}/`uname -p` is not known to "
		echo "be a good host configuration"
		echo ""
		;;
	*)
		error_found
		echo "ERROR - This host architecture is not known to work."
		echo "        Proceed with great caution"
		echo ""
		;;
esac

echo "Check curl"
curl_ok=1
rm -r -f $tmpf
curl --version > $tmpf
if [ ! -s "$tmpf" ]; then
        error_found
        echo "ERROR - curl does not appear to be installed"
        echo "        Use yum install curl or apt-get install curl"
        echo ""
        curl_ok=0
fi


echo Check proxy variables
if [ -n "$HTTP_PROXY$http_proxy" ]; then
        if [ -z "$no_proxy" ]; then
                        error_found
                        echo "ERROR - proxy problem"
                        echo ""
                        echo "        You must set no_proxy if HTTP_PROXY or http_proxy are set"
                        echo "        You need to disable intel.com URLs from proxy settings"
                        echo ""
                        echo "        Examine no_proxy=. The recommended value is"
                        echo "        export no_proxy=localhost,.intel.com,10.0.0.0/8,127.0.0.0/8"
                        echo ""
                        curl_ok=0
        fi
fi

echo Check no_proxy
if [ -n "$no_proxy" ]; then
        echo $no_proxy | grep '*.intel.com' > /dev/null
        if [ $? -eq 0 ]; then
                error_found
                echo "ERROR - Proxy problem"
                echo ""
                echo "        Environment variable no_proxy= includes"
                echo "        *.intel.com, but this syntax is not valid."
                echo "        Use '.intel.com' instead (just remove the asterisk)"
                echo ""
                curl_ok=0
        fi
fi

if [ $(( $enabled_checks & 2 )) -ne 0 ]; then
git --version > /dev/null
git_ok=$(( $? == 0 ))
if [ $git_ok -ne 0 ]; then
        tmp=`git config --global user.name`
        if [ -z "$tmp" ]; then
                warning_found
                echo "WARNING - git user.name is not set for the current userid"
                echo "          This is not required, but makes \"repo init\""
                echo "          easier to run."
                echo "          (git config --global user.name \"John Doe\")"
                echo ""
        fi
        tmp=`git config --global user.email`
        case x$tmp in
           x)
                warning_found
                echo "WARNING - git user.email is not set for the current userid"
                echo "          This is not required, but makes \"repo init\""
                echo "          easier to run."
                echo "          (git config --global user.email john@intel.com)"
                echo ""
                ;;
           *@intel.com)
                ;;
           *)
                error_found
                echo "ERROR - git user.email ($tmp) is not an Intel email address"
                echo "        This will make life difficult."
                echo "        (git config --global user.email foo@intel.com)"
                echo ""
                ;;
        esac
fi

fi

if [ $(( $enabled_checks & 0x2 )) -eq 0 ]; then
	curl_ok=0
	git_ok=0
fi

if [ $curl_ok -ne 0 ]; then
        echo "Check proxy"
        echo "        Caveat: This only checks proxy for command-line tools"
        echo "        Browsers use their own mechanism (under Gnome, see"
        echo "        System -> Preferences -> Network proxy)"
        echo ""
        curl --silent --max-time 30 http://${scc_server}/repo > $tmpf
        trial_normal=$?
        curl --silent --max-time 30 --noproxy intel.com http://${scc_server}/repo > $tmpf.2
        trial_noproxy=$?
        if [ $trial_normal -eq 0 -a $trial_noproxy -eq 0 ]; then
                  diff $tmpf $tmpf.2 > /dev/null
                  if [ $? -ne 0 ]; then
                        error_found
                        echo "ERROR - proxy problem"
                        echo ""
                        echo "        You need to disable intel.com URLs"
                        echo "        from proxy settings"
                        echo ""
                        echo "        Examine no_proxy=. The recommended"
                        echo "        value is"
                        echo "        export no_proxy=localhost,.intel.com,10.0.0.0/8,127.0.0.0/8"
                  else
                        echo "        OK"
                  fi
                  echo ""
        fi
        if [ $trial_noproxy -eq 0 -a $trial_normal -ne 0 ]; then
                        error_found
                        echo "ERROR - proxy problem"
                        echo ""
                        echo "        You need to disable intel.com URLs"
                        echo "        from proxy settings"
                        echo ""
                        echo "        Examine no_proxy=. The recommended"
                        echo "        value is"
                        echo "        export no_proxy=localhost,.intel.com,10.0.0.0/8,127.0.0.0/8"
                        echo ""
                        curl_ok=0
        fi
        rm -r -f $tmpf.2
fi

if [ $(( $enabled_checks & 2 )) -ne 0 ]; then
if [ $curl_ok -ne 0 ]; then
        echo "Check http to $scc_server (MSC SCC home)"
        curl --silent --max-time 20 http://${scc_server}/repo > $tmpf
        if [ $? -ne 0 ]; then
            error_found
            echo "ERROR - curl could not get http://${scc_server}/repo"
            echo "        (could be proxy problem)"
            echo "        (disabling further URL checks)"
            curl_ok=0
        else
            tmp=`head -1 $tmpf | grep -c '/bin/sh'`
            if [ "$tmp" != "1" ]; then
                error_found
                echo "ERROR - downloaded repo is not a script"
                grep HEAD $tmpf
                if [ $? -eq 0 ]; then
                        echo "        (could be a proxy problem)"
                fi
            fi
        
            if [ -f /bin/repo ]; then
                diff /bin/repo $tmpf > /dev/null
                if [ $? -ne 0 ]; then
                        error_found
                        echo "ERROR - /bin/repo is not the one from $scc_server"
                        echo "        You might have official Android one,"
                        echo "        but that one will not work for MCG."
                        echo ""
                        echo "        To fix this, download"
                        echo "        http://$scc_server/repo and copy it to"
                        echo "        /bin/repo"
                        echo ""
                fi
            fi
        fi
fi

echo "Check git to $scc_git_server (straight git://)"
git ls-remote --heads git://${scc_git_server}/manifest > /dev/null
if [ $? -ne 0 ]; then
        error_found
        echo "ERROR - Cannot access git server"
        echo "        Command was git ls-remote git://${scc_git_server}/manifest"
        echo "        (Verify server is up, or you may have proxy problems)"
        echo ""
        
	if [ -n "$GIT_PROXY_COMMAND" ]; then
		if [ ! -r "$GIT_PROXY_COMMAND" ]; then
		    error_found
		    echo "ERROR - \$GIT_PROXY_COMMAND set but does not exist"
		    echo "        GIT_PROXY_COMMAND=$GIT_PROXY_COMMAND"
		    echo ""
		elif [ ! -x "$GIT_PROXY_COMMAND" ]; then
		    error_found
		    echo "ERROR - \$GIT_PROXY_COMMAND set, but the file"
		    echo "        \"${GIT_PROXY_COMMAND}\" is not executable"
		    echo "        GIT_PROXY_COMMAND=$GIT_PROXY_COMMAND"
		    echo ""
		fi

                warning_found
                echo "WARNING - \$GIT_PROXY_COMMAND is set, make sure"
                echo "          it does not use proxy for .intel.com addresses."
                echo "          (failure mode is that repo sync fails to get projects)"
		echo ""
		echo "          Here is an example of one that works:"
		echo "          case \$1 in"
		echo "            *.intel.com|10.*|192.168.*|127.*|localhost)"
		echo "               nc \$*"
		echo "               ;;"
		echo "            *)"
		echo "               nc -xproxy-socks.jf.intel.com -X5 \$*"
		echo "               ;;"
		echo "          esac"
		echo ""
        fi
fi
fi

if [ $curl_ok -ne 0 ]; then
        echo "Check http to $gerrit_server (Gerrit server)"
        curl --silent --max-time 20 http://${gerrit_server}/ssh_info > $tmpf
        if [ $? -ne 0 ]; then
            error_found
            echo "ERROR - curl could not get http://${scc_server}/ssh_info"
            echo "        (could be proxy problem)"
            echo ""
        else
            tmp=`awk 'NR==1' $tmpf | grep -c 29418`
            if [ "$tmp" != "1" ]; then
                error_found
                echo "ERROR - downloaded ssh_info not correct"
                echo "        (could be a proxy problem)"
                echo ""
            fi
        fi
fi

# This step seems to be broken... at least for me.
# you need to have a cookie to log into it... so how did it ever work?
#if [ $curl_ok -ne 0 ]; then
#        echo "Check http to MCG wiki"
#        curl --silent --max-time 20 http://${mcgwiki} > $tmpf
#        if [ $? -ne 0 ]; then
#            error_found
#            echo "ERROR - curl could not get http://${mcgwiki}"
#            echo "        (could be proxy problem)"
#        else
#            echo "        OK, can connect to http://${mcgwiki}"
#            grep "401 Unauthoriz" $tmpf > /dev/null
#            if [ $? -eq 0 ]; then
#                warning_found
#                echo ""
#                echo "WARNING - Wiki reports that you are not authorized."
#                echo "          This is OK if you use a different machine to use the wiki."
#                echo "          But, you do need access, so be sure you browse to the site"
#                echo "          and request access if you do not already have it"
#                echo ""
#                echo "              http://${mcgwiki}"
#                echo ""
#            fi
#        fi
#fi

echo "Check repo"
f_effective=""
found=0
for i in `echo $PATH | tr ':' ' '` ; do
        if [ -f "${i}/repo" -a "${i}/repo" != "$f_effective" ]; then
                found=$(( $found + 1 ))
                if [ $found -eq 1 ]; then
                        f_effective=${i}/repo
                fi
                if [ ! -x "${i}/repo" ]; then
                    error_found
                    echo "ERROR - repo found in path, at ${i}/repo,"
                    echo "        but it is not executable"
                fi

                grep android.intel.com "${i}/repo" > /dev/null
                if [ $? -ne 0 ]; then
                    error_found
                    echo "ERROR - repo found in path, at ${i}/repo,"
                    echo "        but it is not a MCG version."
                    echo "        This version will not work for MCG development"
                fi
        fi
done
if [ $found -eq 0 ]; then
        error_found
        echo "ERROR - repo not found in path"
        echo ""
        echo "        To fix this problem:"
        echo "          curl http://${scc_server}/repo > /bin/repo"
        echo "          chmod +x /bin/repo"
elif [ $found -gt 1 ]; then
	warning_found
        echo "WARNING - repo found multiple times in your path"
        echo "          Only the one at $f_effective is effective"
        echo ""
        echo "          Reccomendation: repo should only be in either"
        echo "          - /bin           - or -"
        echo "          - $HOME/bin"
        echo "          Use \$HOME/bin if you also do upstream Android development"
        echo ""
fi

echo "Check \$HOME/bin"
if [ -n "$HOME" ]; then
        if [ ! -d "$HOME/bin" ]; then
            warning_found
            echo "WARNING - No ${HOME}/bin directory."
            echo "          Several steps in setup assume that this directory exists."
            echo ""
        else
         if [ ! -d ~/bin ]; then
            warning_found
            echo "WARNING - No ~/bin directory."
            echo "          Several steps in setup assume that this directory exists."
            echo ""
         fi
        fi
fi

if [ -n "$HOME" ]; then
  echo $PATH | grep $HOME/bin > /dev/null
  if [ $? -ne 0 ]; then
        warning_found
        echo "WARNING - $HOME/bin is not found in your path"
        echo "          Several steps in setup assume that this directory"
        echo "          will be on your path."
        echo ""
  fi
fi


if [ $(( $enabled_checks & 2 )) -ne 0 ]; then
ssh_config_ok=1
echo Check ssh config
if [ ! -f ~/.ssh/config ]; then
        error_found
        echo "ERROR - ~/.ssh/config file not found, this must be set per the"
        echo "        one-time setup instructions"
        echo "        Refer to 'setup' under developer documentation"
        echo "        from http://${scc_server}"
        echo ""
        if [ "`whoami`" = "root" ]; then
          echo "        (But, you are running as root.  You probably just"
          echo "        want to re-run as a regular user."
        fi
        ssh_config_ok=0
else
        malformed=0
        grep 29418 ~/.ssh/config > /dev/null
        if [ $? -eq 1 ]; then
                malformed=1
        fi
        grep "$scc_server" ~/.ssh/config > /dev/null
                if [ $? -eq 1 ]; then
                malformed=1
        fi
        if [ $malformed -ne 0 ]; then
          error_found
          echo "ERROR - ~/.ssh/config file is malformed, this must be set per the"
          echo "        one-time setup instructions"
          echo "        Refer to 'setup' under developer documentation"
          echo "        from http://${scc_server}"
          ssh_config_ok=0
        fi
fi

echo Check ssh to Gerrit
if [ $ssh_config_ok -ne 0 ]; then
        ssh -o ConnectTimeout=20 ${scc_server} gerrit ls-projects > /dev/null
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Cannot connect to Gerrit server"
                echo "        Failing command is:"
                echo "          ssh ${scc_server} gerrit ls-projects"
                echo "        Typically, the problem is either:"
                echo "        o   an invalid or missing ~/.ssh/config file"
                echo "        o   did not create account on ${gerrit_server}"
                echo "        o   did not enter ssh key on ${gerrit_server}"
                echo ""
                ssh_config_ok=0
        fi
fi

if [ $ssh_config_ok -ne 0 ]; then
        echo Check git+ssh via Gerrit
        git ls-remote --heads ssh://${scc_git_server}/manifest > /dev/null
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Cannot access git+ssh via Gerrit"
                echo "        - If straight ssh check passes, then something may be wrong"
                echo "          with Gerrit.  Consult a sysadmin for ${scc_git_server}"
                echo ""
        fi
fi
fi

if [ -d ~/.repo ]; then
        error_found
        echo "ERROR - There is a (hidden) .repo directory in your home directory"
        echo "        This directory should only exist in a workspace"
        echo "        and you cannot nest workspaces"
        echo ""
        echo "        Recommendation: delete it (rm -r -f ~/.repo)"
        echo ""
fi

echo Check disk space
tmp=`df -k . | awk 'NR==2{print $4}'`
if [ -n "$tmp" ]; then
        recommended_free=6
        if [ $tmp -lt $(( $recommended_free * 1024 * 1024)) ]; then
                warning_found
                echo "WARNING - Disk space is low"
                echo "          Recommend having ${recommended_free}GB free"
                echo ""
        fi
fi


echo ""
echo "===  Android-specific checks"
echo "        The following checks can be ignored if you are not doing Android"
echo "        builds."
echo ""

echo Check bash
if [ -r /bin/bash -a -r /bin/dash ]; then
        if [ "`stat -c%s /bin/bash`" == "`stat -c%s /bin/dash`" ]; then
           error_found
           echo "ERROR - /bin/bash is a soft link to /bin/dash"
           echo "        This is a setting that sometimes occurs on Ubuntu"
           echo "        systems.  However, this will not work for Android"
           echo "        development"
           echo ""
        fi
fi

echo Check misc tools
for i in awk gcc python install grep gperf git make makeinfo; do
        echo '-' Check $i
        $i --version  >& $tmpf
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Problem running $i, make sure it is installed"
        fi
done
for i in ant ; do
        echo '-' Check $i
        $i -version >& $tmpf
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Problem running $i, make sure it is installed"
        fi
done
echo '-' Check dos2unix
dos2unix -V >& $tmpf
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Problem running dos2unix, make sure it is installed"
                echo "        Ubuntu: apt-get install tofrodos"
                echo "        Fedora: yum install dos2unix"
                echo ""
                if [ "$distro" = "ubuntu" ]; then
                  echo "        If tofrodos *is* installed, you may need to create a link"
                  echo "        from dos2unix.  Newer updates to tofrodos do not include"
                  echo "        the tool with its hsitorical name (which is the name we need)"
                  echo "        # ln -s /usr/bin/fromdos /usr/bin/dos2unix"
                  echo ""
                fi
        fi

echo Check installed packages
case x$distro in
        x)
                echo "Skipping packages checks" ;;
        xubuntu)
                # What is equiv of wxGTK-devel on Ubuntu?
                dpkg -l > $tmpf
                for pkg in \
                  install git-core python gcc patch zip \
                  gawk texinfo ant gperf \
                  tofrodos libncurses5-dev \
                ; do
                        grep $pkg $tmpf > /dev/null
                        if [ $? -ne 0 ]; then
                          error_found
                          echo "ERROR - Package $pkg is not installed"
                          echo "        (# apt-get install $pkg)"
                          echo ""
                        fi
                done
                ;;
        xfedora)
                for pkg in \
                  install curl git-core python gcc gcc-c++ patch zip flex \
                  gawk texinfo ant gperf wxGTK-devel \
                  dos2unix make \
		  glibc.i686 glibc-devel.i686 libstdc++-devel.i686 \
		  libstdc++.i686 zlib-devel.i686 ncurses-devel.i686 \
		  readline-devel.i686 libgcc.i686 wxGTK-devel.i686 \
                ; do
                        echo '-' Checking package $pkg
                        tmp=`rpm -q $pkg `
                        if [ -z "$tmp" ]; then
                          error_found
                          echo "ERROR - Package $pkg is not installed"
                          echo "        (# yum install $pkg)"
                        fi
                done
                ;;
esac

echo Check java version
if [ -z "$JAVA_HOME" ]; then
        error_found
        echo ""
        echo "ERROR - JAVA_HOME is not set, it must point to Java 1.6.0"
        echo ""
        echo "        Recommend you add this to ~/.bashrc"
else
        if [ ! -d "$JAVA_HOME" ]; then
                error_found
                echo "ERROR - JAVA_HOME does not exist"
                echo "        (\${JAVA_HOME=$JAVA_HOME})"
        elif [ ! -x "${JAVA_HOME}/bin/javac" ] ; then
                error_found
                echo "ERROR - Cannot find \$JAVA_HOME/bin/javac"
                echo "        or it is not executable"
        else
            $JAVA_HOME/bin/javac -version 2> $tmpf
            grep 1.6.0 $tmpf > /dev/null
            if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - JAVA_HOME does not point to Java 1.6.0"
            else
		jdk_revision=`echo ${JAVA_HOME} | cut -f2 -d_`
		echo "JDK 1.6.0 revision is $jdk_revision"
		if [ "$jdk_revision" -lt 22 ]; then
                    warning_found
                    echo "WARNING - JDK 1.6.0 revision is $jdk_revision"
                    echo "          1.6.0_22 or later is recommended"
                fi
            fi
        fi
fi

if [ "$CLASSPATH" != "." ]; then
        error_found
        echo "ERROR - CLASSPATH env variable is not ."
        echo ""
        echo "        Recommend you add this to ~/.bashrc"
        echo "        export CLASSPATH=."
fi

if [ -z "$ANT_HOME" ]; then
        error_found
        echo "ERROR - ANT_HOME is not set"
        echo ""
        echo "        Recommend you add this to ~/.bashrc"
        echo "        export ANT_HOME /usr/share/ant"
fi

if [ ! -d /usr/share/ant ]; then
        error_found
        echo "ERROR - Ant is not installed in /usr/share/ant"
        echo ""
        echo "        Install the package \"ant\""
fi

echo ""
echo ""
echo "        The following checks can be ignored if you are not running"
echo "        a target machine from this machine"
echo ""

if [ $(( $enabled_checks & 0x8 )) -ne 0 ]; then
echo "Check fastboot"
if [ -f "${HOME}/bin/fastboot" ]; then
                if [ ! -x "${HOME}/bin/fastboot" ]; then
                    error_found
                    echo "ERROR - fastboot found in ${HOME}/bin/fastboot,"
                    echo "        but it is not executable"
                fi
else
        warning_found
        echo "WARNING - fastboot not found in {HOME}/bin"
        echo "          To fix, copy it to ~/bin/fastboot after you have a"
        echo "          successful build"
        echo ""
fi
echo "Check adb"
if [ -f "${HOME}/bin/adb" ]; then
                if [ ! -x "${HOME}/bin/adb" ]; then
                    error_found
                    echo "ERROR - adb found in ${HOME}/bin/adb,"
                    echo "        but it is not executable"
                fi
else
        warning_found
        echo "WARNING - adb not found in {HOME}/bin"
        echo "          To fix, copy it to ~/bin/adb after you have a"
        echo "          successful build"
        echo ""
fi

if [ ! -f /etc/udev/rules.d/51-android.rules ]; then
        error_found
        echo "ERROR - /etc/udev/rules.d/51-android.rules not found"
        echo "        (File might exist and just not be visible to current user)"
        echo ""
        if [ ! -f /etc/udev/rules.d/fastboot.rules ]; then
         echo "        Note: Alternative name of fastboot.rules is no longer suggested"
         echo ""
        fi
fi
fi


echo ""
echo "===  Moblin/MeeGo specific checks"
echo "        The following checks can be ignored if you are not doing "
echo "        Moblin or MeeGo builds."
echo ""

echo Check misc tools
for i in awk gcc python git make patch ; do
        echo '-' Check $i
        $i --version  >& $tmpf
        if [ $? -ne 0 ]; then
                error_found
                echo "ERROR - Problem running $i, make sure it is installed"
        fi
done

if [ $errors_found -gt 0 ]; then
	echo ""
	echo "   Errors found, please repair"
	echo "   It is more productive to tackle the earlier ones first"
fi
echo ""
echo "$errors_found errors, $warnings_found warnings"
echo "(end)"

# clean up
rm -r $tmpf

exit $errors_found

