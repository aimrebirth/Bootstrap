#!/usr/bin/python3
# -*- coding: utf-8 -*-

import dropbox
import hashlib
import json
import os
import sys

def main():
    if len(sys.argv) < 3:
        print('Usage: make_links.py abs_path_to_dropbox rel_path_to_dir_to_process [old.json]')
        return
    data = []
    db_folder = sys.argv[2].replace('\\', '/')
    base_name = os.path.basename(db_folder)
    
    has_old_json = True
    old_json = dict()
    if len(sys.argv) > 3:
        try:
            j = json.load(open(sys.argv[3]))
            old_files = j['files']
            for f in old_files:
                old_json[f['check_path']] = f
        except:
            has_old_json = False

    client = dropbox.client.DropboxClient(open('key.txt').read())

    for folder, subs, files in os.walk(sys.argv[1] + '/' + db_folder):
        folder = os.path.abspath(folder).replace('\\', '/')
        db_path = folder[folder.find(db_folder):]
        for filename in files:
            real_filename = folder + '/' + filename
            filename = db_path + '/' + filename
            file = filename[filename.find(db_folder) + len(db_folder):]
            obj = dict()
            lwt = int(os.path.getmtime(real_filename))

            if has_old_json and file in old_json.keys():
                if 'lwt' in old_json[file].keys() and old_json[file]['lwt'] == lwt:
                    url = old_json[file]['url']
                    obj['md5'] = old_json[file]['md5']
                else:
                    file_md5 = md5(real_filename)
                    if old_json[file]['md5'] == file_md5:
                        url = old_json[file]['url']
                    obj['md5'] = file_md5
            else:
                obj['md5'] = md5(real_filename)

                f = client.share(filename, False)
                url = f['url']
                url = url[:len(url)-1] + '1'
                print(url)

            # add to json
            obj['name'] = os.path.basename(filename)
            obj['url'] = url
            obj['check_path'] = file
            obj['packed'] = False
            obj['lwt'] = lwt

            data.append(obj)
    json_data = dict()
    json_data['files'] = data
    json.dump(json_data, open(base_name + '.json', 'w'), indent = 2, sort_keys = True)

def md5(file):
    md5 = hashlib.md5()
    f = open(file, mode = 'rb')
    while True:
        data = f.read(2 ** 20)
        if not data:
            break
        md5.update(data)
    return md5.hexdigest()

if __name__ == '__main__':
    main()