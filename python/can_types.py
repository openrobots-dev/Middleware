class DATA(Union):
    _fields_=[("data_c_byte", c_byte * 8),
              ("data_string", c_char_p)]

class TPCANMsg(Structure):
    _fields_=[("ID", c_int),
              ("MSGTYPE", c_byte),
              ("LEN", c_byte),
              ("DATA", c_byte * 8)]

class TPCANRdMsg(Structure):
    _fields_=[("Msg", TPCANMsg),
              ("dwTime", c_int),
              ("wUsec", c_int)]
