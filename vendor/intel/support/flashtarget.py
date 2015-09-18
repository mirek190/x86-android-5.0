#!/usr/bin/env python
import sys
import json

# return the list of filename from json configuration file
with open(sys.argv[1], 'r') as f:
    data = json.loads(f.read())

tg = [l['filename'] for l in data['config']]
print ' '.join(tg)
