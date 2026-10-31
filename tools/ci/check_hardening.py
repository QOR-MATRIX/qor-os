#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Verifies every program in a build folder was built with QOR OS's exploit mitigations.

cmake/QorHardening.cmake asks the compiler and linker for them; this reads what
came out, so a flag lost in a later change, or a target that never got it, fails
CI instead of shipping (QOR-0018). It reads the files' own headers and needs no
tool but Python.

  Windows (PE):  address randomisation with high entropy, no execution from
                 data, Control Flow Guard, CET shadow-stack compatible.
  Linux (ELF):   position-independent, read-only relocations bound at start,
                 no executable stack, and stack protection. A very small program
                 may hold no function the compiler protects, so that last one
                 is asked only of a program of real size.

Usage: python tools/ci/check_hardening.py <build folder> [<build folder> ...]
Exit code 1 lists each program and what it lacks.
"""

from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

# PE: IMAGE_OPTIONAL_HEADER64.DllCharacteristics
PE_HIGH_ENTROPY_VA = 0x0020
PE_DYNAMIC_BASE = 0x0040
PE_NX_COMPAT = 0x0100
PE_GUARD_CF = 0x4000
# PE: the extended DLL characteristics in the debug directory.
PE_DEBUG_TYPE_EX_DLLCHARACTERISTICS = 20
PE_CET_COMPAT = 0x01

# ELF
ET_DYN = 3
PT_DYNAMIC = 2
PT_GNU_STACK = 0x6474e551
PT_GNU_RELRO = 0x6474e552
PF_X = 1
DT_NULL = 0
DT_FLAGS = 30
DT_FLAGS_1 = 0x6ffffffb
DF_BIND_NOW = 0x8
DF_1_NOW = 0x1
DF_1_PIE = 0x08000000
SHT_DYNSYM = 11
SHT_STRTAB = 3
# Ninja's statement for linking a program: "build <program> | <other outputs>: CXX_EXECUTABLE_LINKER__<target>_<config> ..."
NINJA_LINK = re.compile(r"^build ((?:[^:|$]|\$.)+?)(?: \|[^:]*)?: [A-Z]+_EXECUTABLE_LINKER__")
# Under this many bytes a program may have nothing for -fstack-protector-strong to protect (a test's one-line main).
ELF_SMALL = 128 * 1024

# Not built by QOR OS, or not programs anyone runs: CMake's compiler checks and Rust's own build scripts.
SKIPPED_FOLDERS = {"CMakeFiles", "_deps", "cargo", "__pycache__"}


def pe_problems(data: bytes) -> list[str]:
    """What a 64-bit PE image lacks. Raises ValueError if it is not one."""
    if data[:2] != b"MZ":
        raise ValueError("not a PE image")
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    if data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    sections = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x20b:
        raise ValueError("not a 64-bit image")
    characteristics = struct.unpack_from("<H", data, optional + 70)[0]

    problems = []
    if not characteristics & PE_DYNAMIC_BASE:
        problems.append("no address randomisation (/DYNAMICBASE)")
    if not characteristics & PE_HIGH_ENTROPY_VA:
        problems.append("no high-entropy addresses (/HIGHENTROPYVA)")
    if not characteristics & PE_NX_COMPAT:
        problems.append("data can execute (/NXCOMPAT)")
    if not characteristics & PE_GUARD_CF:
        problems.append("no Control Flow Guard (/guard:cf)")
    if not _pe_cet_compatible(data, optional, optional_size, sections):
        problems.append("not CET compatible (/CETCOMPAT)")
    return problems


def _pe_cet_compatible(data: bytes, optional: int, optional_size: int, sections: int) -> bool:
    rva, size = struct.unpack_from("<II", data, optional + 112 + 6 * 8)  # data directory 6: debug
    section_table = optional + optional_size
    for i in range(sections):
        header = section_table + i * 40
        virtual_size, virtual_address, _, raw_pointer = struct.unpack_from("<IIII", data, header + 8)
        if not virtual_address <= rva < virtual_address + virtual_size:
            continue
        offset = raw_pointer + rva - virtual_address
        for entry in range(offset, offset + size, 28):
            kind, _, _, pointer = struct.unpack_from("<IIII", data, entry + 12)
            if kind == PE_DEBUG_TYPE_EX_DLLCHARACTERISTICS and pointer + 4 <= len(data):
                return bool(struct.unpack_from("<I", data, pointer)[0] & PE_CET_COMPAT)
    return False


def elf_problems(data: bytes) -> list[str]:
    """What a little-endian 64-bit ELF program lacks. Raises ValueError if it is not one."""
    if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 1:
        raise ValueError("not a 64-bit ELF file")
    kind = struct.unpack_from("<H", data, 16)[0]
    phoff, shoff = struct.unpack_from("<QQ", data, 32)
    phsize, phnum, shsize, shnum = struct.unpack_from("<HHHH", data, 54)

    has_relro = False
    stack_executable = True  # no PT_GNU_STACK means the loader assumes an executable stack
    dynamic = None
    for i in range(phnum):
        entry = phoff + i * phsize
        ptype, flags, offset = struct.unpack_from("<IIQ", data, entry)
        filesz = struct.unpack_from("<Q", data, entry + 32)[0]
        if ptype == PT_GNU_RELRO:
            has_relro = True
        elif ptype == PT_GNU_STACK:
            stack_executable = bool(flags & PF_X)
        elif ptype == PT_DYNAMIC:
            dynamic = (offset, filesz)

    bind_now = False
    pie = False
    if dynamic:
        offset, size = dynamic
        for at in range(offset, offset + size, 16):
            tag, value = struct.unpack_from("<qQ", data, at)
            if tag == DT_NULL:
                break
            if tag == DT_FLAGS and value & DF_BIND_NOW:
                bind_now = True
            if tag == DT_FLAGS_1:
                bind_now = bind_now or bool(value & DF_1_NOW)
                pie = bool(value & DF_1_PIE)

    problems = []
    if kind != ET_DYN or not pie:
        problems.append("not position-independent (-pie)")
    if not has_relro:
        problems.append("no read-only relocations (-z relro)")
    if not bind_now:
        problems.append("relocations not bound at start (-z now)")
    if stack_executable:
        problems.append("executable stack (-z noexecstack)")

    names = _elf_imports(data, shoff, shsize, shnum)
    if len(data) >= ELF_SMALL and names and "__stack_chk_fail" not in names:
        problems.append("no stack protection (-fstack-protector-strong)")
    return problems


def _elf_imports(data: bytes, shoff: int, shsize: int, shnum: int) -> set[str]:
    """The names in the dynamic symbol table."""
    for i in range(shnum):
        header = shoff + i * shsize
        stype = struct.unpack_from("<I", data, header + 4)[0]
        if stype != SHT_DYNSYM:
            continue
        link = struct.unpack_from("<I", data, header + 40)[0]
        strings = shoff + link * shsize
        if struct.unpack_from("<I", data, strings + 4)[0] != SHT_STRTAB:
            return set()
        offset, size = struct.unpack_from("<QQ", data, strings + 24)
        table = data[offset : offset + size]
        return {name.decode("utf-8", "replace") for name in table.split(b"\0") if name}
    return set()


def _built_by_ninja(build: Path) -> list[Path]:
    """The programs a build folder's own build.ninja makes (several, for the products' build root)."""
    found = []
    for ninja in sorted(build.rglob("build.ninja")):
        if SKIPPED_FOLDERS & set(ninja.relative_to(build).parts):
            continue
        for line in ninja.read_text(encoding="utf-8", errors="replace").splitlines():
            link = NINJA_LINK.match(line)
            if not link:
                continue
            path = ninja.parent / link.group(1).replace("$ ", " ").replace("$:", ":")
            if path.is_file() and not SKIPPED_FOLDERS & set(path.relative_to(build).parts):
                found.append(path)
    return found


def programs(build: Path) -> list[Path]:
    """The programs a build folder makes. Not everything in it: CI keeps its build folders, and a program a build no
    longer makes stays there unchecked and unused. With no build.ninja to ask, every .exe, or executable ELF file."""
    made = _built_by_ninja(build)
    if made:
        return made
    found = []
    for path in sorted(build.rglob("*")):
        if not path.is_file() or SKIPPED_FOLDERS & set(path.relative_to(build).parts):
            continue
        if path.suffix == ".exe":
            found.append(path)
        elif not path.suffix and path.stat().st_mode & 0o111:
            with path.open("rb") as handle:
                if handle.read(4) == b"\x7fELF":
                    found.append(path)
    return found


def check(build: Path) -> tuple[int, dict[str, list[str]]]:
    """How many programs were read, and what each that falls short lacks."""
    failures: dict[str, list[str]] = {}
    found = programs(build)
    for path in found:
        data = path.read_bytes()
        problems = pe_problems(data) if path.suffix == ".exe" else elf_problems(data)
        if problems:
            failures[path.relative_to(build).as_posix()] = problems
    return len(found), failures


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    failed = False
    for folder in argv[1:]:
        count, failures = check(Path(folder))
        if count == 0:
            print(f"{folder}: no programs found", file=sys.stderr)
            failed = True
        for name, problems in failures.items():
            print(f"{folder}/{name}: {'; '.join(problems)}", file=sys.stderr)
        failed = failed or bool(failures)
        print(f"{folder}: {count} programs read, {len(failures)} fall short")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
