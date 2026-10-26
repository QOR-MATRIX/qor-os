#!/usr/bin/env python3
"""Builds and checks the QTOS Store's signed first-party catalogue.

    python tools/store/build_store.py build      # sign changed packages and the catalogue
    python tools/store/build_store.py check      # verify store/catalog against store/content (CI)
    python tools/store/build_store.py fixtures   # regenerate tests/store/fixtures

store/content/ holds the sources: developers.json, then one folder per item
under themes/ or widgets/ with a listing.json (the catalogue text) and the
files the package carries. `build` writes store/catalog/: one .qtospkg per
item and catalog.json with its signature, catalog.json.sig.

Signing keys (ECDSA P-256) never enter the repository. They live in --keys
(default ~/.qtos/store-signing): store.pem signs the catalogue and
developer-<id>.pem signs that developer's packages. `build` creates missing
keys and writes the store's public key to store/keys/store.pub, which QTOS
embeds as its root of trust. Items whose content is unchanged keep their
existing signed package, so a rebuild changes nothing. See docs/STORE.md.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import io
import json
import os
import re
import sys
import zipfile
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature, encode_dss_signature

REPO = Path(__file__).resolve().parents[2]
CONTENT = REPO / "store" / "content"
CATALOG = REPO / "store" / "catalog"
STORE_PUB = REPO / "store" / "keys" / "store.pub"
FIXTURES = REPO / "tests" / "store" / "fixtures"
DEFAULT_KEYS = Path.home() / ".qtos" / "store-signing"

ID_PATTERN = re.compile(r"^[a-z][a-z0-9]*(\.[a-z0-9][a-z0-9-]*){2,}$")
VERSION_PATTERN = re.compile(r"^\d{1,5}\.\d{1,5}\.\d{1,5}$")
KINDS = {"themes": "theme", "widgets": "widget"}
ZIP_TIME = (1980, 1, 1, 0, 0, 0)


# ── Keys and signatures ─────────────────────────────────────────────────────

def load_or_create_key(path: Path) -> ec.EllipticCurvePrivateKey:
    if path.exists():
        key = serialization.load_pem_private_key(path.read_bytes(), password=None)
        assert isinstance(key, ec.EllipticCurvePrivateKey) and isinstance(key.curve, ec.SECP256R1)
        return key
    path.parent.mkdir(parents=True, exist_ok=True)
    key = ec.generate_private_key(ec.SECP256R1())
    pem = key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
                            serialization.NoEncryption())
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as out:
        out.write(pem)
    print(f"Created signing key {path}")
    return key


def public_point(key: ec.EllipticCurvePrivateKey) -> bytes:
    return key.public_key().public_bytes(serialization.Encoding.X962,
                                         serialization.PublicFormat.UncompressedPoint)


def sign(key: ec.EllipticCurvePrivateKey, data: bytes) -> bytes:
    r, s = decode_dss_signature(key.sign(data, ec.ECDSA(hashes.SHA256())))
    return r.to_bytes(32, "big") + s.to_bytes(32, "big")


def verify(point: bytes, data: bytes, signature: bytes) -> bool:
    if len(point) != 65 or len(signature) != 64:
        return False
    public = ec.EllipticCurvePublicKey.from_encoded_point(ec.SECP256R1(), point)
    der = encode_dss_signature(int.from_bytes(signature[:32], "big"), int.from_bytes(signature[32:], "big"))
    try:
        public.verify(der, data, ec.ECDSA(hashes.SHA256()))
        return True
    except InvalidSignature:
        return False


# ── Content ─────────────────────────────────────────────────────────────────

class Item:
    def __init__(self, kind: str, listing: dict, files: dict[str, bytes]):
        self.kind = kind
        self.listing = listing
        self.files = files

    @property
    def id(self) -> str:
        return self.listing["id"]

    @property
    def version(self) -> str:
        return self.listing["version"]

    @property
    def package_name(self) -> str:
        return f"{self.id}-{self.version}.qtospkg"

    def preview(self) -> dict:
        if self.kind == "theme":
            theme = json.loads(self.files["theme.json"])
            return {
                "accentHue": theme["accentHue"],
                "appearance": theme.get("appearance", "any"),
                "neutralHue": theme.get("neutral", {}).get("hue", 262),
                "neutralTint": theme.get("neutral", {}).get("tint", 1),
                "wallpaperSeed": theme.get("wallpaper", {}).get("seed", 42),
                "wallpaperIntensity": theme.get("wallpaper", {}).get("intensity", 1),
            }
        widget = json.loads(self.files["widget.json"])
        return {"size": widget["size"], "glyph": widget.get("glyph", "layout-grid"), "hue": widget.get("hue", 274)}

    def manifest(self) -> bytes:
        manifest = {
            "format": 1,
            "id": self.id,
            "kind": self.kind,
            "version": self.version,
            "name": self.listing["name"],
            "developer": self.listing["developer"],
            "files": {path: hashlib.sha256(data).hexdigest() for path, data in sorted(self.files.items())},
        }
        return (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")


def validate(item: Item, developers: dict) -> None:
    where = f"{item.kind} {item.listing.get('id', '?')}"
    for field in ("id", "name", "version", "developer", "summary", "description", "released"):
        if not item.listing.get(field):
            raise SystemExit(f"{where}: listing.json needs '{field}'")
    if not ID_PATTERN.match(item.id):
        raise SystemExit(f"{where}: invalid id")
    if not VERSION_PATTERN.match(item.version):
        raise SystemExit(f"{where}: version must be major.minor.patch")
    if item.listing["developer"] not in developers:
        raise SystemExit(f"{where}: unknown developer {item.listing['developer']}")
    if item.kind == "theme" and "theme.json" not in item.files:
        raise SystemExit(f"{where}: a theme needs theme.json")
    if item.kind == "widget":
        widget = json.loads(item.files.get("widget.json", b"{}"))
        if widget.get("entry") not in item.files or widget.get("size") not in ("small", "medium", "large"):
            raise SystemExit(f"{where}: widget.json needs an entry file and a size (small, medium, large)")


def read_content(content: Path) -> tuple[dict, list[Item]]:
    developers = json.loads((content / "developers.json").read_text(encoding="utf-8"))
    items = []
    for folder, kind in KINDS.items():
        for directory in sorted(p for p in (content / folder).glob("*") if p.is_dir()):
            listing = json.loads((directory / "listing.json").read_text(encoding="utf-8"))
            files = {path.relative_to(directory).as_posix(): path.read_bytes()
                     for path in sorted(directory.rglob("*"))
                     if path.is_file() and path.name != "listing.json"}
            item = Item(kind, listing, files)
            validate(item, developers)
            items.append(item)
    ids = [item.id for item in items]
    if len(ids) != len(set(ids)):
        raise SystemExit("duplicate item ids in content")
    return developers, items


# ── Packages and catalogue ──────────────────────────────────────────────────

def make_package(item: Item, manifest: bytes, signature: bytes) -> bytes:
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_STORED) as archive:
        entries = [("manifest.json", manifest), ("manifest.sig", signature)] + sorted(item.files.items())
        for name, data in entries:
            info = zipfile.ZipInfo(name, date_time=ZIP_TIME)
            info.external_attr = 0o644 << 16
            archive.writestr(info, data)
    return buffer.getvalue()


def read_package(data: bytes) -> dict[str, bytes]:
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        return {info.filename: archive.read(info) for info in archive.infolist() if not info.is_dir()}


def catalog_entry(item: Item, package: bytes) -> dict:
    listing = item.listing
    return {
        "id": item.id,
        "kind": item.kind,
        "name": listing["name"],
        "developer": listing["developer"],
        "version": item.version,
        "summary": listing["summary"],
        "description": listing["description"],
        "whatsNew": listing.get("whatsNew", ""),
        "released": listing["released"],
        "tags": listing.get("tags", []),
        "featured": bool(listing.get("featured", False)),
        "price": listing.get("price", 0),
        "preview": item.preview(),
        "package": {"file": f"packages/{item.package_name}", "size": len(package),
                    "sha256": hashlib.sha256(package).hexdigest()},
    }


def catalog_bytes(developers: dict, developer_keys: dict[str, bytes], entries: list[dict]) -> bytes:
    catalog = {
        "format": 1,
        "store": "QTOS Store",
        "developers": {
            developer_id: {"name": info["name"], "verified": bool(info.get("verified", False)),
                           "key": base64.b64encode(developer_keys[developer_id]).decode("ascii")}
            for developer_id, info in sorted(developers.items())
        },
        "items": sorted(entries, key=lambda e: (e["kind"], e["name"].lower())),
    }
    return (json.dumps(catalog, indent=2, ensure_ascii=False) + "\n").encode("utf-8")


def write_if_changed(path: Path, data: bytes) -> bool:
    if path.exists() and path.read_bytes() == data:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return True


def build(content: Path, out: Path, store_key: ec.EllipticCurvePrivateKey,
          developer_keys: dict[str, ec.EllipticCurvePrivateKey], store_pub: Path | None) -> None:
    developers, items = read_content(content)
    points = {developer_id: public_point(key) for developer_id, key in developer_keys.items()}
    packages_dir = out / "packages"
    packages_dir.mkdir(parents=True, exist_ok=True)
    entries = []
    written = 0
    for item in items:
        manifest = item.manifest()
        path = packages_dir / item.package_name
        package = None
        if path.exists():
            # Unchanged content keeps its signed package.
            existing = read_package(path.read_bytes())
            if existing.get("manifest.json") == manifest and verify(points[item.listing["developer"]], manifest,
                                                                      existing.get("manifest.sig", b"")):
                package = path.read_bytes()
        if package is None:
            package = make_package(item, manifest, sign(developer_keys[item.listing["developer"]], manifest))
            path.write_bytes(package)
            written += 1
        entries.append(catalog_entry(item, package))
    wanted = {item.package_name for item in items}
    for stale in packages_dir.glob("*.qtospkg"):
        if stale.name not in wanted:
            stale.unlink()
            print(f"Removed {stale.name}")

    catalog = catalog_bytes(developers, points, entries)
    catalog_path = out / "catalog.json"
    signature_path = out / "catalog.json.sig"
    if not (catalog_path.exists() and catalog_path.read_bytes() == catalog and signature_path.exists()
            and verify(public_point(store_key), catalog, signature_path.read_bytes())):
        catalog_path.write_bytes(catalog)
        signature_path.write_bytes(sign(store_key, catalog))
        print(f"Signed {catalog_path.relative_to(REPO) if catalog_path.is_relative_to(REPO) else catalog_path}")
    if store_pub is not None and write_if_changed(store_pub, base64.b64encode(public_point(store_key)) + b"\n"):
        print(f"Wrote the store's public key to {store_pub.relative_to(REPO)}")
    print(f"{len(items)} items, {written} package(s) signed")


# ── Check ───────────────────────────────────────────────────────────────────

def check(content: Path, out: Path, store_pub: Path) -> int:
    errors: list[str] = []
    store_point = base64.b64decode(store_pub.read_text(encoding="ascii").strip())
    catalog = (out / "catalog.json").read_bytes()
    if not verify(store_point, catalog, (out / "catalog.json.sig").read_bytes()):
        errors.append("catalog.json does not carry the store's signature")
    parsed = json.loads(catalog)
    developers, items = read_content(content)
    keys = {d: base64.b64decode(info["key"]) for d, info in parsed["developers"].items()}
    for developer_id, info in developers.items():
        listed = parsed["developers"].get(developer_id)
        if not listed or listed["name"] != info["name"] or listed["verified"] != bool(info.get("verified")):
            errors.append(f"developer {developer_id} differs from developers.json")

    by_id = {entry["id"]: entry for entry in parsed["items"]}
    for item in items:
        entry = by_id.pop(item.id, None)
        if entry is None:
            errors.append(f"{item.id} is not in the catalogue: run build")
            continue
        path = out / entry["package"]["file"]
        if not path.exists():
            errors.append(f"{item.id}: {path.name} is missing")
            continue
        package = path.read_bytes()
        expected = catalog_entry(item, package)
        for field in ("name", "kind", "developer", "version", "summary", "description", "whatsNew",
                      "released", "tags", "featured", "price", "preview", "package"):
            if entry.get(field) != expected[field]:
                errors.append(f"{item.id}: catalogue '{field}' differs from the content: run build")
        files = read_package(package)
        manifest = item.manifest()
        if files.get("manifest.json") != manifest:
            errors.append(f"{item.id}: the package does not match the content: run build")
        elif not verify(keys.get(item.listing["developer"], b""), manifest, files.get("manifest.sig", b"")):
            errors.append(f"{item.id}: the manifest does not carry its developer's signature")
        if {k: v for k, v in files.items() if k not in ("manifest.json", "manifest.sig")} != item.files:
            errors.append(f"{item.id}: the package's files differ from the content")
    for leftover in by_id:
        errors.append(f"{leftover} is in the catalogue but not in the content")
    listed = {Path(entry["package"]["file"]).name for entry in parsed["items"]}
    for package in (out / "packages").glob("*.qtospkg"):
        if package.name not in listed:
            errors.append(f"{package.name} is not in the catalogue")

    for error in errors:
        print(f"error: {error}")
    print(f"{len(items)} items checked, {len(errors)} error(s)")
    return 1 if errors else 0


# ── Test fixtures ───────────────────────────────────────────────────────────

FIXTURE_WIDGET = b"""// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick

