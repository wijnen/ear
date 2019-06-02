# Database read and write support.
import logging
import fhs
import os
import media

FRAGMENTS = 'fragments.txt'
INDENT = '\t'
EOL = '\r\n'
exts = ('.mp3', '.ogg', '.wav', '.mp4', '.ogv', '.mpg', '.mpeg', '.avi','m4a','aac') #TODO: Get these from whatever GStreamer can read

def makepath(root, filename):
    '''Write a filename in a form that can be used to detect duplicates'''
    return os.path.realpath(os.path.abspath(os.path.join(root, filename)))

def parse_fragments(root, tracks, used, usednames):
    '''Read tracks from a FRAGMENTS file and insert them into tracks parameter.
    Note: this does not add the 'end' marker to the tracks.'''
    with open(os.path.join(root, FRAGMENTS), 'r',encoding = 'utf-8') as f:
        state = 'TRACK' #What are we looking for? A TRACK, VALUES, or FRAGMENTS?
        
        for linenumber,line in enumerate(f.readlines()):
            try:
                # Remove ending whitespace and starting BOM.
                line = line.lstrip('\ufeff').rstrip()
                # Empty lines and lines with # as the first character are ignored.
                # Note that while this allows for comments in the file, they are
                # not written back.
                # Also note that fragment and group names can start with #, because
                # their first character is always indentation.
                if line.strip() == '' or line.startswith('#'):
                    continue
                if ':' in line:
                    key, value = line.split(':', 1)
                    value = value.strip()
                else:
                    key, value = None, None
                if state == 'FRAGMENTS':
                    # Read fragments.
                    indent = len(line) - len(line.lstrip())
                    while indent < istack[-1]:
                        stack.pop()
                        istack.pop()
                    if indent > istack[-1]:
                        if need_indent:
                            istack.append(indent)
                        else:
                            raise NotImplementedError('Annotations are not currently supported: {}:{}\n {}'.format(root,linenumber,line))
                    elif need_indent:
                        raise ValueError('Expected indent')
                    need_indent = False
                    if indent == 0:
                        # Fragments are done, read next track.
                        state = 'TRACK'
                    else:
                        if line[-1] == ':': 
                            # New group. If we're in a group, this is a group in a group, so a child group.
                            need_indent = True
                            new_group = ['group', line[:-1].strip(), []]
                            stack[-1].append(new_group)
                            stack.append(new_group[2])  # Add the children of the new group to the stack, because that is now the active group.
                        else:
                            name, start = line.rsplit(None, 1)
                            name = name.strip()
                            try:
                                start = int(start)
                            except:
                                start = int(float(start))
                            stack[-1].append(['fragment', name, start])
                if state == 'TRACK':
                    if key != 'Track':
                        raise ValueError('New track expected; got: ' + line)
                    state = 'VALUES'
                if state in ('VALUES', 'TRACK'):
                    if key == 'Track':
                        current = { 'root': root, 'name': value, 'files': [], 'fragments': [], 'tags' : [], 'dirty' : False }
                        if current['name'] in usednames: 
                            origname = current['name']
                            i = 1
                            while current['name'] in usednames:
                                current['name'] = origname + " ({0})".format(i)
                        usednames.add(current['name'])
                        tracks.append(current) #We're appending here, but because current is a dict, and dicts are mutable, we can still write to it and get the changes through. No return needed
                        
                        tags = autotag(value, root) 
                        if len(tags)>0:
                            current['tags'].extend(tags)
                    elif key == 'Tags':
                        current['tags'].append(value.split(';'))
                        if "no-auto" in value:
                            current['tags'] = value.split(';')
                    elif key == 'File':
                        parts = value.split(';')
                        filename = parts[0].strip()
                        fileargs = [x.split('=') for x in parts[1:]]
                        offsets = [x[1] for x in fileargs if x[0] == 'offset'] 
                        if len(offsets) == 1:
                            offset = int(offsets[0])
                        else:
                            offset = 0
                        current['files'].append((filename, offset))
                        used.add(makepath(root, filename))
                    elif key =='AnnounceFile': #An audio file containing a recording annoucning this dance. Used for balls and maybe performances. Can be played before the dance.
                        parts = value.split(';')
                        filename = parts[0].strip()
                        fileargs = [x.split('=') for x in parts[1:]]
                        offsets = [x[1] for x in fileargs if x[0] == 'offset'] 
                        if len(offsets) == 1:
                            offset = int(offsets[0])
                        else:
                            offset = 0
                        current['announcefiles'].append((filename, offset))
                        used.add(makepath(root, filename))
                        
                    elif key == 'Fragments':
                        need_indent = True
                        stack = [current['fragments']]
                        istack = [0]
                        state = 'FRAGMENTS'
                        current['tags'].append("has fragments")
            except Exception as e:
                   logging.warning("Exception raised while parsing file {}, line  {}".format(os.path.join(root, FRAGMENTS),linenumber))
                   logging.exception(e)


