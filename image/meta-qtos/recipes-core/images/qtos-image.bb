SUMMARY = "QTOS reference image: Prism compositor, Lumen shell and QTOS services"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "splash ssh-server-openssh"

IMAGE_INSTALL:append = " \
    qtos \
    qtbase qtdeclarative qtwayland qtremoteobjects qtshadertools qtsvg qtgraphs \
    qtmultimedia qtquick3d qtvirtualkeyboard \
    networkmanager bluez5 upower pipewire wireplumber \
    fontconfig \
"

# Room for the QML caches and shader pipeline caches written on first boot.
IMAGE_ROOTFS_EXTRA_SPACE = "524288"
