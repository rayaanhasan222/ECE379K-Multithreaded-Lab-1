#!/usr/bin/env python3
"""Package the final lab report, source, and indexed measurement evidence."""
from pathlib import Path
import csv
import hashlib
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Lab1_Submission.zip'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    report = ROOT / 'Lab1 Report.pdf'
    if not report.is_file():
        raise SystemExit('Missing Lab1 Report.pdf')
    parts = (ROOT / 'starter_files/parts.h').read_text()
    for switch in ('HAVE_SHARDED', 'HAVE_LOCKS', 'HAVE_RW', 'HAVE_HASHED'):
        if not re.search(rf'^#define\s+{switch}\s+1\b', parts, re.M):
            raise SystemExit(f'{switch} must be enabled')
    with (ROOT / 'raw_measurements/INDEX.csv').open(newline='') as f:
        rows = list(csv.DictReader(f))
    for row in rows:
        copied = ROOT / 'raw_measurements' / row['copied_relative_path']
        original = ROOT / row['original_relative_path']
        if digest(copied) != row['sha256'] or copied.read_bytes() != original.read_bytes():
            raise SystemExit(f'Measurement copy differs: {copied}')
    files = {'Lab1 Report.pdf': report, 'README.md': ROOT / 'README.md'}
    for name in ('concurrent_map.h', 'locks.h', 'hash_map.h', 'parts.h',
                 'interface.h', 'bench.cpp', 'Makefile', 'sweep.sh', 'perfstat.sh'):
        files['starter_files/' + name] = ROOT / 'starter_files' / name
    for name in ('test_map.cpp', 'test_locks.cpp', 'test_extended.cpp',
                 'run_extended.py', 'diagnose_size.cpp', 'EXTENDED_TESTS.md'):
        files['tests/' + name] = ROOT / 'tests' / name
    for path in (ROOT / 'raw_measurements').rglob('*'):
        if path.is_file():
            files[path.relative_to(ROOT).as_posix()] = path
    for name in ('DELIVERABLES_CHECKLIST.md', 'build_zip.py'):
        files['submission/' + name] = ROOT / 'submission' / name
    # Keep the requested part-specific copies, and supply the canonical harness
    # filenames in the ZIP as well, in separate directories for each experiment.
    for row in rows:
        if Path(row['copied_relative_path']).name in ('sharded_mutex_part_4.csv', 'sharded_mutex_part_5.csv'):
            files['raw_measurements/' + row['original_relative_path']] = ROOT / 'raw_measurements' / row['copied_relative_path']
    manifest = ''.join(f'{digest(path)}  {name}\n' for name, path in sorted(files.items()))
    with zipfile.ZipFile(OUT, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, path in sorted(files.items()):
            z.write(path, 'Lab1_Submission/' + name)
        z.writestr('Lab1_Submission/PACKAGE_SHA256SUMS', manifest)
    with zipfile.ZipFile(OUT) as z:
        assert z.testzip() is None
        for name, path in files.items():
            assert z.read('Lab1_Submission/' + name) == path.read_bytes()
        assert not any('/.git/' in n or '.DS_Store' in n or '.dSYM/' in n for n in z.namelist())
    print(f'Created {OUT.name}: {len(files)+1} entries, {OUT.stat().st_size:,} bytes')
    print(f'ZIP SHA-256: {digest(OUT)}')
    print('All ZIP entries verified against their source bytes; all parts enabled.')

if __name__ == '__main__':
    main()
