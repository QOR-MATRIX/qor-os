"""Tests for tools/ci/check_hardening.py, on PE and ELF images built here byte by byte."""

from __future__ import annotations

import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("check_hardening", REPO / "tools" / "ci" / "check_hardening.py")
hardening = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(hardening)

ALL_PE = (
    hardening.PE_DYNAMIC_BASE | hardening.PE_HIGH_ENTROPY_VA | hardening.PE_NX_COMPAT | hardening.PE_GUARD_CF
)


def pe(characteristics: int = ALL_PE, cet: bool = True, machine64: bool = True) -> bytes:
    """A PE image with one section holding the debug directory and its extended characteristics."""
    pe_at = 0x40
    optional_size = 240
    section_at = pe_at + 24 + optional_size
    raw = section_at + 40
    rva = 0x1000
    debug_entry = struct.pack(
        "<IIHHIIII",
        0,
        0,
        0,
        0,
        hardening.PE_DEBUG_TYPE_EX_DLLCHARACTERISTICS if cet else 2,
        4,
        rva + 28,
        raw + 28,
    )
    body = debug_entry + struct.pack("<I", hardening.PE_CET_COMPAT)

    image = bytearray(raw + len(body))
    image[0:2] = b"MZ"
    struct.pack_into("<I", image, 0x3c, pe_at)
    image[pe_at : pe_at + 4] = b"PE\0\0"
    struct.pack_into("<H", image, pe_at + 6, 1)
    struct.pack_into("<H", image, pe_at + 20, optional_size)
    optional = pe_at + 24
    struct.pack_into("<H", image, optional, 0x20b if machine64 else 0x10b)
    struct.pack_into("<H", image, optional + 70, characteristics)
    struct.pack_into("<II", image, optional + 112 + 6 * 8, rva, len(debug_entry))
    struct.pack_into("<IIII", image, section_at + 8, len(body), rva, len(body), raw)
    image[raw:] = body
    return bytes(image)


