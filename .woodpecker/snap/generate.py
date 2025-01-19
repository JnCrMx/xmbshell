#!/usr/bin/env python3

import yaml
import os
import sys

def merge(a: dict, b: dict, path=[]):
    for key in b:
        if key in a:
            if isinstance(a[key], dict) and isinstance(b[key], dict):
                merge(a[key], b[key], path + [str(key)])
            elif isinstance(a[key], list)and isinstance(b[key], list):
                a[key] += b[key]
            elif b[key] is None:
                del a[key]
            elif a[key] != b[key]:
                raise Exception('Conflict at ' + '.'.join(path + [str(key)]))
        else:
            a[key] = b[key]
    return a

UBUNTU_KEY_ID = 'F6ECB3762474EDA9D21B7022871920D1991BC93C'
UBUNTU_URL = 'http://de.archive.ubuntu.com/ubuntu'

if len(sys.argv) < 6:
    print("Usage: generate.py <input> <output> <main part> <version> <url> [additional yaml files...]")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

MAIN_PART = sys.argv[3]
BUILD_VERSION = sys.argv[4]
BUILD_URL = sys.argv[5]

root_dir = os.path.dirname(input_file)
if(root_dir.endswith('snap')):
    root_dir = os.path.dirname(root_dir)

with open(input_file, 'r') as f:
    data = yaml.safe_load(f)
    for additional_file in sys.argv[6:]:
        with open(additional_file, 'r') as f:
            additional_data = yaml.safe_load(f)
            if('pre' in additional_data):
                data = merge(data, additional_data['pre'])
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
    data['platforms'] = {
        'amd64-on-arm64':{
            'build-on': ['arm64'],
            'build-for': ['amd64']
        },
        'amd64-on-amd64':{
            'build-on': ['amd64'],
            'build-for': ['amd64']
        }
    }
    data['lint'] = {'ignore': ['classic', 'library']}
    data['version'] = BUILD_VERSION
    data['icon'] = os.path.relpath(root_dir+'/'+data['icon'], os.path.dirname(output_file))
    part = data['parts'][MAIN_PART]
    data['parts'][MAIN_PART] = {
        'plugin': 'dump',
        'source': BUILD_URL,
        'stage-packages': [
            package + ':amd64' for package in part['stage-packages']
        ] + ['shared-mime-info:amd64']
    }
    for additional_file in sys.argv[6:]:
        with open(additional_file, 'r') as f:
            additional_data = yaml.safe_load(f)
            if('post' in additional_data):
                data = merge(data, additional_data['post'])
    yaml.dump(data, open(output_file, 'w'), default_flow_style=False)
