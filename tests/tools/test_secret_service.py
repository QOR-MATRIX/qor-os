# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Tests for tools/dev/secret-service.py, the products' suites' own keyring.

Each test starts a session bus of its own and the service on it, and talks
to it as a Secret Service client does. Needs dbus-daemon, python3-dbus and
python3-gi (Linux); skipped elsewhere.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SERVICE = REPO / "tools" / "dev" / "secret-service.py"
SECRET = b"sk-ant-test-0-SECRET"

try:
    import dbus
    import dbus.bus
    from dbus.mainloop.glib import DBusGMainLoop
    from gi.repository import GLib

    HAVE_DBUS = shutil.which("dbus-daemon") is not None
except ImportError:
    HAVE_DBUS = False


@unittest.skipUnless(HAVE_DBUS, "needs dbus-daemon, python3-dbus and python3-gi")
class SecretService(unittest.TestCase):
    def setUp(self) -> None:
        out = subprocess.run(["dbus-daemon", "--session", "--fork", "--print-address=1", "--print-pid=1"],
                             check=True, capture_output=True, text=True).stdout.split()
        self.address, self.bus_pid = out[0], int(out[1])
        env = dict(os.environ, DBUS_SESSION_BUS_ADDRESS=self.address)
        self.service = subprocess.Popen([sys.executable, str(SERVICE)], env=env, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, text=True)
        self.assertEqual(self.service.stdout.readline().strip(), "Secret Service ready")
        DBusGMainLoop(set_as_default=True)
        self.bus = dbus.bus.BusConnection(self.address)
        self.root = self.bus.get_object("org.freedesktop.secrets", "/org/freedesktop/secrets")
        self.keyring = dbus.Interface(self.root, "org.freedesktop.Secret.Service")

    def tearDown(self) -> None:
        self.bus.close()
        self.service.terminate()
        out, err = self.service.communicate(timeout=10)
        os.kill(self.bus_pid, 15)
        # What it was given is never printed.
        self.assertNotIn(SECRET.decode(), out + err)

    def prompt(self, path: str) -> tuple[bool, list]:
        """Shows the prompt at `path` and waits for its answer."""
        loop = GLib.MainLoop()
        answer: list = []

        def completed(dismissed, result) -> None:
            answer.extend([bool(dismissed), [str(p) for p in result]])
            loop.quit()

        self.bus.add_signal_receiver(completed, "Completed", "org.freedesktop.Secret.Prompt", path=path)
        dbus.Interface(self.bus.get_object("org.freedesktop.secrets", path),
                       "org.freedesktop.Secret.Prompt").Prompt("")
        GLib.timeout_add_seconds(5, loop.quit)
        loop.run()
        self.assertTrue(answer, "the prompt never answered")
        return answer[0], answer[1]

    def secret(self, session, value: bytes) -> dbus.Struct:
        return dbus.Struct((session, dbus.ByteArray(b""), dbus.ByteArray(value), "text/plain"), signature="oayays")

    def test_a_key_is_kept_read_replaced_and_deleted(self) -> None:
        _, session = self.keyring.OpenSession("plain", dbus.String("", variant_level=1))
        path = self.keyring.ReadAlias("default")
        self.assertNotEqual(path, "/")
        collection = dbus.Interface(self.bus.get_object("org.freedesktop.secrets", path),
                                    "org.freedesktop.Secret.Collection")
        attributes = dbus.Dictionary({"application": "QQ Studio", "target": "a test"}, signature="ss")
        properties = dbus.Dictionary({"org.freedesktop.Secret.Item.Label": "a test",
                                      "org.freedesktop.Secret.Item.Attributes": attributes}, signature="sv")

        # It starts locked: nothing is kept until a prompt unlocks it.
        with self.assertRaises(dbus.DBusException) as locked:
            collection.CreateItem(properties, self.secret(session, SECRET), True)
        self.assertEqual(locked.exception.get_dbus_name(), "org.freedesktop.Secret.Error.IsLocked")
        unlocked, prompt = self.keyring.Unlock([path])
        self.assertEqual(list(unlocked), [])
        self.assertEqual(self.prompt(prompt), (False, [str(path)]))
        unlocked, prompt = self.keyring.Unlock([path])
        self.assertEqual((list(unlocked), prompt), ([path], "/"))

        item, prompt = collection.CreateItem(properties, self.secret(session, SECRET), True)
        self.assertEqual(prompt, "/")
        found, still_locked = self.keyring.SearchItems(attributes)
        self.assertEqual((list(found), list(still_locked)), ([item], []))
        handle = self.bus.get_object("org.freedesktop.secrets", item)
        kept = dbus.Interface(handle, "org.freedesktop.Secret.Item").GetSecret(session, byte_arrays=True)
        self.assertEqual(bytes(kept[2]), SECRET)

        # Kept again under the same attributes, it replaces what was there.
        again, _ = collection.CreateItem(properties, self.secret(session, b"another"), True)
        self.assertEqual(again, item)
        kept = dbus.Interface(handle, "org.freedesktop.Secret.Item").GetSecret(session, byte_arrays=True)
        self.assertEqual(bytes(kept[2]), b"another")

        self.assertEqual(dbus.Interface(handle, "org.freedesktop.Secret.Item").Delete(), "/")
        self.assertEqual([list(x) for x in self.keyring.SearchItems(attributes)], [[], []])

    def test_a_search_says_what_is_locked(self) -> None:
        _, session = self.keyring.OpenSession("plain", dbus.String("", variant_level=1))
        path = self.keyring.ReadAlias("default")
        self.prompt(self.keyring.Unlock([path])[1])
        collection = dbus.Interface(self.bus.get_object("org.freedesktop.secrets", path),
                                    "org.freedesktop.Secret.Collection")
        attributes = dbus.Dictionary({"target": "locked"}, signature="ss")
        item, _ = collection.CreateItem({"org.freedesktop.Secret.Item.Attributes": attributes},
                                        self.secret(session, SECRET), True)
        self.keyring.Lock([path])
        self.assertEqual([list(x) for x in self.keyring.SearchItems(attributes)], [[], [item]])

    def test_only_plain_sessions_are_served(self) -> None:
        with self.assertRaises(dbus.DBusException) as refused:
            self.keyring.OpenSession("dh-ietf1024-sha256-aes128-cbc-pkcs7", dbus.ByteArray(b"x", variant_level=1))
        self.assertEqual(refused.exception.get_dbus_name(), "org.freedesktop.DBus.Error.NotSupported")


if __name__ == "__main__":
    unittest.main()
