#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is part of the provisioning scripts of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL21$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://www.qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## $QT_END_LICENSE$
##
#############################################################################

# This script builds Node js from sources

# shellcheck source=../unix/DownloadURL.sh
source "${BASH_SOURCE%/*}/../unix/DownloadURL.sh"

set -ex

version="14.15.3"
officialUrl="https://nodejs.org/dist/v${version}/node-v${version}.tar.gz"
cachedUrl="http://ci-files01-hki.intra.qt.io/input/nodejs/node-v${version}.tar.gz"
prebuiltUrl="http://ci-files01-hki.intra.qt.io/input/nodejs/node-v${version}_prebuilt.tar.gz"
sourceSha1="5d9fa942d4e32467a27ad0b240f4e34366ae07ec"
prebuiltSha1="0e0d4aa98bac7ba43eed4f71831433c0072b6c89"
targetFile="/tmp/node-v${version}.tar.gz"
prebuiltTargetFile="/tmp/node-v${version}_prebuilt.tar.gz"

echo "Fetching prebuilt Nodejs"
curl --fail -L --retry 5 --retry-delay 5 -o "$prebuiltTargetFile" "$prebuiltUrl" || (
    echo "Fetching prebuilt Nodejs failed. Building from sources."
    DownloadURL "$cachedUrl" "$officialUrl" "$sourceSha1" "$targetFile"
)
if [ -f "$prebuiltTargetFile" ]; then
    echo "$prebuiltSha1 $prebuiltTargetFile" | sha1sum -c -
    sudo tar -xzf "$prebuiltTargetFile" -C "/home/qt/"
    cd "/home/qt/node-v$version"
else
    sudo tar -xzf "$targetFile" -C "/home/qt/"
    sudo chown -R qt:qt "/home/qt/node-v$version"
    cd "/home/qt/node-v$version"
    ./configure
    make -j5
fi
sudo make install

rm -rf $prebuiltTargetFile || rm -fr $targetFile

echo "Nodejs = $version" >> ~/versions.txt
