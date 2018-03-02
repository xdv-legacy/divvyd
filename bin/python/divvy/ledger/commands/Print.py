from __future__ import absolute_import, division, print_function, unicode_literals

from divvy.ledger.Args import ARGS
from divvy.ledger import SearchLedgers

import json

SAFE = True

HELP = """print

Print the ledgers to stdout.  The default command."""

def run_print(server):
    ARGS.display(print, server, SearchLedgers.search(server))
