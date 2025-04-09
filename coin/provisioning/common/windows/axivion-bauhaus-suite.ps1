############################################################################
##
## Copyright (C) 2022 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
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

# Requires: 7z

. "$PSScriptRoot\helpers.ps1"

# Installs Axivion Bauhaus Suite

$version = "7_4_5"
$zip = "bauhaus-suite-${version}-x86_64-windows.zip"
$sha1 = "cd4f13a707ea968241101c6d24e7be3d2e58f2e0"
$destination = "C:\Users\qt\"
$zip_url = "\\ci-files01-hki.intra.qt.io\provisioning\axivion\bauhaus-suite-${version}-x86_64-windows.zip"

Download $zip_url $zip_url $zip
Verify-Checksum $zip $sha1
Extract-7Zip $zip $destination
Remove $zip
New-Item -ItemType directory -Path "C:\Users\qt\.bauhaus"
$license_key = "Qt_Evaluation_20221231.key"
$license_key_sha1 = "00add6569485c24bfc198fd49e234d499bb28c13"
$license_key_url = "\\ci-files01-hki.intra.qt.io\provisioning\axivion\Qt_Evaluation_20221231.key"
Download $license_key_url $license_key_url $license_key 
Verify-Checksum $license_key $license_key_sha1
Move-Item -Path "C:\Users\qt\Qt_Evaluation_20221231.key" -Destination "C:\Users\qt\.bauhaus"

Write-Output "Axivion Bauhaus Suite = $version" >> ~/versions.txt
