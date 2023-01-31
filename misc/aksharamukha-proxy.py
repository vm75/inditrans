#!/usr/bin/python

import argparse
import re
import sys
from os.path import exists
from aksharamukha import transliterate
from datetime import datetime


def convert(src, dst, text, nativize, time, pre_options, post_options):
    start_time = datetime.now()
    response = transliterate.process(
        args.src,
        args.dst,
        text,
        nativize,
        pre_options,
        post_options,
    )

    if time:
        time_taken = datetime.now() - start_time
        print(
            "Time taken for", src, "to", dst, ":", (time_taken.total_seconds() * 1000)
        )
    else:
        sys.stdout.write(response)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--pre", default=[], action="append")
    parser.add_argument("--post", default=[], action="append")
    parser.add_argument("--nativize")
    parser.add_argument("--src", required=True)
    parser.add_argument("--dst", required=True)
    parser.add_argument("--time")
    parser.add_argument("--len")
    parser.add_argument("--file")
    args = parser.parse_args()

    if args.nativize:
        nativize = True
    else:
        nativize = False

    if args.time:
        time = True
    else:
        time = False

    if args.file and exists(args.file):
        inFile = open(args.file)
        text = inFile.read()
        convert(
            args.src,
            args.dst,
            text,
            nativize,
            time,
            pre_options=args.pre,
            post_options=args.post,
        )
    else:
        if args.len is not None:
            text = sys.stdin.read(int(args.len))
            convert(
                args.src,
                args.dst,
                text,
                nativize,
                time,
                pre_options=args.pre,
                post_options=args.post,
            )
        else:
            while True:
                text = sys.stdin.readline()
                if text == "":
                    break
                convert(
                    args.src,
                    args.dst,
                    text,
                    nativize,
                    time,
                    pre_options=args.pre,
                    post_options=args.post,
                )