def elf(
    pie: bool = True,
    relro: bool = True,
    now: bool = True,
    stack_flags: int | None = 6,
    canary: bool = True,
    kind: int = hardening.ET_DYN,
    size: int = hardening.ELF_SMALL,
) -> bytes:
    """A little-endian 64-bit ELF image with program headers, a dynamic section and dynamic symbols."""
    names = b"\0" + (b"__stack_chk_fail\0" if canary else b"") + b"puts\0"
    flags_1 = (hardening.DF_1_PIE if pie else 0) | (hardening.DF_1_NOW if now else 0)
    dynamic = struct.pack("<qQ", hardening.DT_FLAGS_1, flags_1) + struct.pack("<qQ", hardening.DT_NULL, 0)

    headers = [(hardening.PT_DYNAMIC, 0)]
    if relro:
        headers.append((hardening.PT_GNU_RELRO, 4))
    if stack_flags is not None:
        headers.append((hardening.PT_GNU_STACK, stack_flags))

    phoff = 64
    phsize = 56
    dynamic_at = phoff + phsize * len(headers)
    names_at = dynamic_at + len(dynamic)
    shoff = names_at + len(names)
    shsize = 64

    image = bytearray(max(shoff + 3 * shsize, size))
    image[0:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<H", image, 16, kind)
    struct.pack_into("<QQ", image, 32, phoff, shoff)
    struct.pack_into("<HHHH", image, 54, phsize, len(headers), shsize, 3)
    for i, (ptype, flags) in enumerate(headers):
        at = phoff + i * phsize
        struct.pack_into("<IIQ", image, at, ptype, flags, dynamic_at if ptype == hardening.PT_DYNAMIC else 0)
        struct.pack_into("<Q", image, at + 32, len(dynamic) if ptype == hardening.PT_DYNAMIC else 0)
    image[dynamic_at:names_at] = dynamic
    image[names_at:shoff] = names
    # Section 1: the dynamic symbols, linked to section 2: their names.
    struct.pack_into("<I", image, shoff + shsize + 4, hardening.SHT_DYNSYM)
    struct.pack_into("<I", image, shoff + shsize + 40, 2)
    struct.pack_into("<I", image, shoff + 2 * shsize + 4, hardening.SHT_STRTAB)
    struct.pack_into("<QQ", image, shoff + 2 * shsize + 24, names_at, len(names))
    return bytes(image)


class Windows(unittest.TestCase):
    def test_a_program_with_every_mitigation_passes(self) -> None:
        self.assertEqual(hardening.pe_problems(pe()), [])

    def test_each_missing_mitigation_is_named(self) -> None:
        for flag, word in (
            (hardening.PE_DYNAMIC_BASE, "DYNAMICBASE"),
            (hardening.PE_HIGH_ENTROPY_VA, "HIGHENTROPYVA"),
            (hardening.PE_NX_COMPAT, "NXCOMPAT"),
            (hardening.PE_GUARD_CF, "guard:cf"),
        ):
            problems = hardening.pe_problems(pe(ALL_PE & ~flag))
            self.assertEqual(len(problems), 1, word)
            self.assertIn(word, problems[0])

    def test_a_program_not_cet_compatible_is_named(self) -> None:
        self.assertEqual(len(hardening.pe_problems(pe(cet=False))), 1)
        self.assertIn("CETCOMPAT", hardening.pe_problems(pe(cet=False))[0])

    def test_what_is_not_a_64_bit_image_is_refused(self) -> None:
        with self.assertRaises(ValueError):
            hardening.pe_problems(pe(machine64=False))
        with self.assertRaises(ValueError):
            hardening.pe_problems(b"ZM" + bytes(0x100))


class Linux(unittest.TestCase):
    def test_a_program_with_every_mitigation_passes(self) -> None:
        self.assertEqual(hardening.elf_problems(elf()), [])

    def test_each_missing_mitigation_is_named(self) -> None:
        for arguments, word in (
            ({"pie": False}, "-pie"),
            ({"kind": 2}, "-pie"),
            ({"relro": False}, "relro"),
            ({"now": False}, "-z now"),
            ({"stack_flags": 7}, "noexecstack"),
            ({"stack_flags": None}, "noexecstack"),
            ({"canary": False}, "stack-protector"),
        ):
            problems = hardening.elf_problems(elf(**arguments))
            self.assertEqual(len(problems), 1, word)
            self.assertIn(word, problems[0])

    def test_a_very_small_program_may_have_nothing_to_protect(self) -> None:
        self.assertEqual(hardening.elf_problems(elf(canary=False, size=0)), [])

    def test_what_is_not_an_elf_file_is_refused(self) -> None:
        with self.assertRaises(ValueError):
            hardening.elf_problems(b"#!/bin/sh\n" + bytes(64))


class Folders(unittest.TestCase):
    def setUp(self) -> None:
        self.dir = tempfile.TemporaryDirectory()
        self.build = Path(self.dir.name)

    def tearDown(self) -> None:
        self.dir.cleanup()

    def put(self, path: str, data: bytes, executable: bool = True) -> None:
        file = self.build / path
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_bytes(data)
        if executable:
            file.chmod(0o755)

    def test_a_weak_program_fails_and_is_named_with_what_it_lacks(self) -> None:
        self.put("good.exe", pe())
        self.put("tests/weak.exe", pe(ALL_PE & ~hardening.PE_GUARD_CF))
        count, failures = hardening.check(self.build)
        self.assertEqual(count, 2)
        self.assertEqual(list(failures), ["tests/weak.exe"])
        self.assertIn("guard:cf", failures["tests/weak.exe"][0])

    def test_what_others_built_and_what_is_not_a_program_are_left_alone(self) -> None:
        self.put("CMakeFiles/3.30/CompilerIdCXX/a.exe", pe(0))
        self.put("_deps/corrosion-build/b.exe", pe(0))
        self.put("cargo/build/build-script.exe", pe(0))
        self.put("notes.txt", b"not a program", executable=False)
        count, failures = hardening.check(self.build)
        self.assertEqual((count, failures), (0, {}))

    def test_what_a_build_no_longer_makes_is_not_counted(self) -> None:
        self.put("current.exe", pe())
        self.put("tests/weak one.exe", pe(ALL_PE & ~hardening.PE_GUARD_CF))
        self.put("stale-from-before-a-rename.exe", pe(0))
        self.put(
            "build.ninja",
            b"build CMakeFiles/x.dir/x.cpp.obj: CXX_COMPILER__x_Release x.cpp\n"
            b"build current.exe | current.pdb: CXX_EXECUTABLE_LINKER__current_Release a.obj\n"
            b"build tests/weak$ one.exe: CXX_EXECUTABLE_LINKER__weak_Release b.obj\n"
            b"build _deps/tool/tool.exe: CXX_EXECUTABLE_LINKER__tool_Release c.obj\n",
            executable=False,
        )
        count, failures = hardening.check(self.build)
        self.assertEqual(count, 2)
        self.assertEqual(list(failures), ["tests/weak one.exe"])

    def test_an_empty_build_folder_fails(self) -> None:
        self.assertEqual(hardening.main(["check_hardening.py", str(self.build)]), 1)


if __name__ == "__main__":
    unittest.main()
