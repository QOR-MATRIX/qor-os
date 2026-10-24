# image — the QTOS OS image (Phase 2 skeleton)

> **Status:** skeleton. Layer structure, distro policy, image and recipe are
> in place but have not been built yet. The first build is a Phase 2 task
> ([ROADMAP](../docs/ROADMAP.md#phase-2--bootable-alpha)).

| Path | Contents |
|------|----------|
| `kas/qtos-qemux86-64.yml` | kas build: poky + meta-openembedded + meta-qt6 + meta-qtos, `qemux86-64` |
| `meta-qtos/conf/layer.conf` | Layer definition (`LAYERSERIES_COMPAT` scarthgap) |
| `meta-qtos/conf/distro/qtos.conf` | Distro policy: systemd, Wayland, Vulkan, no X11 |
| `meta-qtos/recipes-core/images/qtos-image.bb` | Image: QTOS + Qt modules + substrate daemons |
| `meta-qtos/recipes-qtos/qtos/qtos_git.bb` | Builds this repository with `qt6-cmake` |
| `meta-qtos/recipes-qtos/qtos/files/qtos-session.service` | systemd unit starting `prism` on tty1 via eglfs/KMS |

```bash
pip install kas
kas build image/kas/qtos-qemux86-64.yml
kas shell image/kas/qtos-qemux86-64.yml -c 'runqemu qemux86-64 kvm sdl gl'
```

## Phase 2 checklist

- [ ] Pin every layer to a commit SHA and choose the meta-qt6 branch matching Qt 6.12
- [ ] Decide between meta-qt6 and commercial **Boot2Qt** (`meta-boot2qt`, included with the Qt Enterprise license)
- [ ] The commercial Qt build in Yocto needs licensed sources: document the account and mirror setup
- [ ] Read-only EROFS root with dm-verity, btrfs data partition, LUKS2 with TPM2
- [ ] RAUC A/B bundles and rollback
- [ ] greetd + Prism greeter; flicker-free boot from the BGRT logo
- [ ] Reference hardware machine config
