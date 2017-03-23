#!/usr/bin/env python3
import fuzzywuzzy
import dbread

search = "Keizerwals"
musts = ["wals"]
mays = [""]

db = dbread.read()
tracks={}
for i,track in enumerate(db):
    for must in musts:
        if must not in track['tags']:
            continue
    #for tag in track['tags']:
    #    if tag not in mays:
    #        continue
    tracks[track['title']]=i
    #Change mays in must not 
    #Actually, discuss filtering
		
out = fuzzywuzzy.process.extract(search,tracks.keys())
toSend ={}
for res in out:
	toSend[res[0]]=tracks[res[0]]

print(toSend)
"""
> choices = ["Atlanta Falcons", "New York Jets", "New York Giants", "Dallas Cowboys"]
>>> process.extract("new york jets", choices, limit=2)
    [('New York Jets', 100), ('New York Giants', 78)]
>>> process.extractOne("cowboys", choices)
    ("Dallas Cowboys", 90)
# vim: set expandtab tabstop=4 shiftwidth=4 :
"""
