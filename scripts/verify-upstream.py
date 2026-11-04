"""Fetch exact public component revisions and compare selected core sources."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import difflib
import hashlib
import io
import json
import sys
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
pins = json.loads((root / 'dependencies.upstream.json').read_text())

def check(name):
    pin = pins[name]
    repository = pin['url'].removeprefix('https://github.com/')
    url = f"https://codeload.github.com/{repository}/zip/{pin['commit']}"
    request = urllib.request.Request(url, headers={'User-Agent': 'WataraSupervision-source-verification'})
    with urllib.request.urlopen(request, timeout=60) as response:
        archive = zipfile.ZipFile(io.BytesIO(response.read()))
    differences, records = [], []
    for local in sorted((root / 'deps' / name).rglob('*')):
        if not local.is_file(): continue
        relative = local.relative_to(root / 'deps' / name).as_posix()
        candidates = [p for p in archive.namelist() if p.endswith('/' + relative)]
        if len(candidates) != 1:
            raise RuntimeError(f'{name}/{relative}: expected one upstream file, found {candidates}')
        original = archive.read(candidates[0])
        current = local.read_bytes()
        records.append({'path': f'deps/{name}/{relative}',
                        'upstream_path': candidates[0].split('/', 1)[1],
                        'upstream_sha256_lf': hashlib.sha256(original.replace(b'\r\n', b'\n')).hexdigest(),
                        'local_sha256_lf': hashlib.sha256(current.replace(b'\r\n', b'\n')).hexdigest()})
        differences.extend(difflib.unified_diff(
            original.decode('utf-8').splitlines(True), current.decode('utf-8').splitlines(True),
            fromfile=f'upstream/{name}/{relative}', tofile=f'local/{name}/{relative}'))
    if '--write-diffs' in sys.argv:
        (root / 'provenance' / f'{name}-changes.diff').write_text(''.join(differences), encoding='utf-8')
        (root / 'provenance' / f'{name}-files.json').write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
    print(f"PASS public revision {name} {pin['commit']}: {len(records)} selected files", flush=True)
    return records

with ThreadPoolExecutor(max_workers=2) as pool:
    list(pool.map(check, ('potator', 'vrEmu6502')))
