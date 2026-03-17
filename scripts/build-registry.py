#!/usr/bin/env python3
"""Regenerates packages/registry.json from the packages/ directory."""

import json
import os
import hashlib
import datetime

def main():
    packages_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'packages')
    packages = []

    for pkg_dir in sorted(os.listdir(packages_dir)):
        pkg_path = os.path.join(packages_dir, pkg_dir)
        if not os.path.isdir(pkg_path):
            continue
        if pkg_dir.startswith('.'):
            continue

        pkg_json_path = os.path.join(pkg_path, 'package.json')
        ilma_path = os.path.join(pkg_path, f'{pkg_dir}.ilma')

        if not os.path.exists(pkg_json_path):
            continue

        with open(pkg_json_path) as f:
            pkg = json.load(f)

        if os.path.exists(ilma_path):
            with open(ilma_path, 'rb') as f:
                content = f.read()
            pkg['file'] = f'{pkg_dir}/{pkg_dir}.ilma'
            pkg['url'] = f'https://ilma-lang.dev/packages/{pkg_dir}/{pkg_dir}.ilma'
            pkg['size_bytes'] = len(content)
            pkg['sha256'] = hashlib.sha256(content).hexdigest()
        else:
            print(f'Warning: {pkg_dir}.ilma not found, skipping')
            continue

        packages.append(pkg)

    registry = {
        'version': '1',
        'updated': datetime.date.today().isoformat(),
        'base_url': 'https://ilma-lang.dev/packages',
        'packages': packages
    }

    registry_path = os.path.join(packages_dir, 'registry.json')
    with open(registry_path, 'w') as f:
        json.dump(registry, f, indent=2)

    print(f'Registry built: {len(packages)} packages -> {registry_path}')
    for pkg in packages:
        print(f'  {pkg["name"]:20} v{pkg["version"]:10} {pkg.get("size_bytes", 0)} bytes')

if __name__ == '__main__':
    main()
