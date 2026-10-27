.pragma library
// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Simulated file index used by Files and by Beam's file search until
// qor-indexd (KFileMetaData + SQLite FTS5) is available.

const KINDS = {
    "folder":   { glyph: "folder",       hue: 248, label: "Folder" },
    "document": { glyph: "file-text",    hue: 230, label: "Document" },
    "image":    { glyph: "file-image",   hue: 155, label: "Image" },
    "audio":    { glyph: "file-music",   hue: 350, label: "Audio" },
    "video":    { glyph: "file-video",   hue: 30,  label: "Video" },
    "code":     { glyph: "file-code",    hue: 274, label: "Source code" },
    "slides":   { glyph: "presentation", hue: 60,  label: "Presentation" },
    "archive":  { glyph: "archive",      hue: 95,  label: "Archive" }
};

const FOLDERS = [
    { id: "Recents",   icon: "clock" },
    { id: "Documents", icon: "file-text" },
    { id: "Pictures",  icon: "images" },
    { id: "Music",     icon: "music" },
    { id: "Downloads", icon: "download" },
    { id: "Projects",  icon: "code-xml" }
];

const TAGS = [
    { id: "Lumen",    hue: 274 },
    { id: "Personal", hue: 155 },
    { id: "Urgent",   hue: 25 }
];

const FILES = [
    ["Documents", "QOR OS Vision.md", "document", "48 KB", "Today, 09:12", "Lumen"],
    ["Documents", "Lumen Design Language.pdf", "document", "6.2 MB", "Today, 08:40", "Lumen"],
    ["Documents", "Quarterly Plan.odt", "document", "212 KB", "Yesterday", ""],
    ["Documents", "Prism Roadmap.odp", "slides", "3.4 MB", "Mon", "Lumen"],
    ["Documents", "Tax Return 2026.pdf", "document", "1.1 MB", "12 Oct", "Urgent"],
    ["Documents", "Reading List.md", "document", "6 KB", "3 Oct", "Personal"],
    ["Pictures", "Aurora over Tromsø.heic", "image", "4.8 MB", "Today, 07:55", "Personal"],
    ["Pictures", "Glass Study 01.png", "image", "2.3 MB", "Yesterday", "Lumen"],
    ["Pictures", "Glass Study 02.png", "image", "2.6 MB", "Yesterday", "Lumen"],
    ["Pictures", "Studio Desk.jpg", "image", "3.1 MB", "Sun", ""],
    ["Pictures", "Night Market.jpg", "image", "5.0 MB", "Sat", "Personal"],
    ["Pictures", "Shelf Icons.svg", "image", "88 KB", "Fri", "Lumen"],
    ["Music", "Glass Horizon.flac", "audio", "38 MB", "Today, 10:02", ""],
    ["Music", "Soft Machines.flac", "audio", "31 MB", "Today, 10:02", ""],
    ["Music", "Afterglow Drive.flac", "audio", "42 MB", "Yesterday", ""],
    ["Music", "Field Recording — Rain.wav", "audio", "120 MB", "2 Oct", "Personal"],
    ["Downloads", "qt-online-installer.exe", "archive", "48 MB", "Today, 06:30", ""],
    ["Downloads", "Inter-4.1.zip", "archive", "32 MB", "Yesterday", "Lumen"],
    ["Downloads", "Keynote Recording.mp4", "video", "820 MB", "Tue", ""],
    ["Downloads", "Spatial Demo.mov", "video", "1.4 GB", "Mon", "Lumen"],
    ["Projects", "prism-compositor", "folder", "—", "Today, 11:20", "Lumen"],
    ["Projects", "lumen-kit", "folder", "—", "Today, 10:48", "Lumen"],
    ["Projects", "Wallpaper.frag", "code", "4 KB", "Today, 10:15", "Lumen"],
    ["Projects", "Spring.cpp", "code", "7 KB", "Yesterday", "Lumen"],
    ["Projects", "qor-image.bb", "code", "2 KB", "Mon", ""]
];

function kind(id) {
    return KINDS[id] || KINDS.document;
}

function folders() {
    return FOLDERS;
}

function tags() {
    return TAGS;
}

function tagHue(tag) {
    for (const t of TAGS)
        if (t.id === tag)
            return t.hue;
    return -1;
}

function allFiles() {
    return FILES.map(function (f) {
        const k = kind(f[2]);
        return {
            folder: f[0], name: f[1], kind: f[2], size: f[3], modified: f[4], tag: f[5],
            path: "/home/user/" + f[0] + "/" + f[1], glyph: k.glyph, hue: k.hue, kindLabel: k.label,
            tagHue: tagHue(f[5])
        };
    });
}

// Files for a sidebar location: a folder, "Recents" (most recent first) or a tag.
function filesIn(location) {
    const all = allFiles();
    if (location === "Recents")
        return all.filter(f => f.modified.indexOf("Today") === 0 || f.modified === "Yesterday");
    for (const t of TAGS)
        if (t.id === location)
            return all.filter(f => f.tag === location);
    return all.filter(f => f.folder === location);
}
