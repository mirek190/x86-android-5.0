


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>
#include "ril.h"

static int readfile(char *pathname, char* buffer,  int size);
static int getfirstinterfacename(char*dir,char*ifname);
static int readinterfaceinfo(char *pathname,char *class, char *subclass, char *protocol);
static int reversefind(const char *str, char ch);
static int get_net_mode();
static int get_channels(char *atchannel, char *datachannel);
static int get_devinfo(char *dir,char *vid);

char interface_name[256];


struct interfaceinfo
{
	char ifclass[4];
	char ifsubclass[4];
	char ifprotocol[4];
};

struct deviceinfo
{
	char vid[6];
	char pid[6];
	char atchannel[15];
	char datachannel[15];
	int mode;
	int phone_is;
};

struct deviceinfo deviceinfotable[]=
{
	{"19d2", "0016", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1435", "/dev/ttyUSB1", "/dev/ttyUSB2", 1,MODE_GSM}, 
	{"19d2", "0017", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1589", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1591", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1592", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0189", "/dev/ttyUSB2", "/dev/ttyUSB2", 1,MODE_GSM},
	{"19d2", "1253", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "1533", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1535", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1433", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1254", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "1594", "/dev/ttyUSB1", "/dev/ttyUSB0", 0,MODE_GSM},
	{"19d2", "1596", "/dev/ttyUSB3", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "1244", "/dev/ttyUSB1", "/dev/ttyUSB2", 1,MODE_GSM},
	{"19d2", "0117", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},  //MF190  MF667
	{"19d2", "0001", "/dev/ttyUSB2", "/dev/ttyUSB0", 0,MODE_GSM},
	{"19d2", "0002", "/dev/ttyUSB4", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0012", "/dev/ttyUSB4", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0015", "/dev/ttyUSB0", "/dev/ttyUSB2", 0,MODE_GSM},  //MF628 iNet
	{"19d2", "0021", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0030", "/dev/ttyUSB3", "/dev/ttyUSB1", 0,MODE_GSM},
	{"19d2", "0031", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},    //MF633,have ttyusb0/1/2, dataport not use ttyUSB3
	{"19d2", "0033", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0042", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0043", "/dev/ttyUSB2", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0048", "/dev/ttyUSB2", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0049", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0061", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0064", "/dev/ttyUSB0", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0066", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0086", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "2003", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0121", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0123", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0124", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0125", "/dev/ttyUSB1", "/dev/ttyUSB5", 0,MODE_GSM},
	{"19d2", "0126", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0128", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0144", "/dev/ttyUSB2", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0145", "/dev/ttyUSB2", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0151", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0153", "/dev/ttyUSB2", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0155", "/dev/ttyUSB1", "/dev/ttyUSB4", 0,MODE_GSM},
	{"19d2", "0156", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "0157", "/dev/ttyUSB1", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "0162", "/dev/ttyUSB2", "/dev/ttyUSB3", 0,MODE_GSM},
	{"19d2", "1510", "/dev/ttyUSB1", "/dev/ttyUSB2", 0,MODE_GSM},
	{"19d2", "fff1", "/dev/ttyUSB1", "/dev/ttyUSB0", 0,MODE_CDMA},  //AC2787,AC583
	
	{NULL, NULL, NULL, NULL, 0}
};

struct deviceinfo mydevinfo = {0};

static int get_devinfo(char *dir,char *vid)
{
	int ifnum = 0;
	int i,ret,len;
	char tmp[20];
	char pid[6];
	char ifname[50];

	struct dirent *filename;
	struct dirent *subfilename;
	struct dirent *fiafilename;
	struct interfaceinfo *ifinfo;
    char mydir[256]={0};  
	int devmode = -1;
    int found = 0; 	
    static char* dot=".";
    static char* dotdot="..";

	int device_index=0;

	int  ecm_pid = 0;
    int ecm_status=0;
	char net[6]={0};
    DIR*dp=NULL;
    DIR*subdp=NULL;
    DIR*fia_name_dp=NULL;
	char pathname[256];
	char idVendor[6]={0};
	
	sprintf(mydevinfo.vid, "%s", vid);
	mydevinfo.mode = -1;
	
    dp=opendir(dir);
	if(dp == NULL)
	{
		ALOGD("Opendir %s failed!", dir);
		return 0;
	}
	//ALOGD("devmode = %d\n", mydevinfo.mode);

    while((filename=readdir(dp))!=NULL)
	{
       /// ALOGD("filename:%s\n",filename->d_name);
        //ALOGD("file type:%d\n",filename->d_type);
        if((filename->d_type == DT_DIR || filename->d_type == DT_LNK) && strcmp(filename->d_name,dot) && strcmp(filename->d_name,dotdot))
        {
            memset(mydir,0,sizeof(mydir));
            strcpy(mydir,dir);
            strcat(mydir,"/");
            strcat(mydir,filename->d_name);                
           // ALOGD("mydir:%-10s\n",mydir);
               
            subdp = opendir(mydir);
			if(subdp == NULL)
			{
				ALOGD("Opendir %s failed!\n", mydir);
				closedir(dp);
				return 0;
			}
			
			while((subfilename = readdir(subdp)) != NULL)
			{
				//ALOGD("subfilename:%s\n",subfilename->d_name);
				//ALOGD("file type:%d\n",subfilename->d_type);

               //add by gaojing to get interface name
               

              if(!strcmp(subfilename->d_name,"net"))

				{

				         char name_path_1[256];

						//  LOGD("zte test net path_name is %s",mydir);

						 memset(name_path_1,0,sizeof(name_path_1));
                                          strcpy(name_path_1,mydir);
                                          strcat(name_path_1,"/");
						strcat(name_path_1,"net");  

					//	LOGD("zte test name_path_1 is %s",name_path_1);

                        fia_name_dp = opendir(name_path_1);
						

						if(fia_name_dp == NULL)
			               {
				               ALOGD("Opendir %s failed!\n", name_path_1);
				               closedir(fia_name_dp);
				               return 0;
			               }

					     while((fiafilename = readdir(fia_name_dp)) != NULL)
			              {

                                         if((fiafilename->d_type == DT_DIR || fiafilename->d_type == DT_LNK) && strcmp(fiafilename->d_name,dot) && strcmp(fiafilename->d_name,dotdot))
                                    {

					  	memcpy(interface_name, fiafilename->d_name, strlen(fiafilename->d_name)+1);

						device_index = 1;

						ALOGD("zte test interface_name is %s\n",interface_name);
                                      }
                                           
				       }
					}

				


				if((subfilename->d_type == DT_DIR || subfilename->d_type == DT_LNK) && strcmp(subfilename->d_name,dot) && strcmp(subfilename->d_name,dotdot))
				{
					continue;
				}
				else if(!strcmp(subfilename->d_name,"idVendor"))
				{
					memset(pathname,0,sizeof(pathname));
					strcpy(pathname,mydir);
					strcat(pathname,"/");
					strcat(pathname,subfilename->d_name); 
			
					readfile(pathname,idVendor, 5);
					idVendor[4] = 0;

					if(strncmp(idVendor,vid,4))
					{
						break;
					}
			
					memset(pathname,0,sizeof(pathname));
					strcpy(pathname,mydir);
					strcat(pathname,"/idProduct");
					readfile(pathname,pid, 5);
					pid[4] = 0;
					//ALOGD("pid:%s\n",pid);
					memcpy(mydevinfo.pid, pid, 5);
			
					memset(pathname,0,sizeof(pathname));
					strcpy(pathname,mydir);
					strcat(pathname,"/bNumInterfaces");
					readfile(pathname,tmp, 2);
					ifnum = atoi(tmp);
					//ALOGD("bNumInterfaces:%s\n",tmp);
			
					ret = getfirstinterfacename(mydir, ifname);
					if (ret == 0)
					{
						ALOGD("get interface name failed\n");
						closedir(subdp);
						closedir(dp);
						return 0;
					}
			
					ifinfo = (struct interfaceinfo *)malloc(ifnum*sizeof(struct interfaceinfo));
					len = strlen(ifname);
					for (i=0; i<ifnum; i++)
					{
						ifname[len-1] = '0'+i;
						memset(pathname, 0, sizeof(pathname));
						strcpy(pathname, mydir);
						strcat(pathname, "/");
						strcat(pathname, ifname);
						//ALOGD("pathname = %s\n", pathname);
						readinterfaceinfo(pathname, ifinfo[i].ifclass, ifinfo[i].ifsubclass, ifinfo[i].ifprotocol);
					}
					
					devmode = 0;
					for (i=0; i<ifnum; i++)
					{
						if((!(strcmp(ifinfo[i].ifclass, "02"))) && (!(strcmp(ifinfo[i].ifsubclass,"06"))) && (!(strcmp(ifinfo[i].ifprotocol, "00"))))
						{
							if((!(strcmp(ifinfo[i+1].ifclass, "0a"))) && (!(strcmp(ifinfo[i+1].ifsubclass,"00"))) && (!(strcmp(ifinfo[i+1].ifprotocol, "00"))))
							{
								devmode = 1;
								break;
							}
						}

					}
					free(ifinfo);
					break;
				}
			}
			if (device_index == 1)
				break;
		}
    }
    mydevinfo.mode = devmode;
 
  ecm_pid = atoi(mydevinfo.pid);

  ALOGD("zte test ecm_pid is %d\n",ecm_pid);

   if( ecm_pid >= 1435 && ecm_pid <= 1446)
    {

        ALOGD("zte test the ecm_Pid is in table\n");
		ecm_status=1;	  

    }

     ALOGD("zte test ecm_status is %d\n",ecm_status);
	
    if((!strcmp(mydevinfo.pid, "1405") )||(ecm_status == 1))
	  {
        ALOGD("zte test to send dhcp commad for old freedata\n");

		char  dhcdp_cmd[256];

		char up_interface[256];

		sprintf(up_interface, "netcfg %s up",interface_name);
		   
		sprintf(dhcdp_cmd, "dhcpcd -ABKL %s &",interface_name);

		system(up_interface);

		sleep(1);

		system(dhcdp_cmd);


    }

	
    int n = sizeof(deviceinfotable)/sizeof(struct deviceinfo);
	for (i=0;i<n;i++)
	{
		if(deviceinfotable[i].vid == NULL)
		{
			ALOGD("This vid is not in the table!\n");
			break;
		}
		if(!(strcmp(deviceinfotable[i].pid, pid)))
		{
			found = 1;
			//ALOGD("The at channel is %s\n", deviceinfotable[i].atchannel);
			memcpy(mydevinfo.atchannel, deviceinfotable[i].atchannel, 15);
			if (mydevinfo.mode == 0)
			{
				ALOGD("The data channel is %s\n", deviceinfotable[i].datachannel);
				memcpy(mydevinfo.datachannel, deviceinfotable[i].datachannel, 15);
			}
			mydevinfo.phone_is = deviceinfotable[i].phone_is;
			break;
		}
	}
	
	closedir(dp);
	closedir(subdp);
	
	return 1;

}

static int readfile(char *pathname, char* buffer, int size)
{
    int fd;
    int count=0;
 
    if(buffer==NULL)
    {
       return 0;
    }
    if(pathname==NULL) 
		return 0;

	
    if((fd=open(pathname,O_RDONLY))==-1)
    {
        return 0;
    }
    count=read(fd,buffer,size);
    close(fd);
    return 1;
}

static int getfirstinterfacename(char*dir, char*ifname)
{
	DIR* dp=NULL;
	struct dirent *filename;
	char subdir[10];
	int index;
	
	
	dp = opendir(dir);
	if(dp == NULL)
	{
		ALOGD("Opendir %s failed!", dir);
		return 0;
	}
	
	index = reversefind(dir, '/');
	memcpy(subdir, dir+index+1, strlen(dir)-index);
	//ALOGD("subdir = %s\n", subdir);
	
	while((filename=readdir(dp))!=NULL)
	{
		//ALOGD("filename:%s\n",filename->d_name);
		if(strstr(filename->d_name, subdir) != NULL)
		{
			memcpy(ifname, filename->d_name, strlen(filename->d_name)+1);
			return 1;
		}
	}
	
	return 0;
}

static int readinterfaceinfo(char *pathname,char *class, char *subclass, char *protocol)
{
	char filename[256];
	char tmp[20];
	
	memset(filename, 0, 256);
	strcat(filename, pathname);
	strcat(filename, "/bInterfaceClass");
	readfile(filename, tmp, 2);
	tmp[2] = 0;
	memcpy(class, tmp, 3);
	
	memset(filename, 0, 256);
	strcat(filename, pathname);
	strcat(filename, "/bInterfaceSubClass");
	readfile(filename, tmp, 2);
	tmp[2] = 0;
	memcpy(subclass, tmp, 3);
	
	memset(filename, 0, 256);
	strcat(filename, pathname);
	strcat(filename, "/bInterfaceProtocol");
	readfile(filename, tmp, 2);
	tmp[2] = 0;
	memcpy(protocol, tmp, 3);

	return 0;
}

static int reversefind(const char *str, char ch)
{
	int i, len;
	
	len = strlen(str);
	for(i=len-1; i>=0; i--)
	{
		if(str[i] == ch)
		{
			return i;
		}
	}
	
	return -1;
}

int getdevinfors(char *vid, char *pid, char *atchannel, char *datachannel, int *phone_is)
{
	char dir[] = "/sys/bus/usb/devices";
	int ret;

	ret = get_devinfo(dir, vid);
	if (!ret)
	{
		ALOGD("get_devinfo failed!\n");
		return 0;
	}

	
	*phone_is = mydevinfo.phone_is;
    memcpy(pid, mydevinfo.pid, 6);
	memcpy(atchannel, mydevinfo.atchannel, 15);
	memcpy(datachannel, mydevinfo.datachannel, 15);
	
    ALOGD("mydevinfo.phone_is = %d\n", mydevinfo.phone_is);
	ALOGD("pid is %s\n", pid);
	ALOGD("At channel is %s, data channel is %s\n", atchannel, datachannel);
	return 1;
}



