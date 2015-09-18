
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#define LOG_TAG "usb3gmonitor"
#include <cutils/log.h>
struct uevent 
{
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    const char *devtype;
	const char *devname;
    int major;
    int minor;
};

#define LIB_PATH_MODEM_PORT_PROPERTY   "ril.datachannel"
#define LIB_PATH_AT_PORT_PROPERTY   "ril.atport"
#define LIB_PATH_START_PORT_PROPERTY   "ril.startport"
#define LIB_PATH_PROPERTY   "rild.libpath"
#define VENDOR_INFO "dongle.vendorinfo"

static char usb_vid[16],usb_pid[16];
static int port_array[5] = {0, 0, 0, 0, 0};

int get_vid_pid(char * path)
{
	int fd,len;
	char usb_path[0xFF] = {0};
	
	memset(usb_vid,0,sizeof(usb_vid));
	memset(usb_pid,0,sizeof(usb_pid));
	
	memset(usb_path,0,sizeof(usb_path));
	strcat(usb_path,path);
	strcat(usb_path,"/idVendor");
	fd=open(usb_path,O_RDONLY);
	len=read(fd,usb_vid,sizeof(usb_vid));
	close(fd);
		
	if(len<=0) {
		ALOGE("Vid :err\n");
		return -1;	
	}	

	usb_vid[len-1] = 0;

	memset(usb_path,0,0xFF);
	strcat(usb_path,path);
	strcat(usb_path,"/idProduct");
	fd=open(usb_path,O_RDONLY);
	len=read(fd,usb_pid,sizeof(usb_pid));
	close(fd);
		
	if(len <= 0) {	
		return -1;
	}	

	usb_pid[len-1] = 0;
	ALOGI("{VID '%s' PID '%s'}\n", usb_vid, usb_pid);
	return 0;
}
static void handleUsbFirstEvent()
{
 	int ret = 0;
	char *p,*cmd = NULL;
	ALOGI("handleUsbFirstEvent\r\n");
	ret = get_vid_pid("/sys/devices/pci0000:00/0000:00:14.0/usb1/1-1");
		
    if((ret < 0)||(usb_pid == NULL)||(usb_vid == NULL)) {
		ALOGI("(ret is %d,)||(usb_pid is 0x%x )||(usb_vid is 0x%x)",ret,strtoul(usb_pid, 0, 16), strtoul(usb_vid, 0, 16));
        	return;	
    }

		
		asprintf(&cmd, "chmod 777 /dev/bus/usb/001/004"); 
        system(cmd);
        
	   if(!strncmp(usb_vid, "19d2", 4) && (!strncmp(usb_pid, "fff5", 4))){
		  asprintf(&cmd, "usb_modeswitch -W -c /etc/usb_modeswitch.conf"); 
		  goto EXCUTE1;   
        }	
		if(!strncmp(usb_vid, "05c6", 4) && !strncmp(usb_pid, "1000", 4))
	      asprintf(&cmd, "/system/xbin/usb_modeswitch -v %04x -p %04x -W -c /system/etc/usb_modeswitch.d/%s.%s.uMa\=StrongRising &", strtoul(usb_vid, 0, 16), strtoul(usb_pid, 0, 16), usb_vid,usb_pid);
		else		
		 asprintf(&cmd, "/system/xbin/usb_modeswitch -v %04x -p %04x -W -c /system/etc/usb_modeswitch.d/%s.%s &", strtoul(usb_vid, 0, 16), strtoul(usb_pid, 0, 16), usb_vid,usb_pid);
EXCUTE1:
		
        ret = system(cmd);

        free(cmd);
return;
}

