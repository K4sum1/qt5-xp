#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2022 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the provisioning scripts of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 3 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL3 included in the
## packaging of this file. Please review the following information to
## ensure the GNU Lesser General Public License version 3 requirements
## will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 2.0 or (at your option) the GNU General
## Public license version 3 or any later version approved by the KDE Free
## Qt Foundation. The licenses are as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-2.0.html and
## https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# This script installs Polyspace Bug Finder tool.

set -ex

# shellcheck source=../unix/DownloadURL.sh
source "${BASH_SOURCE%/*}/../unix/DownloadURL.sh"
# shellcheck source=../unix/SetEnvVar.sh
# source "${BASH_SOURCE%/*}/../unix/SetEnvVar.sh"

DownloadAndExtract () {
    url=$1
    sha=$2
    file=$3
    folder=$4

    DownloadURL "$url" "$url" "$sha" "$file"
    tar -xzvf $file

    rm -rf $file
}


targetPath="$HOME/temp"

if [ ! -d "$targetPath" ]; then
    mkdir -p $targetPath
fi

# Polyspace Bug Finder
sourceFile="http://ci-files01-hki.intra.qt.io/input/polyspace/polyspace_bugfinder.tar.gz"
targetFile="polyspace_bugfinder.tar.gz"
sha1="2f3dd86a595344007bef8b2500c37b3478f692a7"
cd $targetPath
DownloadAndExtract "$sourceFile" "$sha1" "$targetFile" "$targetPath"

cd "$targetPath/MathWorks/R2021b/2022_01_27_11_34_31"
wget http://ci-files01-hki.intra.qt.io/input/polyspace/input.txt
wget http://ci-files01-hki.intra.qt.io/input/polyspace/license.dat
sudo ./install -inputFile input.txt
sudo mkdir -p /usr/local/Polyspace_Server/R2021b/licenses
sudo cp license.dat /usr/local/Polyspace_Server/R2021b/licenses/license.dat

echo "Polyspace Bug Finder = R2021b" >> ~/versions.txt
