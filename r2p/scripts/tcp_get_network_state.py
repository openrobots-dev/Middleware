#!/usr/bin/env python2

import sys, os, struct
import logging
import argparse
import r2p
from helpers import *


def _create_argsparser():
    parser = argparse.ArgumentParser(
        description='R2P get network state'
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
        '-t', '--r2p-topic', required=False, default='R2P',
        help='name of the R2P topic; format: "[\\w]{1,%d}"' % r2p.MODULE_NAME_MAX_LENGTH,
        dest='topic', metavar='R2P_TOPIC'
    )
    
    return parser


def _main():
    parser = _create_argsparser()
    args = parser.parse_args()
    
    logging.basicConfig(stream=sys.stderr, level=verbosity2level(int(args.verbosity)))
    
    # TODO: Automate transport construction from "--transport" args
#    assert args.transport[0] == 'DebugTransport'
#    assert args.transport[1] == 'SerialLineIO'
#    lineio = r2p.SerialLineIO(str(args.transport[2]), int(args.transport[3]))
    lineio = r2p.TCPLineIO('10.0.0.12', 23)
    transport = r2p.DebugTransport(lineio)
    
    payload = struct.pack('<B', 0x20).ljust(32, '\0')
    
    try:
        transport.open()
        logging.info('Requesting network state')
        transport.send_message(args.topic, payload)
        while True:
            fields = transport.recv()
            if fields[0] == r2p.Transport.TypeEnum.MESSAGE and fields[1] == args.topic:
                logging.info('  ' + (fields[2]))
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

