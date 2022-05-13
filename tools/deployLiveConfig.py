#!/usr/bin/env python
import argparse
import json
import os
import tempfile


if __name__ == "__main__":

    scriptdir = os.path.dirname(os.path.realpath(__file__))
    default_key_path = scriptdir + "/../meta/config_encryption_key"

    parser = argparse.ArgumentParser(description='Deploy a live config')

    parser.add_argument('config', help='The path to the config file')
    parser.add_argument('metadata', nargs='?', help='The path to the metadata file, default is <config>.json')
    parser.add_argument('-t', '--deploy-target', type=str, dest='target', default='mategames-api',
                        help='The ssh alias of the target host')
    parser.add_argument('-r', '--deploy-root', type=str, dest='root', default='configurations',
                        help='The root of the configuration files on the target')
    parser.add_argument('-k', '--orxcrypt-keyfile', type=str, dest='keyfile',
                        default=default_key_path, help='The path to the orxcrypt key file')

    args = parser.parse_args()

    try:
        with open(args.keyfile) as keyfile:
            key = keyfile.readlines()[0]
    except:
        print('Failed to load the key from %s' % args.keyfile)
        raise

    try:
        meta_path = args.metadata or args.config + '.json'
        with open(meta_path) as meta_file:
            meta = json.load(meta_file)
    except:
        print('Failed to load a json object from %s' % meta_path)
        raise

    game = meta['game']
    for pname, pdetails in meta['platforms'].items():
        encrypted = pdetails.get('encrypted', False)
        for vname, vdetails in pdetails['versions'].items():
            if encrypted:
                temp_res = tempfile.mkstemp()
                file_to_serve = temp_res[1]
                print("Encrypting %s as %s using key %s" %
                      (args.config, file_to_serve, key))
                os.system("orxcrypt -f %s -o %s -k %s" % (args.config, file_to_serve, key))
            else:
                file_to_serve = args.config

            target_folder = "%s/%s/%s/%s" % (args.root, game, pname, vname)

            print("Creating folder %s on %s" % (target_folder, args.target))
            os.system("ssh %s mkdir -p %s" % (args.target, target_folder))

            target_file = "%s/liveconfig" % target_folder
            command = "scp %s %s:%s" % \
                      (file_to_serve, args.target, target_file)
            print("Copying %s to %s on %s" % (file_to_serve, target_file, args.target))
            os.system(command)
            if encrypted:
                os.remove(file_to_serve)
