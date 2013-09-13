#!/usr/bin/env python2

import sys, os
import logging
import argparse
import r2p
from helpers import autoint, verbosity2level


def _create_argsparser():
    parser = argparse.ArgumentParser(
        description='R2P app bootloader'
    )
    
    parser.add_argument(
        '-v', '--verbose', required=False, action='count', default=0,
        help='verbosity level (default %(default)s): 0=critical, 1=error, 2=warning, 3=info, 4=debug',
        dest='verbosity'
    )
    
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        '-l', '--last', required=False, action='store_true',
        help='removes only the last application',
        dest='last'
    )
    group.add_argument(
        '-a', '--all', required=False, action='store_true',
        help='removes all the apps',
        dest='all'
    )
    
    group = parser.add_argument_group('transport setup')
    group.add_argument(
        '-p', '--transport', required=False, nargs='+',
        default=['DebugTransport', 'SerialLineIO', '/dev/ttyUSB0', 115200],
        help='transport parameters',
        dest='transport', metavar='PARAMS'
    )
    group.add_argument(
        '-t', '--boot-topic', required=True,
        help='name of the bootloader topic for the target R2P module; format: "[\\w]{1,%d}"' % r2p.MODULE_NAME_MAX_LENGTH,
        dest='topic', metavar='BOOT_TOPIC'
    )
    
    return parser


def _main():
    parser = _create_argsparser()
    args = parser.parse_args()
    
    logging.basicConfig(stream=sys.stderr, level=verbosity2level(int(args.verbosity)))
    
    # TODO: Automate transport construction from "--transport" args
    assert args.transport[0] == 'DebugTransport'
    assert args.transport[1] == 'SerialLineIO'
    lineio = r2p.SerialLineIO(str(args.transport[2]), int(args.transport[3]))
    transport = r2p.DebugTransport(lineio)
    bootloader = r2p.Bootloader(transport, args.topic)
    
    try:
        transport.open()
        bootloader.stop()
        if args.last:
            bootloader.remove_last()
        elif args.all:
            bootloader.remove_all()
        else:
            raise RuntimeException('Missing "remove last/all" action switch')
        transport.close()

    except KeyboardInterrupt:
        try:
            transport.close()
        except:
            pass
    
    except:
        try:
            transport.close()
        except:
            pass
        print '~' * 80
        raise


if __name__ == '__main__':
    _main()

