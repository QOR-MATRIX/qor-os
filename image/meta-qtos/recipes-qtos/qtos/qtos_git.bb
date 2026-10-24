SUMMARY = "QTOS: Prism compositor, Lumen design system, services and first-party apps"
HOMEPAGE = "https://github.com/QOR-MATRIX/QTOS"
LICENSE = "CLOSED"

SRC_URI = "git://github.com/QOR-MATRIX/QTOS.git;protocol=ssh;branch=main \
           file://qtos-session.service \
          "
SRCREV = "${AUTOREV}"
PV = "0.1.0+git"

S = "${WORKDIR}/git"

inherit qt6-cmake systemd

DEPENDS += "qtbase qtdeclarative qtwayland qtremoteobjects qtremoteobjects-native \
            qtshadertools qtshadertools-native qtsvg qtgraphs"

EXTRA_OECMAKE += " \
    -DQTOS_BUILD_TESTS=OFF \
    -DQTOS_BUILD_COMPOSITOR=ON \
    -DQTOS_BUILD_PLATFORM=ON \
"

SYSTEMD_SERVICE:${PN} = "qtos-session.service"

do_install:append() {
    install -Dm0644 ${WORKDIR}/qtos-session.service ${D}${systemd_system_unitdir}/qtos-session.service
}

FILES:${PN} += "${systemd_system_unitdir} ${libdir}/plugins ${prefix}/plugins"
