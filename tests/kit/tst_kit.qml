// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtTest
import QOR.Kit

TestCase {
    id: testCase
    name: "Kit"
    when: windowShown
    width: 400
    height: 300

    Component {
        id: springComponent
        Spring { }
    }

    function test_tokensGenerated() {
        compare(Tokens.name, "Lumen");
        verify(Tokens.accentPresets.length >= 6);
        compare(Tokens.radius.window, 14);
        verify(Tokens.typography.body.size > 0);
        compare(Tokens.elevation.length, 6);
    }

    function test_themeResolvesEveryRole() {
        const roles = ["background", "surface", "surfaceRaised", "glassTint", "textPrimary", "textSecondary",
                       "accent", "onAccent", "separator", "shadow", "positive", "critical"];
        for (const role of roles)
            verify(Theme[role].a > 0 || role === "separator", role);
    }

    function test_darkTextIsLightOnDark() {
        Theme.dark = true;
        tryVerify(() => Theme.darkness > 0.999, 3000);
        verify(Theme.textPrimary.hslLightness > 0.9);
        Theme.dark = false;
        tryVerify(() => Theme.darkness < 0.001, 3000);
        verify(Theme.textPrimary.hslLightness < 0.2);
        Theme.dark = true;
    }

    function test_typography() {
        compare(Theme.body.pixelSize, Tokens.typography.body.size);
        compare(Theme.hero.pixelSize, Tokens.typography.hero.size);
        verify(Theme.fontFamily.length > 0);
    }

    function test_springAnimatesInQml() {
        const spring = createTemporaryObject(springComponent, testCase, { value: 0, target: 1 });
        verify(spring.running);
        tryCompare(spring, "value", 1, 3000);
        verify(!spring.running);
    }

    function test_oklchSingleton() {
        const c = Oklch.color(0.7, 0.15, 274);
        verify(c.b > c.r);
        const lch = Oklch.toOklch(c);
        fuzzyCompare(lch[0], 0.7, 0.01);
    }
}
