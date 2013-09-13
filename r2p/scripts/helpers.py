import logging


def verbosity2level(verbosity):
    if   verbosity <= 0: return logging.CRITICAL
    elif verbosity == 1: return logging.ERROR
    elif verbosity == 2: return logging.WARNING
    elif verbosity == 3: return logging.INFO
    elif verbosity >= 4: return logging.DEBUG


def str2hexb(data):
    return ''.join([ ('%.2X' % ord(b)) for b in data ])


def hexb2str(data):
    assert (len(data) & 1) == 0
    return ''.join([ chr(int(data[i : i + 2], 16)) for i in range(0, len(data), 2) ])


def autoint(value):
    value = str(value).strip().lower()
    if value[0:2] == '0x':
        return int(value[2:], 16) 
    else:
        return int(value)

