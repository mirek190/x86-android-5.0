# Set properties for the selected configuration

# read all FeatureTeam's init.props file
for f in /local_cfg/*/init.props
do
    while read l; do

        # Ignore empty lines and comments
        case "$l" in
            ''|'#'*)
                continue
                ;;
        esac

        # Set property
        setprop `echo ${l/=/ }`

    done < $f
done
