#!/usr/bin/env python
import sys
import json

# return a list of TARGET:$(TARGET) for each target found in .json
with open(sys.argv[1], 'r') as f:
    data = json.loads(f.read())

deps = [l['target'] + ':$(' + l['target'] + ')' for l in data['commands'] if 'target' in l]
print ' '.join(set(deps))
