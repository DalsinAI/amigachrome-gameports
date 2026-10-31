#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Stage an isolated, checksummed Open4 compiler using the existing SDK importer."""
from pathlib import Path
import argparse
import sys
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'scripts'))
import stage_open2_stove as importer
RELEASE = '16.2.0-open4'
SHA256 = '495319a2ffd803278f8827cad70074029c7aa9ecbe05d037e8646ba43ab94ab2'
if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['archive','ndk-prefix','sdk-prefix','destination']:
        ap.add_argument('--'+n,type=Path,required=True)
    a=ap.parse_args()
    importer.RELEASES[RELEASE]=(SHA256,'openamigagcc-16.2.0-open4-x86_64-linux')
    importer.stage(a.archive,a.ndk_prefix,a.sdk_prefix,a.destination,release=RELEASE)
    print(a.destination)
