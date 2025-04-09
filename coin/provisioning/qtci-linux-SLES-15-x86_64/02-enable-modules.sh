#!/usr/bin/env bash

set -e

curl --retry 5 --retry-delay 10 --retry-max-time 60 http://ci-files01-hki.intra.qt.io/input/semisecure/suse_rk.sh -o "/tmp/suse_rk.sh" &>/dev/null
sudo chmod 755 /tmp/suse_rk.sh
/tmp/suse_rk.sh

# Activate these modules
sudo SUSEConnect -p sle-module-desktop-applications/15/x86_64
sudo SUSEConnect -p sle-module-development-tools/15/x86_64
# This is needed by Nodejs and QtWebEngine
sudo SUSEConnect -p sle-module-web-scripting/15/x86_64

sudo rm -f /tmp/suse_rk.sh
