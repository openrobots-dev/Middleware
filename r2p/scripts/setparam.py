#!/usr/bin/env python2

import sys, os
import logging
import argparse
import r2p
from helpers import hexb2str, autoint, verbosity2level


def _create_argsparser():
    parser = argparse.ArgumentParser(
        description='R2P app bootloader'
    )
    
    parser.add_argument(
        '-v', '--verbose', required=False, action='count', default=0,
        help='verbosity level (default %(default)s): 0=critical, 1=error, 2=warning, 3=info, 4=debug',
        dest='verbosity'
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
    
    group = parser.add_argument_group('parameter setup')
    group.add_argument(
        '-n', '--app-name', required=True,
        help='name of the app (R2P Node); format: "[\\w]{1,%d}"' % r2p.NODE_NAME_MAX_LENGTH,
        dest='appname', metavar='APP_NAME'
    )
    group.add_argument(
        '-o', '--param-offset', required=True,
        help='offset of the target parameter, relative to the app config struct',
        dest='offset', metavar='OFFSET'
    )
    group.add_argument(
        '-b', '--param-bytes', required=True,
        help='parameter value, as hex byte stream; format: "([0-9A-Fa-f]{2})+"',
        dest='value', metavar='HEX_BYTES'
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
        bootloader.set_parameter(appname = str(args.appname),
                                 offset = autoint(args.offset),
                                 value = hexb2str(str(args.value)))
        transport.close()
        print str2hexb(bytes)

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