def autotag(trackname, root):
    """Function to automagically add some tags based on filenames. Contents are up for discussion"""
    #TODO Mark these tags as autotags so we don't write them and end up with repeat tags
    names = {"wals": ["wals","waltz","valse"], "polka":["polka"]}
    tags = set()
    parts = [root]
    path = root
    
    while True:
        parts = os.path.split(path)
        if path in parts:
            break #The check has to be made in the middle in of the loop (between splitting and setting the new vars. That's why we have a while true and a break. Of course, it could be recursive... 
        path = parts[0]
        tags.add(parts[1].lower())

    for tag, options in names.items():
        for option in options:
            if option in trackname.lower():
                tags.add(tag.lower())
                break
    return list(tags)

def add_unfragmented_file(filename, root, usednames):
    current = { 'root': root, 'name': filename, 'files': [(filename, 0)], 'fragments': [] , 'tags' : autotag(filename,root), "dirty" : False}
    if current['name'] in usednames: 
        origname = current['name']
        i = 1
        while current['name'] in usednames:
            current['name'] = origname + " ({0})".format(i)
    usednames.add(current['name'])
    current['fragments'].append(["fragment",filename,0])
    return current

def read(use_cache = True, unique_names = False):
    '''Read all db files from all fhs data directories (and pwd)
    Return list of tracks.'''
    # db = list of tracks. 
    # track = { 'name': name, 'files': list of (filename, offset), 'announcefiles' : list of (filename, offset),'fragments': list of fragments and groups, 'end': time }
    # fragment = ( 'fragment', name, start_time )
    # group = ( 'group', name, list of fragments and groups )
    tracks = []
    basedirs = fhs.read_data('db', dir = True, opened = False, multiple = True)
    if use_cache:
        try:
            import pickle
            logging.info ("Trying to use cache from {}".format(os.path.join(basedirs[0],"cache.pickle")))
            tracks = pickle.load(open(os.path.join(basedirs[0],"cache.pickle"),'rb'))
        except Exception as e: #TODO: Narrow exception class.
            logging.info("Cache not found")
        if len(tracks) > 1:
            return tracks
    # Parse all fragments files.
    used = set()
    usednames = set()
    for dirname in basedirs:
        for root, dirs, files in os.walk(dirname, followlinks=True):
            if FRAGMENTS in files:
                if makepath(root, FRAGMENTS) in used:
                    continue
                used.add(makepath(root, FRAGMENTS))
                parse_fragments(root, tracks, used, usednames)
    # Add all other files.
    for dirname in basedirs:
        for root, dirs, files in os.walk(dirname, followlinks=True):
            root = os.path.abspath(root)
            for filename in files:
                if makepath(root, filename) in used:
                    continue
                if os.path.splitext(filename)[1] not in exts:
                    continue
                tracks.append(add_unfragmented_file(filename, root, usednames))
                used.add(makepath(filename,root))
    tracks = load_test_tracks(tracks)
    try:
        import pickle
        logging.info ("Writing cache")
        pickle.dump(tracks,open(os.path.join(basedirs[0],"cache.pickle"),'wb'))
    except Exception as e:
        logging.warning ("Writing cache failed")
        logging.exception (e)
    return tracks

def get_times(fragments):
    """Returns a flattened list of all times in the fragments"""
    times = set()
    for fragment in fragments:
        if fragment[0]!='fragment' and type(fragment) == list:
            times.update( get_times(fragment[2]))
        else:
            if type(fragment[2]) == int:
                times.add(fragment[2])
    return times

