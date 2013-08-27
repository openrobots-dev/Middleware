#!/usr/bin/env python2

import sys, os
import logging
import r2p

def _readint(value):
    value = str(value).strip().lower()
    if value[0:2] == '0x':
        return int(value[2:], 16) 
    else:
        return int(value)

def _main(appname, boot_topic, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, *ldobjs):
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)

    lineio = r2p.SerialLineIO('/dev/ttyUSB0', 115200) # TODO: by command line
    #lineio = r2p.StdLineIO()
    transport = r2p.DebugTransport(lineio)
    bootloader = r2p.SimpleBootloader(transport, boot_topic)

    stacklen = _readint(stacklen)
    bootloader.run(appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs)


if __name__ == '__main__':
    if len(sys.argv) < 8:
        raise ValueError("Usage:\n\t%s appname boot_topic hexpath stacklen ldexe ldscript ldoself ldmap ldobjelf ldobjs..." % sys.argv[0])
    _main(*sys.argv[1:])

