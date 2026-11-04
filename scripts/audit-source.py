"""Check the source-only export, notices and public configuration."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
errors, count = [], 0
excluded = {'.git', 'build', '.claude', '.codex', '__pycache__'}
for path in root.rglob('*'):
    rel = path.relative_to(root)
    if any(part in excluded for part in rel.parts) or rel.parts[:2] == ('tools', 'release'): continue
    if not path.is_file(): continue
    count += 1
    if path.is_symlink(): errors.append(f'Symlink: {rel}')
    if path.suffix.lower() in ('.sv','.rom','.sav','.uf2','.elf','.bin','.o','.obj','.exe','.dll','.zip'):
        errors.append(f'Binary/content excluded from source release: {rel}')
    text = path.read_text(encoding='utf-8', errors='replace')
    if path.resolve() != Path(__file__).resolve():
        if re.search(r'(?i)(?<![A-Za-z])[A-Z]:[/\\]|/(?:Users|home)/[A-Za-z0-9._-]+/', text):
            errors.append(f'Local source reference: {rel}')
        if re.search(r'(?i)\bHW_CONFIG\s*[= ]\s*(?!8\b)\d+\b|/emu/\d+/', text):
            errors.append(f'Private board reference: {rel}')
for notice in ('LICENSE','LICENSE.BSD-3-Clause','THIRD_PARTY.md','dependencies.lock.json',
               'deps/potator/LICENSE','deps/vrEmu6502/LICENSE','deps/pico_shared/LICENSE',
               'deps/pico_shared/drivers/pico_fatfs/LICENSE'):
    if not (root / notice).is_file(): errors.append(f'Missing notice: {notice}')
if errors:
    print('\n'.join(errors))
    sys.exit(1)
print(f'PASS source selection, notices and public paths: {count} files')
