"""
purehuff - A puristic Huffman encoding module for Python

Thomas Grill, 2011
http://grrrr.org/purehuff
"""

from copy import deepcopy

class BitArray:
    "Bit array"
    def __init__(self,data=()):
        try:
            self.data = bytearray()
        except NameError:
            # for Python version < 2.6
            self.data = list()  # becomes list of ints
        self.bits = 0
        self += data
    def copy(self):
        return deepcopy(self)
    def __cmp__(self,other):
        clen = cmp(self.bits,other.bits)
        return clen if clen else cmp(self.data,other.data)
    def __len__(self):
        return self.bits
    def _getbit(self,ix):
        return (self.data[ix>>3]>>(7-(ix&7)))&1
    def _clip(self,ix,mn,mx):
        if mn > mx: mn,mx = mx,mn
        return max(mn,min(ix if ix >= 0 else self.bits+ix,mx))
    def __getitem__(self,ix):
        if type(ix) is slice:
            step = 1 if ix.step is None else ix.step
            if step >= 0:
                start = 0 if ix.start is None else self._clip(ix.start,0,self.bits)
                stop = self.bits if ix.stop is None else self._clip(ix.stop,0,self.bits)
            else:
                start = self.bits-1 if ix.start is None else self._clip(ix.start,self.bits-1,-1)
                stop = -1 if ix.stop is None else self._clip(ix.stop,self.bits-1,-1)
#            print "slice",ix,">",start,stop,step
            return BitArray(self._getbit(ix) for ix in xrange(start,stop,step))
        else:
            assert 0 <= ix < self.bits 
            return self._getbit(ix)
    def __iadd__(self,data):
        for d in data:
            self.append(d)
        return self
    def __add__(self,other):
        ret = self.copy()
        ret += other
        return ret
    def append(self,b):
        if type(b) is str: b = int(b)
        bitpos = self.bits&7
        bitor = (1<<(7-bitpos)) if b else 0
        if not bitpos:
            self.data.append(bitor)
        else:
            self.data[-1] |= bitor
        self.bits += 1
    def __iter__(self):
        return (self._getbit(ix) for ix in xrange(self.bits))
    def __str__(self):
        return "".join(str(s) for s in self)
    def __repr__(self):
        return "BitArray('%s')"%str(self)

import unittest

class TestBitArray(unittest.TestCase):
    def test_inout(self,length=7777):
        import random
        testdata = [random.randint(0,1) for _ in xrange(length)]
        
        ba = BitArray(testdata)
        self.assertEqual(len(ba),length)       
        
        outdata = list(ba)
        self.assertEqual(testdata,outdata)       

    def test_slicings(self,length=77,runs=1000):
        import random
        testdata = [random.randint(0,1) for _ in xrange(length)]
        ba = BitArray(testdata)

        for _ in xrange(runs):
            step = random.randint(-length/5,length/5)
            sl = slice(random.randint(-length/2,3*length/2),random.randint(-length/2,3*length/2),step if step else 1)
            self.assertEqual(testdata[sl],list(ba[sl]))       

    def test_repr(self,length=777):
        import random
        testdata = [random.randint(0,1) for _ in xrange(length)]
        ba = BitArray(testdata)
        rba = repr(ba)
        ba2 = BitArray(ba)
        self.assertEqual(ba,ba2)
        self.assertEqual(rba,repr(ba2))
        self.assertEqual(ba,eval(rba))

if __name__ == "__main__":
    unittest.main()

