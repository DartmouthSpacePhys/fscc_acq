#!/usr/bin/python2
#06/04/2014
#I am here just trying to make sure that the data we pull in from
#the FSCC-LVDS serial card in 'transparent' (i.e., blind to
#what sort of data it is reading in--no sync bytes, no header 
#recognition, etc.) mode is pulling in data chunks of equal size.
#The big idear is to simply look at the number of bytes between
#instances of the word "Dart" in the binary data.
#
#A related issue in X-Sync (i.e., sync to 'Dart' or some other
#specified pattern) mode is that the card appears to get out
#of sync with some clock or other when it searches for 'Dart'.
#

import struct,binascii,os, sys

from collections import Counter

df = "synchr_test.data"
d_word = b"lege"

if len(sys.argv) <= 1:
    print("./check_data.py INPUT_FILE SEARCH_WORD")
    print("\tNote: SEARCH_WORD is some repeating word in the data")
    sys.exit(1)

if len(sys.argv) >= 2:
    df = sys.argv[1]
    if len(sys.argv) == 3:
        d_word = sys.argv[2]
    elif len(sys.argv) > 3:
        print("Too many arguments!")
        print("./check_data.py INPUT_FILE SEARCH_WORD")
        print("\tNote: SEARCH_WORD is some repeating word in the data")
        sys.exit(1)

print("Data file: " + df)
print("Search word: " + d_word)
print

d_stat = os.stat(df)

f = open(df,"rb")

dread=f.read(d_stat.st_size)
#Given such a file size, we expect about 26 frames


pos_d = [0,dread.find(d_word)]

while True:
    pos_d.append(dread.find(d_word,pos_d[-1]+1))
    if pos_d[-1] == -1:
        break
        
diff = [j-i for i, j in zip(pos_d[:-1], pos_d[1:])]

c = Counter(diff)

print("Data info:")
print(c)
print

f.close()
