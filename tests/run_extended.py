"""Build and run supplied and extra checks with bounded runtimes and saved logs."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
parser.add_argument("--timeout", type=int, default=180,
                    help="maximum seconds per compilation or test program")
parser.add_argument("--output", type=Path,
                    help="new directory for binaries and logs (default: temporary directory)")
parser.add_argument("--modes", nargs="+", choices=["plain", "tsan", "asan"],
                    default=["plain", "tsan", "asan"])
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
if args.output:
    args.output.mkdir(parents=True, exist_ok=False)
    output = args.output.resolve()
else:
    output = Path(tempfile.mkdtemp(prefix="lab1-validation-"))
print(f"Results: {output}", flush=True)
flags = {
    "plain": ["-O2"],
    "tsan": ["-O1", "-g", "-fsanitize=thread"],
    "asan": ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"],
}
env = os.environ.copy()
env["TSAN_OPTIONS"] = "halt_on_error=1"
env["ASAN_OPTIONS"] = "halt_on_error=1"
env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"


def run(command, name):
    start = time.monotonic()
    path = output / f"{name}.log"
    with path.open("w") as log:
        log.write(shlex.join(command) + "\n")
        log.flush()
        try:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                    env=env, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            raise SystemExit(f"TIMEOUT: {name}; see {path}")
    if result.returncode:
        print(path.read_text()[-6000:])
        raise SystemExit(f"FAIL: {name}, status {result.returncode}; see {path}")
    print(f"PASS: {name} ({time.monotonic() - start:.1f}s)", flush=True)


run(shlex.split(args.cxx) + ["--version"], "compiler")
for mode in args.modes:
    for test in ["test_map", "test_locks", "test_extended"]:
        binary = output / f"{test}_{mode}"
        command = shlex.split(args.cxx) + ["-std=c++20", "-Wall", "-Wextra", "-pthread"]
        command += flags[mode] + ["-I", str(root / "starter_files"),
                                str(root / "tests" / f"{test}.cpp"), "-o", str(binary)]
        run(command, f"build_{test}_{mode}")
        run([str(binary)], f"run_{test}_{mode}")
print(f"All requested checks passed. Logs: {output}", flush=True)
