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

    has_old_json = False
    old_json = None
    old_files = None
    old_md5 = dict()
    if len(sys.argv) > 3:
        has_old_json = True
        try:
            old_json = json.load(open(sys.argv[3]))
        except:
            has_old_json = False
        if has_old_json:
            old_files = old_json['files']
            for f in old_files:
                old_md5[f['check_path']] = f

    client = dropbox.client.DropboxClient(open('key.txt').read())

    for folder, subs, files in os.walk(sys.argv[1] + '/' + db_folder):
        folder = os.path.abspath(folder).replace('\\', '/')
        db_path = folder[folder.find(db_folder):]
        for filename in files:
            real_filename = folder + '/' + filename
            filename = db_path + '/' + filename
            file_md5 = md5(real_filename)
            file = filename[filename.find(db_folder) + len(db_folder):]

            if has_old_json and file in old_md5.keys() and old_md5[file]['md5'] == file_md5:
                #print('file with the same md5 has already a link')
                url = old_md5[file]['url']
            else:
                f = client.share(filename, False)
                url = f['url']
                url = url[:len(url)-1] + '1'
                print(url)

            # add to json
            obj = dict()
            obj['name'] = os.path.basename(filename)
            obj['url'] = url
            obj['md5'] = file_md5
            obj['check_path'] = file
            obj['packed'] = False

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