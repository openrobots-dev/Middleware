
import r2p

def _autoint(value):
    value = str(value)
    if value.strip().lower()[0:2] == '0x':
        return int(value[2:], 16) 
    else:
        return int(value)

def _main(argc, argv):
    lineio = r2p.StdLineIO()
    topic = "BOOT_IMU_0" # TODO: argv
    transport = r2p.DebugTransport(lineio, topic)
    bootloader = r2p.SimpleBootloader(transport)

    # Get the LD parameters
    if argc < 7:
        raise ValueError('Command line:\n\tappname hexpath stacklen ldexe ldscript ldoself ldmap ldobjelf ldobj*\nGiven:\n\t' + str(sys.argv))
    appname  = argv[1]
    hexpath  = argv[2]
    stacklen = _autoint(argv[3]) 
    ldexe    = argv[4]
    ldscript = argv[5]
    ldoself  = argv[6]
    ldmap    = argv[7]
    ldobjelf = argv[8]
    ldobjs   = argv[9:]
    
    bootloader.run(appname, hexpath, stacklen, ldexe, ldscript, ldoself, ldmap, ldobjelf, ldobjs)


if __name__ == '__main__':
    _main(len(sys.argv), sys.argv)

