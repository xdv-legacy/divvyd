#!/usr/bin/env python

import sys
from divvy.util import Sign

result = Sign.run_command(sys.argv[1:])
exit(0 if result else -1)
