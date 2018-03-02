from __future__ import absolute_import, division, print_function, unicode_literals

import sys

from divvy.ledger.Args import ARGS
from divvy.util import Log
from divvy.util import Range
from divvy.util import Search

def search(server):
    """Yields a stream of ledger numbers that match the given condition."""
    condition = lambda number: ARGS.condition(server, number)
    ledgers = server.ledgers
    if ARGS.binary:
        try:
            position = Search.FIRST if ARGS.position == 'first' else Search.LAST
            yield Search.binary_search(
                ledgers[0], ledgers[-1], condition, position)
        except:
            Log.fatal('No ledgers matching condition "%s".' % condition,
                      file=sys.stderr)
    else:
        for x in Search.linear_search(ledgers, condition):
            yield x