Rectangle { color: "teal" }
"""


def fixture_content(root: Path, theme_version: str, widget: bool) -> Path:
    content = root / "content"
    (content / "themes" / "test").mkdir(parents=True)
    (content / "developers.json").write_text(json.dumps({"org.example": {"name": "Example", "verified": True}}))
    listing = {"id": "org.example.theme.test", "name": "Test Theme", "version": theme_version,
               "developer": "org.example", "summary": "A test theme.", "description": "For tests.",
               "released": "2026-10-26"}
    (content / "themes" / "test" / "listing.json").write_text(json.dumps(listing))
    theme = {"accentHue": 120 if theme_version == "1.0.0" else 200, "appearance": "dark",
             "neutral": {"hue": 100, "tint": 1.2}, "wallpaper": {"style": "aurora", "seed": 3, "intensity": 0.9}}
    (content / "themes" / "test" / "theme.json").write_text(json.dumps(theme))
    if widget:
        directory = content / "widgets" / "test"
        directory.mkdir(parents=True)
        (directory / "listing.json").write_text(json.dumps(
            {"id": "org.example.widget.test", "name": "Test Widget", "version": "1.0.0", "developer": "org.example",
             "summary": "A test widget.", "description": "For tests.", "released": "2026-10-26"}))
        (directory / "widget.json").write_text(json.dumps({"entry": "Test.qml", "size": "small"}))
        (directory / "Test.qml").write_bytes(FIXTURE_WIDGET)
    return content


def fixtures() -> None:
    import shutil
    import tempfile

    # Throwaway keys: tests only ever need the store's public key.
    store_key = ec.generate_private_key(ec.SECP256R1())
    developer_key = ec.generate_private_key(ec.SECP256R1())
    rogue_key = ec.generate_private_key(ec.SECP256R1())
    if FIXTURES.exists():
        shutil.rmtree(FIXTURES)
    FIXTURES.mkdir(parents=True)
    (FIXTURES / "store.pub").write_bytes(base64.b64encode(public_point(store_key)) + b"\n")
    with tempfile.TemporaryDirectory() as temp:
        temp_path = Path(temp)
        build(fixture_content(temp_path / "a", "1.0.0", True), FIXTURES / "v1", store_key,
              {"org.example": developer_key}, None)
        build(fixture_content(temp_path / "b", "1.1.0", True), FIXTURES / "v2", store_key,
              {"org.example": developer_key}, None)
        # A store-signed catalogue listing a package whose manifest a
        # different key signed.
        content = fixture_content(temp_path / "c", "1.0.0", False)
        build(content, FIXTURES / "forged", store_key, {"org.example": rogue_key}, None)
        developers, items = read_content(content)
        entries = [catalog_entry(item, (FIXTURES / "forged" / "packages" / item.package_name).read_bytes())
                   for item in items]
        forged = catalog_bytes(developers, {"org.example": public_point(developer_key)}, entries)
        (FIXTURES / "forged" / "catalog.json").write_bytes(forged)
        (FIXTURES / "forged" / "catalog.json.sig").write_bytes(sign(store_key, forged))
        # A catalogue the store did not sign.
        build(fixture_content(temp_path / "d", "1.0.0", False), FIXTURES / "unsigned", rogue_key,
              {"org.example": developer_key}, None)
    print(f"Wrote {FIXTURES.relative_to(REPO)}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["build", "check", "fixtures"])
    parser.add_argument("--keys", type=Path, default=DEFAULT_KEYS, help="folder of signing keys (build)")
    args = parser.parse_args(argv)

    if args.command == "check":
        return check(CONTENT, CATALOG, STORE_PUB)
    if args.command == "fixtures":
        fixtures()
        return 0
    developers, _ = read_content(CONTENT)
    store_key = load_or_create_key(args.keys / "store.pem")
    developer_keys = {d: load_or_create_key(args.keys / f"developer-{d}.pem") for d in developers}
    build(CONTENT, CATALOG, store_key, developer_keys, STORE_PUB)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
