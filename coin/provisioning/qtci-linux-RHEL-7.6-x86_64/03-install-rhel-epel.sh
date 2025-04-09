#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2018 The Qt Company Ltd.
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

set -ex

# shellcheck source=../common/unix/DownloadURL.sh
source "${BASH_SOURCE%/*}/../common/unix/DownloadURL.sh"

# EPEL 7 has moved under archive -folder and is no longer supported version of EPEL:
# https://docs.fedoraproject.org/en-US/epel/#what_packages_and_versions_are_available_in_epel
package="epel-release-7-14.noarch.rpm"
primaryUrl="https://dl.fedoraproject.org/pub/archive/epel/7/x86_64/Packages/e/$package"
cacheUrl="http://ci-files01-hki.intra.qt.io/input/rhel7/$package"
sha256="e2d5ffdd4cfe09dde17018a31d100db611abe88cc6761d9bdc0c1f41efaa5aa0"
targetFile="/tmp/$package"

DownloadURL "$cacheUrl" "$primaryUrl" "$sha256" "$targetFile"

sudo yum -y install "$targetFile"
rm "$targetFile"
