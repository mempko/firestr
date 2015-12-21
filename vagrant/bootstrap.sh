#!/bin/bash
apt-get update && apt-get -y upgrade && apt-get -y autoremove
apt-get install -y xfce4
apt-get install -y virtualbox-guest-dkms virtualbox-guest-utils virtualbox-guest-x11
apt-get install -y build-essential cmake libbotan1.10-dev libopus-dev libsnappy-dev libqt5core5a libqt5widgets5 libqt5network5 libqt5multimedia5 libqt5multimedia5-plugins libqt5multimediawidgets5 qtmultimedia5-dev libqt5svg5-dev libboost-system-dev libboost-program-options-dev libboost-filesystem-dev libboost-regex-dev libboost-thread-dev uuid-dev libgmp-dev libssl-dev libxss-dev

sed -i 's/console/anybody/' /etc/X11/Xwrapper.config 
perl /usr/share/debconf/fix_db.pl
