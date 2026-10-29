#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""A Secret Service of the run's own: freedesktop.org's keyring, in memory.

tools/dev/products.sh starts it on a session bus of the run's own, so QQ
Studio's agent suite keeps its key in a keyring (QOR-0011) without reaching
the developer's. It serves what a client of the specification needs:

- plain sessions;
- the default collection;
- items found by their attributes, created (or replaced), read and deleted;
- unlocking through a prompt.

The collection starts locked, and its prompt answers at once, so every run
takes the path a locked keyring takes. What it holds is never written
anywhere or printed. Needs python3-dbus and python3-gi:

    dbus-run-session -- python3 tools/dev/secret-service.py
"""
from __future__ import annotations

import itertools
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

BUS_NAME = "org.freedesktop.secrets"
ROOT = "/org/freedesktop/secrets"
SERVICE = "org.freedesktop.Secret.Service"
COLLECTION = "org.freedesktop.Secret.Collection"
ITEM = "org.freedesktop.Secret.Item"
SESSION = "org.freedesktop.Secret.Session"
PROMPT = "org.freedesktop.Secret.Prompt"
PROPERTIES = "org.freedesktop.DBus.Properties"
NO_PROMPT = dbus.ObjectPath("/")


class IsLocked(dbus.DBusException):
    _dbus_error_name = "org.freedesktop.Secret.Error.IsLocked"


class NoSession(dbus.DBusException):
    _dbus_error_name = "org.freedesktop.Secret.Error.NoSession"


class NoSuchObject(dbus.DBusException):
    _dbus_error_name = "org.freedesktop.Secret.Error.NoSuchObject"


class NotSupported(dbus.DBusException):
    _dbus_error_name = "org.freedesktop.DBus.Error.NotSupported"


def paths(objects) -> dbus.Array:
    return dbus.Array([dbus.ObjectPath(o.path) for o in objects], signature="o")


class Session(dbus.service.Object):
    def __init__(self, keyring: Keyring, path: str) -> None:
        super().__init__(keyring.bus, path)
        self.keyring = keyring
        self.path = path

    @dbus.service.method(SESSION)
    def Close(self) -> None:
        self.keyring.sessions.pop(self.path, None)
        self.remove_from_connection()


class Prompt(dbus.service.Object):
    """Answers at once: unlocks what it was made for, then says so."""

    def __init__(self, keyring: Keyring, path: str, answer) -> None:
        super().__init__(keyring.bus, path)
        self.path = path
        self.answer = answer

    @dbus.service.method(PROMPT, in_signature="s")
    def Prompt(self, window_id: str) -> None:
        result = self.answer()
        GLib.idle_add(self.complete, False, result)  # after the reply, as a real prompt's answer comes

    @dbus.service.method(PROMPT)
    def Dismiss(self) -> None:
        GLib.idle_add(self.complete, True, dbus.Array([], signature="o"))

    def complete(self, dismissed: bool, result) -> bool:
        self.Completed(dismissed, result)
        self.remove_from_connection()
        return False

    @dbus.service.signal(PROMPT, signature="bv")
    def Completed(self, dismissed, result) -> None:
        pass


class Item(dbus.service.Object):
    def __init__(self, keyring: Keyring, path: str, label: str, attributes: dict, secret: bytes,
                 content_type: str) -> None:
        super().__init__(keyring.bus, path)
        self.keyring = keyring
        self.path = path
        self.label = label
        self.attributes = attributes
        self.secret = secret
        self.content_type = content_type

    def matches(self, attributes: dict) -> bool:
        return all(self.attributes.get(k) == v for k, v in attributes.items())

    @dbus.service.method(ITEM, in_signature="o", out_signature="(oayays)")
    def GetSecret(self, session):
        self.keyring.check(session)
        return (session, dbus.ByteArray(b""), dbus.ByteArray(self.secret), self.content_type)

    @dbus.service.method(ITEM, in_signature="(oayays)", byte_arrays=True)
    def SetSecret(self, secret) -> None:
        self.keyring.check(secret[0])
        self.secret, self.content_type = bytes(secret[2]), str(secret[3])

    @dbus.service.method(ITEM, out_signature="o")
    def Delete(self):
        self.keyring.check_unlocked()
        self.keyring.items.remove(self)
        self.remove_from_connection()
        return NO_PROMPT

    @dbus.service.method(PROPERTIES, in_signature="ss", out_signature="v")
    def Get(self, interface: str, name: str):
        return self.GetAll(interface)[name]

    @dbus.service.method(PROPERTIES, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface: str):
        return {"Label": self.label, "Locked": self.keyring.locked,
                "Attributes": dbus.Dictionary(self.attributes, signature="ss")}


class Collection(dbus.service.Object):
    def __init__(self, keyring: Keyring, path: str) -> None:
        super().__init__(keyring.bus, path)
        self.keyring = keyring
        self.path = path

    @dbus.service.method(COLLECTION, in_signature="a{sv}(oayays)b", out_signature="oo", byte_arrays=True)
    def CreateItem(self, properties, secret, replace):
        self.keyring.check_unlocked()
        self.keyring.check(secret[0])
        attributes = {str(k): str(v) for k, v in properties.get(ITEM + ".Attributes", {}).items()}
        label = str(properties.get(ITEM + ".Label", ""))
        same = [i for i in self.keyring.items if i.attributes == attributes] if replace else []
        if same:
            item = same[0]
            item.label, item.secret, item.content_type = label, bytes(secret[2]), str(secret[3])
        else:
            item = Item(self.keyring, f"{self.path}/{next(self.keyring.serial)}", label, attributes,
                        bytes(secret[2]), str(secret[3]))
            self.keyring.items.append(item)
        return (dbus.ObjectPath(item.path), NO_PROMPT)

    @dbus.service.method(COLLECTION, in_signature="a{ss}", out_signature="ao")
    def SearchItems(self, attributes):
        return paths(self.keyring.matching(attributes))

    @dbus.service.method(PROPERTIES, in_signature="ss", out_signature="v")
    def Get(self, interface: str, name: str):
        return self.GetAll(interface)[name]

    @dbus.service.method(PROPERTIES, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface: str):
        return {"Label": "Login", "Locked": self.keyring.locked, "Items": paths(self.keyring.items)}


class Keyring(dbus.service.Object):
    """The service itself, with one collection, the default."""

    def __init__(self, bus: dbus.Bus) -> None:
        super().__init__(bus, ROOT)
        self.bus = bus
        self.serial = itertools.count(1)
        self.sessions: dict[str, Session] = {}
        self.items: list[Item] = []
        self.locked = True
        self.collection = Collection(self, ROOT + "/collection/login")

    def check(self, session) -> None:
        if str(session) not in self.sessions:
            raise NoSession("No such session.")

    def check_unlocked(self) -> None:
        if self.locked:
            raise IsLocked("The collection is locked.")

    def matching(self, attributes) -> list[Item]:
        wanted = {str(k): str(v) for k, v in attributes.items()}
        return [i for i in self.items if i.matches(wanted)]

    def known(self, objects) -> list:
        mine = {self.collection.path} | {i.path for i in self.items}
        return [o for o in objects if str(o) in mine]

    @dbus.service.method(SERVICE, in_signature="sv", out_signature="vo")
    def OpenSession(self, algorithm: str, _input):
        if algorithm != "plain":
            raise NotSupported("Only plain sessions are served here.")
        path = f"{ROOT}/session/{next(self.serial)}"
        self.sessions[path] = Session(self, path)
        return (dbus.String("", variant_level=1), dbus.ObjectPath(path))

    @dbus.service.method(SERVICE, in_signature="a{ss}", out_signature="aoao")
    def SearchItems(self, attributes):
        found = paths(self.matching(attributes))
        none = dbus.Array([], signature="o")
        return (none, found) if self.locked else (found, none)

    @dbus.service.method(SERVICE, in_signature="ao", out_signature="aoo")
    def Unlock(self, objects):
        known = dbus.Array(self.known(objects), signature="o")
        if not self.locked:
            return (known, NO_PROMPT)

        def answer():
            self.locked = False
            return known

        prompt = Prompt(self, f"{ROOT}/prompt/{next(self.serial)}", answer)
        return (dbus.Array([], signature="o"), dbus.ObjectPath(prompt.path))

    @dbus.service.method(SERVICE, in_signature="ao", out_signature="aoo")
    def Lock(self, objects):
        self.locked = True
        return (dbus.Array(self.known(objects), signature="o"), NO_PROMPT)

    @dbus.service.method(SERVICE, in_signature="aoo", out_signature="a{o(oayays)}")
    def GetSecrets(self, items, session):
        self.check(session)
        self.check_unlocked()
        found = {i.path: i for i in self.items}
        return {dbus.ObjectPath(p): (session, dbus.ByteArray(b""), dbus.ByteArray(found[str(p)].secret),
                                     found[str(p)].content_type)
                for p in items if str(p) in found}

    @dbus.service.method(SERVICE, in_signature="s", out_signature="o")
    def ReadAlias(self, name: str):
        return dbus.ObjectPath(self.collection.path if name == "default" else "/")


def main() -> int:
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    try:
        name = dbus.service.BusName(BUS_NAME, bus, do_not_queue=True)
    except dbus.exceptions.NameExistsException:
        print(f"{BUS_NAME} is already served on this bus.", file=sys.stderr)
        return 1
    keyring = Keyring(bus)
    print("Secret Service ready", flush=True)
    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    del name, keyring
    return 0


if __name__ == "__main__":
    sys.exit(main())
