import os.path

application = defines.get("app", "build/Ipe.app")
appname = os.path.basename(application)

files = [
    (application, appname),
    ("doc/news.txt", "news.txt"),
    ("doc/gpl.txt", "gpl.txt"),
]

symlinks = { "Applications": "/Applications" }

icon = "build/ipe.icns"

background = "artwork/ipe-dmg-background.png"

window_rect = ((100, 100), (540, 455))

icon_locations = {
    "Applications": (420, 290),
    appname: (180, 290),
    "news.txt": (300, 80),
    "gpl.txt": (440, 80),
}
