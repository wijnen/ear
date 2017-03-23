#!/usr/bin/env python3

#import fuzzywuzzy
from fuzzywuzzy import process
import dbread
import fhs
config = fhs.init({"webUI":True, 'gtkUI':False })
from collections import OrderedDict


search = "Quadrille"
musts = [""]
mays = [""]

db = dbread.read()
tick=time.time()
tracks={}
for i,track in enumerate(db):
    for must in musts:
        if must not in track['tags']:
            continue
    #for tag in track['tags']:
    #    if tag not in mays:
    #        continue
    tracks[track['name']]=i
    #Change mays in must not 
    #Actually, discuss filtering
out = process.extract(search,tracks.keys(),limit=100)
toSend =OrderedDict()
for res in out:
    print(res)
    toSend[res[0]]=tracks[res[0]]
import pprint
pprint.pprint(toSend)
"""
> choices = ["Atlanta Falcons", "New York Jets", "New York Giants", "Dallas Cowboys"]
>>> process.extract("new york jets", choices, limit=2)
    [('New York Jets', 100), ('New York Giants', 78)]
>>> process.extractOne("cowboys", choices)
    ("Dallas Cowboys", 90)
# vim: set expandtab tabstop=4 shiftwidth=4 :
"""
