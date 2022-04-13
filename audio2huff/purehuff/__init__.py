"""
purehuff - A puristic Huffman encoding module for Python

Thomas Grill, 2011
http://grrrr.org/purehuff
"""

import heapq
from bitarray import BitArray
from histogram import histogram

class HuffEncoder:
    "Simplistic Huffman encoder class"
    def __init__(self,huffdict):
        self.huffdict = dict(huffdict)
        
    def __call__(self,s):
        "encode symbol or sequence of symbols"
        try:
            it = iter(s)
        except TypeError:
            # return bit code for one symbol
            return self.huffdict[s]
        else:
            # return bit array for iterator
            ret = BitArray()
            for s in it:
                ret += self.huffdict[s]
            return ret
        
    def __repr__(self):
        return "HuffEncoder(%s)"%repr(self.huffdict)


class HuffDecoder:
    "Zero memory Huffman decoder class"
    def __init__(self,huff):
        self.huff = tuple(huff)
        
    def __call__(self,bitstream):
        "returns generator for data decoded from input bit stream"
        data = iter(bitstream)
        while True:
            huffix = 0
            while True:
                b = data.next()  # get bit
                if b:
                    offs = self.huff[huffix]
                    if offs:
                        huffix += offs+1
                    else:
                        huffix += 2
                        
                if not self.huff[huffix]:
                    yield self.huff[huffix+1] # yield code symbol
                    break
                else:
                    huffix += 1
                    
    def __repr__(self):
        return "HuffDecoder(%s)"%repr(self.huff)

class HuffTree:
    """
    Huffman encoder class
    Based on http://en.literateprograms.org/Special:Downloadcode/Huffman_coding_%28Python%29
    """
    def __init__(self,hist):
        trees = list(hist)
        heapq.heapify(trees)
        while len(trees) > 1:
            childR, childL = heapq.heappop(trees), heapq.heappop(trees)
            parent = (childL[0] + childR[0], childL, childR)
            heapq.heappush(trees, parent)
        self.tree = trees[0]

    def encoder(self):
        "get decoder instance"
        return HuffEncoder(HuffTree._getdict(self.tree))

    def decoder(self):
        "get decoder instance"
        return HuffDecoder(HuffTree._flatten(self.tree))

    @staticmethod
    def _getdict(huffTree,bits=BitArray()):
        if len(huffTree) == 2:
            yield huffTree[1],bits
        else:
            for ix,h in enumerate(huffTree[1:3]):
                for y in HuffTree._getdict(h,bits+(ix,)):
                    yield y
            
    @staticmethod
    def _flatten(huffTree):
        assert len(huffTree) > 2
        l = []
        for h in huffTree[1:3]:
            if len(h) == 2:
                l.append(0)
                l.append(h[1])
            else:
                t = HuffTree._flatten(h)
                l.append(len(t))
                l += t
        return l

import unittest

class TestHuffman(unittest.TestCase):
    def test_encoding(self,range=50,length=1000):
        import random
        testdata = [random.randint(0,range) for _ in xrange(length)]
        
        hist = histogram(testdata)
        hufftree = HuffTree(hist)
        
        encoder = hufftree.encoder()
        encdata = list(encoder(testdata))
        
        decoder = hufftree.decoder()
        decdata = list(decoder(encdata))
        
        self.assertEqual(testdata,decdata)       

if __name__ == "__main__":
    unittest.main()