static void handleUsbEvent(struct uevent *evt)
{

    const char *devtype = evt->devtype;  
    char *p,*cmd = NULL, path[0xFF] = {0};  
    int try_timers = 5;
    int ret,status;
	
    if(!strcmp(evt->action, "add") && !strcmp(devtype, "usb_device")) {   
  
	    ALOGI("event {'%s', '%s', '%s', '%s', %d, %d, '%s'}\n", evt->action, evt->path, evt->subsystem,
                    evt->firmware, evt->major, evt->minor, evt->devname);  
                    
        p = strstr(evt->path,"usb");
        if(p == NULL) {
        	return;
        }
        p += sizeof("usb");

        p = strchr(p,'-');
        if(p == NULL) {
        	return;	
        }

        strcat(path,"/sys");
        strcat(path,evt->path);
        ALOGI("path : '%s'\n",path); 
		
        ret = get_vid_pid(path);
		
        if((ret < 0)||(usb_pid == NULL)||(usb_vid == NULL)) {
        	return;	
        }
		 		
        ALOGD("VID = 0x%x, PID= 0x%x", strtoul(usb_vid, 0, 16), strtoul(usb_pid, 0, 16));
		if(!strncmp(usb_vid,"12d1",4)) {
            return;
		}
		
		ALOGD("%s,%d",__FUNCTION__,__LINE__);
		if(!strncmp(usb_vid,"19d2",4)) {
		    if((strtoul(usb_pid, 0, 16) & 0xff00) == 0xff00){
		        ;       // there PID dongles not processed under usb driver. Do below switch code  
		    }else if(strncmp(usb_pid, "0166", 4)){
		       return;
		    }
		}
		ALOGD("%s,%d",__FUNCTION__,__LINE__);
	    if(!strncmp(usb_vid, "05c6", 4) && (!strncmp(usb_pid, "6000", 4)))
		  return;
        
        //chmod /dev/bus/usb/00*/00*
        ALOGD("%s,%d,chmod 777 /dev/%s",__FUNCTION__,__LINE__,evt->devname);
        asprintf(&cmd, "chmod 777 /dev/%s",evt->devname); 
        system(cmd);
        
	    if(!strncmp(usb_vid, "19d2", 4) && (!strncmp(usb_pid, "fff5", 4))){
		  asprintf(&cmd, "usb_modeswitch -W -c /etc/usb_modeswitch.conf"); 
		  goto EXCUTE;   
        }		       	    	  
	    if(!strncmp(usb_vid, "05c6", 4) && !strncmp(usb_pid, "1000", 4)) {
	      asprintf(&cmd, "/system/xbin/usb_modeswitch -v %04x -p %04x -W -c /system/etc/usb_modeswitch.d/%s.%s.uMa\=StrongRising &", strtoul(usb_vid, 0, 16), strtoul(usb_pid, 0, 16), usb_vid,usb_pid);
		  sleep(2);
		} else		
		 asprintf(&cmd, "/system/xbin/usb_modeswitch -v %04x -p %04x -W -c /system/etc/usb_modeswitch.d/%s.%s &", strtoul(usb_vid, 0, 16), strtoul(usb_pid, 0, 16), usb_vid,usb_pid);

EXCUTE:
	    ALOGD("cmd=%s, try_timers=%d", cmd,try_timers);
        ret = system(cmd);
        ALOGD("ret = %d", ret);
        if((ret != 0) && (--try_timers > 0)) {
            sleep(2);
            goto EXCUTE;
        }
        free(cmd);
        
        if(try_timers == 0){
            ALOGD("try_timers = %d, can't translate to ttyUSB* devices", try_timers);
        }
    } 
    return;     
}  

static char rild_no_started_flag = 1;

void print_dev_ll(){
    char *p,*cmd = NULL;
    asprintf(&cmd, "ls /dev >> /data/dev_ll &"); 
    system(cmd);           
}
	
