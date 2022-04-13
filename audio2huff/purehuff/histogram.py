"""
purehuff - A puristic Huffman encoding module for Python

Thomas Grill, 2011
http://grrrr.org/purehuff
"""

from collections import defaultdict

def histogram(seq,normalize=False):
    d = defaultdict(int)
    for s in seq:
        d[s] += 1
    if normalize:
        nf = 1./sum(d.itervalues())
    else:
        nf = 1
    return [(v*nf,k) for k,v in d.iteritems()]

