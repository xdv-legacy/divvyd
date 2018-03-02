#!/usr/bin/env python

from __future__ import absolute_import, division, print_function, unicode_literals

import sys
import traceback

from divvy.ledger import Server
from divvy.ledger.commands import Cache, Info, Print
from divvy.ledger.Args import ARGS
from divvy.util import Log
from divvy.util.CommandList import CommandList

_COMMANDS = CommandList(Cache, Info, Print)

if __name__ == '__main__':
    try:
        server = Server.Server()
        args = list(ARGS.command)
        _COMMANDS.run_safe(args.pop(0), server, *args)
    except Exception as e:
        if ARGS.verbose:
            print(traceback.format_exc(), sys.stderr)
        Log.error(e)