int start_rild(const char * usb_vid){
    
    char *usb_vendor;
//    print_dev_ll();
    
    if(!strncmp(usb_vid,"12d1", 4)){
        usb_vendor = "HUAWEI";
        ALOGD("start_rild,HUAWEI Dongle");
    }
    else if(!strncmp(usb_vid,"19d2", 4)) {
        usb_vendor = "ZTE";
        ALOGD("start_rild,ZTE Dongle");
    } else {
	  usb_vendor = usb_vid;
	}
    
    if(property_set(LIB_PATH_PROPERTY, "libsev-ril.so") < 0) {
  	   ALOGD("Faild to set %s libpath",usb_vendor);
    }
 
    if(property_set("dongle.vendorinfo", usb_vendor) < 0 ) {
        ALOGD("Faild to set vendor information");
    } else {
	    ALOGD("the dongle info %s", usb_vendor);
	}
	
	if(rild_no_started_flag) {
         rild_no_started_flag = 0;
         if (property_set("rild.start", "start") < 0)
            ALOGD("Failed to start rild ");
    }
    
    return 0;
}

#if 1
static void handleTtyEvent(struct uevent *evt)
{

    const char *devname = evt->devname;
	
	
	/******************************************/

	static char iCount = 0;
    /******************************************/

    if(!strcmp(evt->action, "add") && (!strncmp(evt->devname, "tty", 3))) {   
            /*call usb mode switch function*/  
	        ALOGI("tty event {'%s', '%s', '%s', '%s', %d, %d, '%s'}\n", evt->action, evt->path, evt->subsystem,
                    evt->firmware, evt->major, evt->minor, evt->devname);  
					
	    iCount++;
			 
		if(iCount == 1) {
	      	if(property_set(LIB_PATH_START_PORT_PROPERTY, evt->devname) < 0 ) {
//            char path_tmp[128] = {0};
//            strcpy(path_tmp,"/dev/");
//	      	if(property_set(LIB_PATH_START_PORT_PROPERTY, strstr(evt->devname,"/dev/") ? evt->devname : strcat(path_tmp,evt->devname)) < 0 ) {
                  ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
            }
        }
                    
		if(strncmp(usb_vid,"12d1", 4) && strncmp(usb_vid,"19d2", 4)) {

			 ALOGD("******Non Huawei and ZTE Dongle*********");
		}
				
		if(!strncmp(usb_vid,"12d1", 4)) {       //Huawei Dongle	 
			if((!strncmp(usb_pid,"1003", 4))) {  //special for E160E
				if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0 ) {
			 	   ALOGE("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          	}
			 
			 	if(property_set("ril.atport", "/dev/ttyUSB1") < 0 ) {
	                ALOGE("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          	}
				
				if(iCount >= 2) {  //special for E160E
                	start_rild("12d1");
                	iCount = 0;
            	} 
				
			}
			
            if(iCount >= 3) {  
                start_rild("12d1");
                iCount = 0;
            } 
		} else if(!strncmp(usb_vid,"19d2", 4)) {
		     if(iCount >= 3) {
                start_rild("19d2");
                iCount = 0;
			 }				  

		} else if(!strncmp(usb_vid,"05c6", 4) && !strncmp(usb_pid, "6000", 4)) {  //CDMA/EVDO
			 if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0 ) {
			 	   ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
			 if(property_set("ril.atport", "/dev/ttyUSB1") < 0 ) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
			  ALOGD("start Strong Rising....");
		       start_rild("StrongRising");
	    } else if(!strncmp(usb_vid,"20a6", 4) && !strncmp(usb_pid, "1105", 4)) {  //StrongRising WCDMA
			 if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0 ) {
			 	   ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
			 if(property_set("ril.atport", "/dev/ttyUSB2") < 0 ) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
			  start_rild("20a6:1105");

		} else {						 
	          if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0) {
	               ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
	
	          if(property_set(LIB_PATH_AT_PORT_PROPERTY, "/dev/ttyUSB1") < 0) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
		}
    } 
	
    if(!strcmp(evt->action, "remove") && (strlen(devname) > 1)) {
        ALOGI("tty event {'%s', '%s', '%s', '%s', %d, %d, '%s'}\n", evt->action, evt->path, evt->subsystem,
                    evt->firmware, evt->major, evt->minor, evt->devname);  

        if (rild_no_started_flag == 0){
            ALOGD("*****************stop rild*********************");           
            if (property_set("rild.stop", "stop") < 0){
                ALOGD("Failed to stop rild ");
            }
            rild_no_started_flag = 1;	
            iCount = 0;	        
        }
	 }
	 
     return;     
}    
#else
static void handleTtyEvent(struct uevent *evt)
{

    const char *devname = evt->devname;
	
	
	/******************************************/
	static char rild_no_started_flag = 1;
	static char iCount = 0;
    char rild_status[128]={0};
    /******************************************/

    if(!strcmp(evt->action, "add") && (!strncmp(evt->devname, "tty", 3))) {   
            /*call usb mode switch function*/  
	        ALOGI("tty event {'%s', '%s', '%s', '%s', %d, %d, '%s'}\n", evt->action, evt->path, evt->subsystem,
                    evt->firmware, evt->major, evt->minor, evt->devname);  
					
	    iCount++;
			 
		if(iCount == 1) {
			      	if(property_set(LIB_PATH_START_PORT_PROPERTY, evt->devname) < 0 ) {
	                      ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	                }
        }
                    
		if(strncmp(usb_vid,"12d1", 4) && strncmp(usb_vid,"19d2", 4)) {

			 ALOGD("******Non Huawei and ZTE Dongle*********");
		}
				
		if(!strncmp(usb_vid,"12d1", 4)) {
		
			 
		     //if(property_set(LIB_PATH_PROPERTY, "libhuawei-ril.so") < 0) {
		     if((iCount >= 3) || ((!strncmp(usb_vid,"12d1", 4)) && (!strncmp(usb_pid,"1003", 4)))) {  //special for E160E
			    if(property_set(LIB_PATH_PROPERTY, "libsev-ril.so") < 0) {
		      	   ALOGD("Faild to set HUAWEI libpath");
	            }
			 
			    if(property_set("dongle.vendorinfo", "HUAWEI") < 0 ) {
	                ALOGD("Faild to set vendor information");
	            }
				
				iCount = 0;

			 }	
			
				
				if(rild_no_started_flag) {
		             rild_no_started_flag = 0;
		             if (property_set("rild.start", "start") < 0)
		                ALOGD("Failed to start rild ");
		        }
			 
		} else if(!strncmp(usb_vid,"19d2", 4)) {
		     if(iCount >= 3) {
			    if(property_set(LIB_PATH_PROPERTY, "libsev-ril.so") < 0) {
		      	   ALOGD("Faild to set HUAWEI libpath");
	            }
			 
			    if(property_set("dongle.vendorinfo", "ZTE") < 0 ) {
	                ALOGD("Faild to set vendor information");
	            }
				
				iCount = 0;

			 }				  
			  if(rild_no_started_flag) {
		            rild_no_started_flag = 0;
		            if (property_set("rild.start", "start") < 0)
		               ALOGD("Failed to start rild ");
		       }
		} else if(!strncmp(usb_vid,"05c6", 4) && !strncmp(usb_pid, "6000", 4)) {  //CDMA/EVDO

			 if(property_set(LIB_PATH_PROPERTY, "libsev-ril.so") < 0) {
		      	   ALOGD("Faild to set strongrising libpath");
	         }
			 
			 if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0 ) {
			 	   ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
			 
			 if(property_set("ril.atport", "/dev/ttyUSB1") < 0 ) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
			  
			  
			  if(property_set("dongle.vendorinfo", "StrongRising") < 0 ) {
	                ALOGD("Faild to set vendor information");
	          }
			  
			  	if(rild_no_started_flag) {
		            rild_no_started_flag = 0;
		            if (property_set("rild.start", "start") < 0)
		            ALOGD("Failed to start rild ");
		       }
	    } else if(!strncmp(usb_vid,"20a6", 4) && !strncmp(usb_pid, "1105", 4)) {  //StrongRising WCDMA

			 if(property_set(LIB_PATH_PROPERTY, "libsev-ril.so") < 0) {
		      	   ALOGD("Faild to set strongrising libpath");
	         }
			 
			 if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0 ) {
			 	   ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
			 
			 if(property_set("ril.atport", "/dev/ttyUSB2") < 0 ) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
			  
			  //asprintf(&vendorinfo, "%s:%s", usb_vid, usb_pid);
			  
			  if(property_set("dongle.vendorinfo", "20a6:1105") < 0 ) {
	                ALOGD("Faild to set vendor information");
	          }
			  
			  if(rild_no_started_flag) {
		            rild_no_started_flag = 0;
		            if (property_set("rild.start", "start") < 0)
		            ALOGD("Failed to start rild ");
		       }
		} else {						 
	          if(property_set(LIB_PATH_MODEM_PORT_PROPERTY, "/dev/ttyUSB0") < 0) {
	               ALOGD("Faild to set LIB_PATH_MODEM_PORT_PROPERTY");
	          }
	
	          if(property_set(LIB_PATH_AT_PORT_PROPERTY, "/dev/ttyUSB1") < 0) {
	                ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
	          }
		}
    } 
	
    if(!strcmp(evt->action, "remove") && (strlen(devname) > 1)) {
        ALOGI("tty event {'%s', '%s', '%s', '%s', %d, %d, '%s'}\n", evt->action, evt->path, evt->subsystem,
                    evt->firmware, evt->major, evt->minor, evt->devname);  

        property_get("rild.stop",rild_status,"stop");
        ALOGD("rild_status:%s",rild_status); 
       
        if (!strcmp(rild_status, "start")){
            ALOGD("*****************stop rild*********************");           
            if (property_set("rild.stop", "stop") < 0){
                ALOGD("Failed to stop rild ");
            }		  
            rild_no_started_flag = 1;	
            iCount = 0;	        
        }
	 }
	 
     return;     
}   
#endif  


static void handle_uevent(struct uevent *event)
{
    
	const char *subsys = event->subsystem;
                   
	if (!strcmp(subsys, "usb")) {
    	    handleUsbEvent(event);	
    }
	
	if(!strcmp(subsys, "tty")) {
	      handleTtyEvent(event);	  	   
	}

    if(!strcmp(subsys, "usb-serial")) {
           ALOGI("usb-serial { '%s', '%s', '%s', '%s',  %d, %d, '%s'}\n", event->action, event->path, event->subsystem,
                    event->firmware, event->major, event->minor, event->devname);  	               
    }
}

static int uevent_init()
{
    struct sockaddr_nl addr;
    int sz = 64*1024;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if(s < 0)
        return s;

    setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

    if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(s);
        return 0;
    }

	return s;
}

static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->firmware = "";
    uevent->major = -1;
    uevent->minor = -1;
	uevent->devname = "";

//    ALOGI("parse_event,msg:%s\n",msg);
    
    while(*msg) {
    	
        if(!strncmp(msg, "ACTION=", 7)) {		    
            msg += 7;
            uevent->action = msg;
        } else if(!strncmp(msg, "DEVPATH=", 8)) {		   
            msg += 8;
            uevent->path = msg;
        } else if(!strncmp(msg, "SUBSYSTEM=", 10)) {		   
            msg += 10;
            uevent->subsystem = msg;
        } else if(!strncmp(msg, "DEVTYPE=", 8)) {		 
            msg += 8;
            uevent->devtype = msg;
        }else if(!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            uevent->firmware = msg;
        } else if(!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            uevent->major = atoi(msg);
        } else if(!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            uevent->minor = atoi(msg);
        } else if(!strncmp(msg, "DEVNAME=", 8)) {
            msg += 8;
            uevent->devname = msg;
        }

        /* advance to after the next \0 */
        while(*msg++)
            ;
    }   
}


#define UEVENT_MSG_LEN  1024

static int uevent_monitor(int fd)
{
    
    struct uevent event;    
    char buffer[UEVENT_MSG_LEN+2];
    int count,nr;
	
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
           
    while (1) {        
  
		nr = select(fd + 1, &rfds, NULL, NULL, NULL);
     	if (FD_ISSET(fd, &rfds) && (nr > 0)) {     	

            memset(buffer, 0, UEVENT_MSG_LEN+2);
            count = recv(fd, buffer, UEVENT_MSG_LEN+2, 0);

            if (count > 0) {
               parse_event(buffer, &event); 
               handle_uevent(&event);
            } 
        }
    }
        
    return 0;
}


static unsigned int getUsbVidPid(const char *path, char *buffer, int len, char *usb_vid)
{
    //link path
    //../../devices/platform/sw-ehci.2/usb3/3-1/3-1.6/    3-1.6:1.4/ttyUSB4/tty/ttyUSB4
    //../../devices/platform/sw-ehci.2/usb3/3-1/    3-1:1.0/ttyUSB0/tty/ttyUSB0
    // /sys
    FILE *fp  = NULL;
    char *pos = NULL;
    int link_len = 0; 
    int i = 0;
    char tmp[256]= {'/','s','y','s','/','\0'};
    char tmp1[5] = {'\0'};
    unsigned int value  = 0;

    if(len < 4)
        return 0;
    
    memset(buffer, '\0', len*sizeof(char));
    
    link_len =  readlink(path, buffer, len - 1);
    if(link_len < 0 || link_len >= len - 1) {
        ALOGE("readfail code = %d", link_len);
        return 0;
    }

    for( i = 0; i < len - 4; ) {
        if( '.' == buffer[i] && '.' == buffer[i+1] && '/'== buffer[i+2]){
            i += 3;
        } else {
            break;
        }
    }  
    
    memcpy(tmp + 5, buffer + i, link_len);
    for( i = 4; i > 0 ; i-- ){    
       pos = (char *)strrchr(tmp, '/'); 
       *pos = '\0';
    }    
    
    strcat(tmp, "/idVendor");    
    fp = fopen(tmp, "r");
    if(!fp) {
        ALOGD("open file error 1");
        return 0;
    }    
    fread((void *)tmp1, 1, 4, fp);
    value  = strtol(tmp1,'\0', 16);
    value <<=16;    
    fclose(fp);
    memcpy(usb_vid,tmp1,4);

    *pos = '\0';

    strcat(tmp, "/idProduct");    
    fp = fopen(tmp, "r");
    if(!fp) {
        ALOGD("open file error 2");
        return 0;
    }    
    fread((void *)tmp1, 1, 4, fp);      
    value  |= strtol(tmp1,'\0', 16);  
    fclose(fp);   

    return value;    
}

void preprocess_uvent()
{
    #define USB_DONGLE_PATH_DEPTH 256
    
    char uid_pid_path[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    char usb_vid[USB_DONGLE_PATH_DEPTH] ={'\0'}; 
    unsigned int vid_pid_cur = 0;
    
    if(access("/sys/class/tty/ttyUSB0", 0) == 0){
        vid_pid_cur = getUsbVidPid("/sys/class/tty/ttyUSB0", uid_pid_path, USB_DONGLE_PATH_DEPTH,usb_vid); 
        ALOGI("Get device vid-pid:0x%08x,usb_vid:%s", vid_pid_cur,usb_vid);
        
        if(property_set(LIB_PATH_START_PORT_PROPERTY, "ttyUSB0") < 0 ) {
            ALOGD("Faild to set LIB_PATH_AT_PORT_PROPERTY");
        }
            
        start_rild(usb_vid);
    }
    else{
        ALOGI("preprocess_uvent, no Modem Dongle plugin befor");
    }
}

int main()
{
    int fd;

	ALOGI("usb 3g dongle monitor start\n");
    
    preprocess_uvent();       //process possilbe missed uvent before appliaction launch
        
	fd = uevent_init();
	handleUsbFirstEvent();
	uevent_monitor(fd);
	return 0;
}
