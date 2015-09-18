# Get selected software configuration

config=`cat /config/local_config`
ln -s /system/etc/catalog/$config /local_cfg

log -p i -t config_init "Activating configuration $config"

# Boot flow has to be suspended so that the ramdisk is not re-mounted r/o.
# Signal configuration is done so that boot flow can resume.
echo > /config_init.done
