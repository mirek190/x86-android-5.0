# define source directory
MY_BIN_DIR=`dirname $_`
TAR_FILE=$1
TOP=`pwd`
SRC_DIR=android-froyo

if [ "x${TAR_FILE}" == "x" ]; then
    echo "ERROR: no tarfile provided"
    exit
fi

if [ ! -e ${TAR_FILE} ]; then
    echo "ERROR: tarfile does not exist"
    exit
fi

# get the android source
mkdir ${TOP}/${SRC_DIR}
cd ${TOP}/${SRC_DIR}
repo init -u git://android.git.kernel.org/platform/manifest.git -b froyo -m froyo-20100715.xml<<EOF


yes
EOF
repo sync

# untar the Intel source overtop of the Google Android source
tar -xvjf ../${TAR_FILE} --strip=1

# patch source
./patches/apply-patch.sh
