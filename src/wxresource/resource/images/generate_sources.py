#!/usr/bin/python

import glob, os
import sys
from pathlib import Path

for p in Path(".").glob("*.png"):
    print(f'm_images["{p.stem.lower()}"] = wxBITMAP_PNG_FROM_DATA({p.stem}_16x16);')

with open('../../../main/Icons.h', 'w') as sys.stdout:

    PNG2C = os.getenv('WX_WIN') + "/misc/scripts/png2c.py"

    sys.argv = ['python', PNG2C, '-s'] + [f for f in glob.glob("*.png")]

    print("#ifndef _ICONS_H_\n#define _ICONS_H_\n")
    exec(open(PNG2C).read());
    print("\n#endif // _ICONS_H_")