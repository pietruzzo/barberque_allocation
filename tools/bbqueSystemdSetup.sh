#!/bin/bash

echo "Systemd configuration files patch: cpuset, cpu, cpuacct, memory"
echo "controllers hierarchy unification"

if [[ "$(pidof systemd)" == "" ]]; then
        echo "Systemd does not appear to be up and running"
        exit
else
        SDV=$(systemd --version | head -n1 | awk '{print $2}')
        echo "Systemd version = $SDV"

        if [ "$SDV" != "225" ]; then
                echo "This patch is needed for systemd version 225 to properly"
                echo "co-exist with The BarbequeRTRM."
                echo "You should not need it for versions higher"
                echo "than that, and you may not need it for versions lower"
                echo "than that. Apply the patch only if you cannot start"
                echo -e "barbeque due to mount errors.\n"
                read -p "Continue anyway? [y/N] " ANS

                [ "$ANS" != "y" ] && [ "$ANS" != "Y" ] && exit
        fi
fi

cat /etc/systemd/system.conf | sed -r 's/.*JoinControllers.*/JoinControllers=cpu,cpuset,cpuacct,memory net_cls,net_prio/g' > systemdconf.bbque.tmp

diff /etc/systemd/system.conf systemdconf.bbque.tmp > systemdconf.bbque.diff

if [ "$(cat systemdconf.bbque.diff)" == "" ]; then
        echo "No changes to do"
        exit
fi

echo -e "\nProposed changes:\n"
cat systemdconf.bbque.diff
echo -e "\n"

rm systemdconf.bbque.diff

read -p "Apply? [y/N] " ANS

if [ "$ANS" == "y" ] || [ "$ANS" == "Y" ]; then
        echo "Changes will be applied."
        sudo mv systemdconf.bbque.tmp /etc/systemd/system.conf
else
        rm systemdconf.bbque.tmp
        exit
fi

read -p "System needs to be rebooted. Do it now? [y/N] " ANS

if [ "$ANS" != "y" ] && [ "$ANS" != "Y" ]; then
	exit
fi

sudo reboot
