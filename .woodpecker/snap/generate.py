#!/usr/bin/env python3

import yaml
import os
import sys

UBUNTU_KEY_ID = 'F6ECB3762474EDA9D21B7022871920D1991BC93C'
UBUNTU_URL = 'http://de.archive.ubuntu.com/ubuntu'

if len(sys.argv) != 5:
    print("Usage: generate.py <input> <output> <version> <url>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

BUILD_VERSION = sys.argv[3]
BUILD_URL = sys.argv[4]

root_dir = os.path.dirname(input_file)
if(root_dir.endswith('snap')):
    root_dir = os.path.dirname(root_dir)

with open(input_file, 'r') as f:
    data = yaml.safe_load(f)
    data['package-repositories'] = [
        {
            'type': 'apt',
            'formats': ['deb'],
            'architectures': ['amd64'],
            'components': ['main', 'universe'],
            'suites': ['noble'],
            'key-id': UBUNTU_KEY_ID,
            'url': UBUNTU_URL,
        }
    ]
    data['architectures'] = [
        {
            'build-on': ['arm64'],
            'build-for': ['amd64']
        }
    ]
    data['lint'] = {'ignore': ['classic', 'library']}
    data['version'] = BUILD_VERSION
    data['icon'] = os.path.relpath(root_dir+'/'+data['icon'], os.path.dirname(output_file))
    part_key = next(iter(data['parts']))
    part = data['parts'][part_key]
    data['parts'][part_key] = {
        'plugin': 'dump',
        'source': BUILD_URL,
        'stage-packages': [
            package + ':amd64' for package in part['stage-packages']
        ] + ['shared-mime-info:amd64']
    }
    yaml.dump(data, open(output_file, 'w'), default_flow_style=False)
