categories = ["ambit"]

microcode = ""
for category in categories:
    exec(f"from . import {category} as cat")
    microcode += cat.microcode
