Works in python2:

```
$ docker run --rm -it -v $PWD:/python:z ubuntu:18.04

docker $ apt update; apt install -y python-virtualenv build-essential python-dev python-numpy libsndfile-dev

docker $ cd python/

docker $ virtualenv .venv

docker $ . .venv/bin/activate

docker $ pip install numpy

docker $ pip install pysndfile

docker $ pip install scikits.audiolab

docker $ python audio2huff.py --help

docker $ python audio2huff.py --bits=8 --sndfile=honk_honk_8.wav --hdrfile=honk_honk.h
```
