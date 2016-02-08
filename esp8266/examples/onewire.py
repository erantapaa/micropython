import esp


class DS:
    @staticmethod
    def calc_crc(indata):
        crc = 0
        for data in indata:
            crc = crc ^ data
            for ii in range(8):
                if crc & 0x01:
                    crc = (crc >> 1) ^ 0x8C
                else:
                    crc >>= 1
        return crc

    @staticmethod
    def convert(LowByte, HighByte):
        reading = (HighByte << 8) + LowByte
        sign_bit = reading & 0x8000
        if sign_bit:
            reading = (reading ^ 0xffff) + 1  # 2's comp
        Tc_100 = (6 * reading) + reading // 4    # multiply by (100 * 0.0625) or 6.25
        Whole = Tc_100 // 100
        Fract = Tc_100 % 100
        return Whole, Fract

    def __init__(self, port=5):
        self.dev = esp.one_wire(port, edge=esp.gpio.INTR_ANYEDGE)

    def get_rom(self):
        self.dev.reset()
        self.dev.write([esp.one_wire.READ_ROM], suppress_skip=True)    # 33 means send id, suppress_skip means don't send a suppress skip rom command
        data = self.dev.read(8)
        print(data)
        print("CRC", self.calc_crc(data[:-1]), data[-1])

    def convert_temp(self):
        self.dev.reset()
        self.dev.write([esp.one_wire.CONVERT_T])

    def get_temp(self):
        self.dev.reset()
        self.dev.write([esp.one_wire.READ_SCRATCH_PAD])
        dd = self.dev.read(9)
        print(dd)
        print("CRC", self.calc_crc(dd[:-1]), dd[-1])
        print(self.convert(*dd[:2]))