def add_end_times(group,times):
    ret = []
    for fragment in group:
        
        if fragment[0] == 'group':
            ret.append([fragment[0],fragment[1],add_end_times(fragment[2],times)])
        else:
            ret.append((fragment[0],fragment[1],fragment[2],times[times.index(fragment[2])+1]))
    return ret

def load_test_tracks(tracks):
    """Opens all media files to insert end markers. Does not just do a single end marker, but also handles all fragments to give start and end times. This duplicates info, but makes it all a lot easier to handle"""
    outtracks=[]
    for track in tracks:
        durations = []
        for filename,offset in track['files']:
            fullpath = os.path.join(track['root'], filename)
            durations.append( media.Media.get_duration(fullpath) + offset)
        times = []
        end = int(max(durations))
        #print(end, track['name'])
        if end <10:
            logging.info("removing {} from db because of length {}".format(track['name'],end))
            logging.info("durations: {}".format(durations))
            continue
        times = sorted(list(get_times(track['fragments'])))
        if times[0] > 0: 
            track['fragments'].insert(0, ["fragment", "Intro", 0] )
            times = sorted(list(get_times(track['fragments'])))
        times.append(end)
        track['fragments'] = add_end_times(track['fragments'],times)
        outtracks.append(track)
    return outtracks


def write_group(group, indent):
    '''Recursively write a group of fragments'''
    ret = ''
    for fragment in group:
        assert len(fragment[1].strip())>0
        if fragment[0] == 'group':
            ret += '{}{}:{}'.format(indent, fragment[1], EOL)
            ret += write_group(fragment[2], indent + INDENT)
        elif fragment[0] == 'fragment':
            ret += '{}{}\t{}{}'.format(indent, fragment[1], int(fragment[2]), EOL)
        else:
            assert(fragment[0] == 'end')
            # Ignore.
    return ret

def write_track(track):
    # Every track starts with an empty line and the track name.
    # There are two reasons for the empty line: it separates the tracks,
    # and it lets the BOM be on a line of its own, so grep can find lines
    # that start with 'Track:' without missing the first.  
    #TODO: Add writing of tags
    ret = ''
    ret += '{}Track:{}{}'.format(EOL, track['name'], EOL)
    for file in track['files']:
        if file[1] != 0:
            ret += 'File:{};offset={}{}'.format(file[0], file[1], EOL)
        else:
            ret += 'File:{}{}'.format(file[0], EOL)
    ret += 'Fragments:{}'.format(EOL)
    ret += write_group(track['fragments'], INDENT)
    return ret

def clear_cache():
    basedirs = fhs.read_data('db', dir = True, opened = False, multiple = True)
    cachefile = os.path.join(basedirs[0],"cache.pickle");
    if os.path.exists(cachefile):
        logging.info ("Clearing cache file {}".format(os.path.join(basedirs[0],"cache.pickle")))
        try:
            os.remove(cachefile)
        except Exception as e:
            logging.exception("Clearing cache failed",e)
     


def write(tracks):
    '''Write the internal track data to a file that can be read back with db.read()'''
    
    output = {}
     
    for track in tracks:
        # Don't define tracks without fragments.
        if len(track['fragments']) == 0:
            continue
        if len(track['fragments'])==1 and track['fragments'][0][2]==0:
            continue
        target = makepath(track['root'], FRAGMENTS)
        if target not in output:
            output[target] = {'contents':'','dirty':False}
        output[target]['contents'] += write_track(track)
        if 'dirty' not in output[target].keys():
             output[target]['dirty'] = False
        output[target]['dirty'] += track['dirty']

    for target in output.keys():
        # Write as binary and manually encode as utf-8, so line endings are not mangled.
        if output[target]['dirty']:
            with open(target, 'wb') as f: #This should probably do a try-except or something or check for disk space
                # To allow Microsoft to write these files as utf-8, start with a BOM.
                f.write(('\ufeff' + output[target]['contents']).encode('utf-8'))
# vim: set expandtab tabstop=4 shiftwidth=4 :
