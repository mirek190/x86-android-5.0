LOCAL_SRC_FILES += \
    jpeg/EXIFMaker.cpp \
    jpeg/EXIFMetaData.cpp \
    jpeg/SWJpegEncoder.cpp \
    jpeg/ExifCreater.cpp \
    jpeg/JpegMaker.cpp \
    jpeg/ImgEncoder.cpp

ifeq ($(USE_INTEL_JPEG), true)
LOCAL_SRC_FILES += \
    jpeg/JpegHwEncoder.cpp
endif
