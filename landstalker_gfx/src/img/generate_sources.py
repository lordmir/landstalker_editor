#!/usr/bin/python

import glob, os
import sys

with open('../include/Icons.h', 'w') as sys.stdout:

    PNG2C = os.getenv('WX_WIN') + "/misc/scripts/png2c.py"

    sys.argv = ['png2c.py', '-s'] + [f for f in glob.glob("*.png")]

    print("#ifndef _ICONS_H_\n#define _ICONS_H_\n")
    exec(open(PNG2C).read());
    print("\n#endif // _ICONS_H_")
